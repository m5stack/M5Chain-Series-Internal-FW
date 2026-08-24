#include "spa06_sensor.h"
#include <math.h>
#include <string.h>

#define SPA06_HAL_ADDR(addr) ((uint16_t)((addr) << 1))

static const uint32_t s_spa06_scale_factors[8] = {524288U, 1572864U, 3670016U, 7864320U,
                                                  253952U, 516096U,  1040384U, 2088960U};

static uint8_t spa06_mode_is_valid(uint8_t mode)
{
    return (mode == SPA06_IDLE) || (mode == SPA06_ONE_PRESSURE) || (mode == SPA06_ONE_TEMPERATURE) ||
                   (mode == SPA06_CONT_PRESSURE) || (mode == SPA06_CONT_TEMPERATURE) || (mode == SPA06_CONT_PRES_TEMP)
               ? 1U
               : 0U;
}

spa06_sensor_t spa06_sensor;
volatile uint32_t spa06_pressure_pa       = 0;
volatile int32_t spa06_temperature_c_x100 = 0;
volatile int32_t spa06_altitude_cm        = 0;
volatile uint8_t spa06_sensor_last_status = 0;

static int32_t spa06_twos_complement(uint32_t raw, uint8_t bits)
{
    return ((int32_t)(raw << (32U - bits))) >> (32U - bits);
}

static int32_t spa06_double_to_int32_x100(double value)
{
    double scaled = value * 100.0;

    if (scaled > 2147483647.0) {
        return INT32_MAX;
    }

    if (scaled < -2147483648.0) {
        return INT32_MIN;
    }

    return (int32_t)scaled;
}

static uint32_t spa06_double_to_uint24(double value)
{
    if (value <= 0.0) {
        return 0U;
    }

    if (value > 16777215.0) {
        return 0xFFFFFFU;
    }

    return (uint32_t)(value + 0.5);
}

static int32_t spa06_altitude_m_to_cm(double value)
{
    double scaled = value * 100.0;

    if (scaled > 2147483647.0) {
        return INT32_MAX;
    }

    if (scaled < -2147483648.0) {
        return INT32_MIN;
    }

    return (int32_t)((scaled >= 0.0) ? (scaled + 0.5) : (scaled - 0.5));
}

static uint32_t spa06_pressure_pa_to_uint24(double pressure_pa)
{
    return spa06_double_to_uint24(pressure_pa);
}

static double spa06_pressure_pa_to_altitude_m(double pressure_pa)
{
    double sea_level_pressure_pa = spa06_sensor.sea_level_pressure_pa;
    double altitude              = 0.0;

    if (sea_level_pressure_pa <= 0.0) {
        sea_level_pressure_pa = SPA06_DEFAULT_SEA_LEVEL_PRESSURE_PA;
    }

    altitude = pressure_pa / sea_level_pressure_pa;
    altitude = 1.0 - pow(altitude, 1.0 / 5.255);
    return 44330.0 * altitude;
}

static uint8_t spa06_reg_read(uint8_t reg, uint8_t *data, uint16_t len)
{
    if ((data == NULL) || (len == 0U) || (spa06_sensor.i2c == NULL)) {
        return 1;
    }

    if (HAL_I2C_Mem_Read(spa06_sensor.i2c, SPA06_HAL_ADDR(spa06_sensor.address), reg, I2C_MEMADD_SIZE_8BIT, data, len,
                         SPA06_I2C_TIMEOUT_MS) != HAL_OK) {
        return 1;
    }

    return 0;
}

static uint8_t spa06_reg_read_byte(uint8_t reg, uint8_t *value)
{
    return spa06_reg_read(reg, value, 1U);
}

static uint8_t spa06_reg_read_integer(uint8_t reg, uint8_t len, uint32_t *value)
{
    uint8_t buf[4] = {0};
    uint32_t out   = 0;

    if ((value == NULL) || (len == 0U) || (len > sizeof(buf))) {
        return 1;
    }

    if (spa06_reg_read(reg, buf, len) != 0U) {
        return 1;
    }

    for (uint8_t i = 0; i < len; i++) {
        out = (out << 8) | buf[i];
    }

    *value = out;
    return 0;
}

static uint8_t spa06_reg_write_byte(uint8_t reg, uint8_t value)
{
    HAL_Delay(1U);

    if (spa06_sensor.i2c == NULL) {
        return 1;
    }

    if (HAL_I2C_Mem_Write(spa06_sensor.i2c, SPA06_HAL_ADDR(spa06_sensor.address), reg, I2C_MEMADD_SIZE_8BIT, &value, 1U,
                          SPA06_I2C_TIMEOUT_MS) != HAL_OK) {
        return 1;
    }

    return 0;
}

static uint8_t spa06_reg_modify_byte(uint8_t reg, uint8_t msb, uint8_t lsb, uint8_t value)
{
    uint8_t reg_value = 0;
    uint8_t mask      = 0;

    if ((msb > 7U) || (lsb > msb)) {
        return 1;
    }

    if (spa06_reg_read_byte(reg, &reg_value) != 0U) {
        return 1;
    }

    mask = (uint8_t)(((1U << (msb - lsb + 1U)) - 1U) << lsb);
    reg_value &= (uint8_t)~mask;
    reg_value |= (uint8_t)((value << lsb) & mask);

    return spa06_reg_write_byte(reg, reg_value);
}

static uint8_t spa06_wait_reg_bit(uint8_t reg, uint8_t bit, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t value  = 0;

    do {
        if (spa06_reg_read_byte(reg, &value) != 0U) {
            return 1;
        }

        if ((value & (uint8_t)(1U << bit)) != 0U) {
            return 0;
        }

        HAL_Delay(1U);
    } while ((HAL_GetTick() - start) < timeout_ms);

    return 1;
}

static uint8_t spa06_wait_data_ready(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t status = 0;

    do {
        if (spa06_reg_read_byte(SPA06_REG_MEAS_CFG, &status) != 0U) {
            return 1;
        }

        if ((status & 0x30U) == 0x30U) {
            return 0;
        }

        HAL_Delay(5U);
    } while ((HAL_GetTick() - start) < timeout_ms);

    return 1;
}

static uint8_t spa06_read_coefficients(void)
{
    uint8_t coef[21] = {0};
    uint32_t temp    = 0;

    if (spa06_wait_reg_bit(SPA06_REG_MEAS_CFG, 7U, 100U) != 0U) {
        return 1;
    }

    if (spa06_reg_read(SPA06_REG_COEF, coef, sizeof(coef)) != 0U) {
        return 1;
    }

    temp            = (((uint16_t)coef[0] << 4) & 0x0FF0U) | ((coef[1] >> 4) & 0x000FU);
    spa06_sensor.c0 = (int16_t)spa06_twos_complement(temp, 12U);

    temp            = (((uint16_t)coef[1] << 8) & 0x0F00U) | coef[2];
    spa06_sensor.c1 = (int16_t)spa06_twos_complement(temp, 12U);

    temp = (((uint32_t)coef[3] << 12) & 0xFF000U) | (((uint16_t)coef[4] << 4) & 0x00FF0U) | ((coef[5] >> 4) & 0x0000FU);
    spa06_sensor.c00 = spa06_twos_complement(temp, 20U);

    temp             = (((uint32_t)coef[5] << 16) & 0xF0000U) | (((uint16_t)coef[6] << 8) & 0x0FF00U) | coef[7];
    spa06_sensor.c10 = spa06_twos_complement(temp, 20U);

    temp             = ((uint16_t)coef[8] << 8) | coef[9];
    spa06_sensor.c01 = (int16_t)spa06_twos_complement(temp, 16U);

    temp             = ((uint16_t)coef[10] << 8) | coef[11];
    spa06_sensor.c11 = (int16_t)spa06_twos_complement(temp, 16U);

    temp             = ((uint16_t)coef[12] << 8) | coef[13];
    spa06_sensor.c20 = (int16_t)spa06_twos_complement(temp, 16U);

    temp             = ((uint16_t)coef[14] << 8) | coef[15];
    spa06_sensor.c21 = (int16_t)spa06_twos_complement(temp, 16U);

    temp             = ((uint16_t)coef[16] << 8) | coef[17];
    spa06_sensor.c30 = (int16_t)spa06_twos_complement(temp, 16U);

    temp             = (((uint16_t)coef[18] << 4) & 0x0FF0U) | ((coef[19] >> 4) & 0x000FU);
    spa06_sensor.c31 = (int16_t)spa06_twos_complement(temp, 12U);

    temp             = (((uint16_t)coef[19] << 8) & 0x0F00U) | coef[20];
    spa06_sensor.c40 = (int16_t)spa06_twos_complement(temp, 12U);

    return 0;
}

uint8_t spa06_sensor_init(void)
{
    return spa06_sensor_init_with_addr(SPA06_ADDR_ALT);
}

uint8_t spa06_sensor_init_with_addr(uint8_t address)
{
    uint8_t id = 0;

    memset(&spa06_sensor, 0, sizeof(spa06_sensor));
    spa06_sensor.i2c                   = &hi2c2;
    spa06_sensor.address               = address;
    spa06_sensor.sea_level_pressure_pa = SPA06_DEFAULT_SEA_LEVEL_PRESSURE_PA;

    if (HAL_I2C_GetState(spa06_sensor.i2c) == HAL_I2C_STATE_RESET) {
        MX_I2C2_Init();
    }

    HAL_Delay(12U);

    if (spa06_reg_read_byte(SPA06_REG_ID, &id) != 0U) {
        return 1;
    }

    if (id != SPA06_EXPECTED_ID) {
        return 1;
    }

    if (spa06_sensor_reset() != 0U) {
        return 1;
    }

    if (spa06_read_coefficients() != 0U) {
        return 1;
    }

    if (spa06_sensor_set_pressure_config(SPA06_4HZ, SPA06_64SAMPLES) != 0U) {
        return 1;
    }

    if (spa06_sensor_set_temperature_config(SPA06_4HZ, SPA06_8SAMPLES) != 0U) {
        return 1;
    }

    if (spa06_sensor_set_mode(SPA06_CONT_PRES_TEMP) != 0U) {
        return 1;
    }

    if (spa06_wait_data_ready(500U) != 0U) {
        return 1;
    }

    spa06_sensor.inited = 1U;
    return 0;
}

uint8_t spa06_sensor_deinit(void)
{
    uint8_t ret = spa06_sensor_set_mode(SPA06_IDLE);

    spa06_sensor.inited = 0U;
    return ret;
}

uint8_t spa06_sensor_reset(void)
{
    if (spa06_reg_write_byte(SPA06_REG_RESET, 0x09U) != 0U) {
        return 1;
    }

    HAL_Delay(12U);
    return spa06_wait_reg_bit(SPA06_REG_MEAS_CFG, 6U, 100U);
}

uint8_t spa06_sensor_set_mode(spa06_mode_t mode)
{
    uint8_t ret = spa06_reg_modify_byte(SPA06_REG_MEAS_CFG, 2U, 0U, (uint8_t)mode);

    if (ret == 0U) {
        spa06_sensor.mode     = mode;
        spa06_sensor.meas_cfg = (uint8_t)((spa06_sensor.meas_cfg & 0xF8U) | ((uint8_t)mode & 0x07U));
    }

    return ret;
}

uint8_t spa06_sensor_set_pressure_config(spa06_measure_rate_t rate, spa06_oversample_rate_t oversample)
{
    uint8_t shift = (oversample > SPA06_8SAMPLES) ? 1U : 0U;
    uint8_t cfg   = (uint8_t)((((uint8_t)rate << 4) & 0xF0U) | ((uint8_t)oversample & 0x0FU));

    if (spa06_reg_modify_byte(SPA06_REG_CFG_REG, 2U, 2U, shift) != 0U) {
        return 1;
    }

    if (spa06_reg_write_byte(SPA06_REG_PRS_CFG, cfg) != 0U) {
        return 1;
    }

    spa06_sensor.pressure_rate          = rate;
    spa06_sensor.pressure_oversample    = oversample;
    spa06_sensor.pressure_shift_enabled = shift;
    spa06_sensor.cfg_reg                = (uint8_t)((spa06_sensor.cfg_reg & 0xFBU) | (uint8_t)(shift << 2));
    return 0;
}

uint8_t spa06_sensor_set_temperature_config(spa06_measure_rate_t rate, spa06_oversample_rate_t oversample)
{
    uint8_t shift = (oversample > SPA06_8SAMPLES) ? 1U : 0U;
    uint8_t cfg   = (uint8_t)((((uint8_t)rate << 4) & 0xF0U) | ((uint8_t)oversample & 0x0FU));

    if (spa06_reg_modify_byte(SPA06_REG_CFG_REG, 3U, 3U, shift) != 0U) {
        return 1;
    }

    if (spa06_reg_write_byte(SPA06_REG_TMP_CFG, cfg) != 0U) {
        return 1;
    }

    spa06_sensor.temperature_rate          = rate;
    spa06_sensor.temperature_oversample    = oversample;
    spa06_sensor.temperature_shift_enabled = shift;
    spa06_sensor.cfg_reg                   = (uint8_t)((spa06_sensor.cfg_reg & 0xF7U) | (uint8_t)(shift << 3));
    return 0;
}

static uint8_t spa06_sensor_sync_config_from_registers(void)
{
    uint8_t prs_cfg  = 0U;
    uint8_t tmp_cfg  = 0U;
    uint8_t meas_cfg = 0U;
    uint8_t cfg_reg  = 0U;
    uint8_t mode     = 0U;
    spa06_measure_rate_t pressure_rate;
    spa06_measure_rate_t temperature_rate;
    spa06_oversample_rate_t pressure_oversample;
    spa06_oversample_rate_t temperature_oversample;

    if ((spa06_reg_read_byte(SPA06_REG_PRS_CFG, &prs_cfg) != 0U) ||
        (spa06_reg_read_byte(SPA06_REG_TMP_CFG, &tmp_cfg) != 0U) ||
        (spa06_reg_read_byte(SPA06_REG_MEAS_CFG, &meas_cfg) != 0U) ||
        (spa06_reg_read_byte(SPA06_REG_CFG_REG, &cfg_reg) != 0U)) {
        return 1U;
    }

    pressure_rate          = (spa06_measure_rate_t)((prs_cfg >> 4) & 0x0FU);
    pressure_oversample    = (spa06_oversample_rate_t)(prs_cfg & 0x0FU);
    temperature_rate       = (spa06_measure_rate_t)((tmp_cfg >> 4) & 0x0FU);
    temperature_oversample = (spa06_oversample_rate_t)(tmp_cfg & 0x0FU);
    mode                   = meas_cfg & 0x07U;

    if ((pressure_oversample > SPA06_128SAMPLES) || (temperature_oversample > SPA06_128SAMPLES) ||
        (spa06_mode_is_valid(mode) == 0U)) {
        return 1U;
    }

    spa06_sensor.pressure_rate             = pressure_rate;
    spa06_sensor.temperature_rate          = temperature_rate;
    spa06_sensor.pressure_oversample       = pressure_oversample;
    spa06_sensor.temperature_oversample    = temperature_oversample;
    spa06_sensor.mode                      = (spa06_mode_t)mode;
    spa06_sensor.temperature_source        = (spa06_temperature_source_t)((meas_cfg >> 3) & 0x01U);
    spa06_sensor.pressure_shift_enabled    = (uint8_t)((cfg_reg >> 2) & 0x01U);
    spa06_sensor.temperature_shift_enabled = (uint8_t)((cfg_reg >> 3) & 0x01U);
    spa06_sensor.meas_cfg                  = (uint8_t)(meas_cfg & 0x0FU);
    spa06_sensor.cfg_reg                   = cfg_reg;

    return 0U;
}

uint8_t spa06_sensor_apply_config_from_registers(void)
{
    spa06_mode_t requested_mode;

    if (spa06_sensor_sync_config_from_registers() != 0U) {
        return 1U;
    }

    requested_mode = spa06_sensor.mode;
    if (spa06_sensor.inited == 0U || requested_mode == SPA06_IDLE) {
        return 0U;
    }

    if (spa06_sensor_set_mode(SPA06_IDLE) != 0U) {
        return 1U;
    }

    return spa06_sensor_set_mode(requested_mode);
}

uint8_t spa06_sensor_set_temperature_source(spa06_temperature_source_t source)
{
    uint8_t ret = spa06_reg_modify_byte(SPA06_REG_MEAS_CFG, 3U, 3U, (uint8_t)source);

    if (ret == 0U) {
        spa06_sensor.temperature_source = source;
        spa06_sensor.meas_cfg           = (uint8_t)((spa06_sensor.meas_cfg & 0xF7U) | (((uint8_t)source & 0x01U) << 3));
    }

    return ret;
}

uint8_t spa06_sensor_set_interrupt_active_high(uint8_t active_high)
{
    uint8_t ret = spa06_reg_modify_byte(SPA06_REG_CFG_REG, 7U, 7U, active_high ? 1U : 0U);

    if (ret == 0U) {
        spa06_sensor.cfg_reg = (uint8_t)((spa06_sensor.cfg_reg & 0x7FU) | (active_high ? 0x80U : 0U));
    }

    return ret;
}

uint8_t spa06_sensor_configure_interrupt(spa06_interrupt_option_t option)
{
    uint8_t ret = spa06_reg_modify_byte(SPA06_REG_CFG_REG, 6U, 4U, (uint8_t)option);

    if (ret == 0U) {
        spa06_sensor.cfg_reg = (uint8_t)((spa06_sensor.cfg_reg & 0x8FU) | (((uint8_t)option & 0x07U) << 4));
    }

    return ret;
}

uint8_t spa06_sensor_get_interrupt_status(uint8_t *status)
{
    uint8_t value = 0;

    if (status == NULL) {
        return 1;
    }

    if (spa06_reg_read_byte(SPA06_REG_INT_STS, &value) != 0U) {
        return 1;
    }

    *status = value & 0x07U;
    return 0;
}

uint8_t spa06_sensor_pressure_available(uint8_t *available)
{
    uint8_t status = 0;

    if (available == NULL) {
        return 1;
    }

    if (spa06_reg_read_byte(SPA06_REG_MEAS_CFG, &status) != 0U) {
        return 1;
    }

    *available = ((status & 0x10U) != 0U) ? 1U : 0U;
    return 0;
}

uint8_t spa06_sensor_temperature_available(uint8_t *available)
{
    uint8_t status = 0;

    if (available == NULL) {
        return 1;
    }

    if (spa06_reg_read_byte(SPA06_REG_MEAS_CFG, &status) != 0U) {
        return 1;
    }

    *available = ((status & 0x20U) != 0U) ? 1U : 0U;
    return 0;
}

void spa06_sensor_set_pressure_offset(double offset_pa)
{
    spa06_sensor.pressure_offset_pa = offset_pa;
}

void spa06_sensor_set_sea_level_pressure(double pressure_pa)
{
    if (pressure_pa > 0.0) {
        spa06_sensor.sea_level_pressure_pa = pressure_pa;
    }
}

void spa06_sensor_set_temperature_offset(double offset_c)
{
    spa06_sensor.temperature_offset_c = offset_c;
}

static uint8_t spa06_sensor_read_pressure_pa(double *pressure_pa)
{
    uint32_t raw    = 0;
    int32_t p_raw   = 0;
    int32_t t_raw   = 0;
    double p_raw_sc = 0.0;
    double t_raw_sc = 0.0;
    double pressure = 0.0;
    uint32_t k_p    = 0;
    uint32_t k_t    = 0;

    if ((pressure_pa == NULL) || (spa06_sensor.inited == 0U)) {
        return 1;
    }

    if (spa06_reg_read_integer(SPA06_REG_PRS_B2, 3U, &raw) != 0U) {
        return 1;
    }
    p_raw = spa06_twos_complement(raw, 24U);

    if (spa06_reg_read_integer(SPA06_REG_TMP_B2, 3U, &raw) != 0U) {
        return 1;
    }
    t_raw = spa06_twos_complement(raw, 24U);

    k_p      = s_spa06_scale_factors[spa06_sensor.pressure_oversample];
    k_t      = s_spa06_scale_factors[spa06_sensor.temperature_oversample];
    p_raw_sc = (double)p_raw / (double)k_p;
    t_raw_sc = (double)t_raw / (double)k_t;

    pressure = (double)spa06_sensor.c00 +
               p_raw_sc * ((double)spa06_sensor.c10 +
                           p_raw_sc * ((double)spa06_sensor.c20 +
                                       p_raw_sc * ((double)spa06_sensor.c30 + (double)spa06_sensor.c40 * p_raw_sc))) +
               t_raw_sc * ((double)spa06_sensor.c01 +
                           p_raw_sc * ((double)spa06_sensor.c11 +
                                       p_raw_sc * ((double)spa06_sensor.c21 + (double)spa06_sensor.c31 * p_raw_sc))) +
               spa06_sensor.pressure_offset_pa;

    *pressure_pa = pressure;
    return 0;
}

uint8_t spa06_sensor_read_pressure(uint32_t *pressure_pa)
{
    double pressure = 0.0;

    if (pressure_pa == NULL) {
        return 1;
    }

    if (spa06_sensor_read_pressure_pa(&pressure) != 0U) {
        return 1;
    }

    *pressure_pa = spa06_pressure_pa_to_uint24(pressure);
    return 0;
}

uint8_t spa06_sensor_read_temperature(double *temperature_c)
{
    uint32_t raw    = 0;
    int32_t t_raw   = 0;
    uint32_t k_t    = 0;
    double t_raw_sc = 0.0;

    if ((temperature_c == NULL) || (spa06_sensor.inited == 0U)) {
        return 1;
    }

    if (spa06_reg_read_integer(SPA06_REG_TMP_B2, 3U, &raw) != 0U) {
        return 1;
    }

    t_raw    = spa06_twos_complement(raw, 24U);
    k_t      = s_spa06_scale_factors[spa06_sensor.temperature_oversample];
    t_raw_sc = (double)t_raw / (double)k_t;

    *temperature_c =
        ((double)spa06_sensor.c0 * 0.5) + ((double)spa06_sensor.c1 * t_raw_sc) + spa06_sensor.temperature_offset_c;

    return 0;
}

uint8_t spa06_sensor_read_altitude(int32_t *altitude_cm)
{
    double pressure = 0.0;
    double altitude = 0.0;

    if (altitude_cm == NULL) {
        return 1;
    }

    if (spa06_sensor_read_pressure_pa(&pressure) != 0U) {
        return 1;
    }

    altitude     = spa06_pressure_pa_to_altitude_m(pressure);
    *altitude_cm = spa06_altitude_m_to_cm(altitude);

    return 0;
}

uint8_t spa06_sensor_read_all(uint32_t *pressure_pa, double *temperature_c, int32_t *altitude_cm)
{
    double pressure = 0.0;

    if (spa06_sensor_read_pressure_pa(&pressure) != 0U) {
        return 1;
    }

    if (pressure_pa != NULL) {
        *pressure_pa = spa06_pressure_pa_to_uint24(pressure);
    }

    if (temperature_c != NULL) {
        if (spa06_sensor_read_temperature(temperature_c) != 0U) {
            return 1;
        }
    }

    if (altitude_cm != NULL) {
        double altitude = spa06_pressure_pa_to_altitude_m(pressure);
        *altitude_cm    = spa06_altitude_m_to_cm(altitude);
    }

    return 0;
}

uint8_t spa06_data_update(void)
{
    uint32_t pressure         = 0;
    double temperature_c      = 0.0;
    int32_t altitude_cm_value = 0;

    spa06_sensor_last_status = spa06_sensor_read_all(&pressure, &temperature_c, &altitude_cm_value);
    if (spa06_sensor_last_status != 0U) {
        return 1;
    }

    spa06_pressure_pa        = pressure;
    spa06_temperature_c_x100 = spa06_double_to_int32_x100(temperature_c);
    spa06_altitude_cm        = altitude_cm_value;

    return 0;
}
