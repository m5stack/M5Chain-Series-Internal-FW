#include "env_function.h"
#include "RGB.h"
#include <math.h>

#define CHAIN_ENV_RATE_PAYLOAD_SIZE                      (1U)
#define CHAIN_ENV_SEA_LEVEL_PRESS_PAYLOAD_SIZE           (3U)
#define CHAIN_ENV_CALIBRATE_SEA_LEVEL_PRESS_PAYLOAD_SIZE (5U)
#define CHAIN_ENV_CALIBRATE_SAMPLE_COUNT                 (16U)
#define CHAIN_ENV_CALIBRATE_SAMPLE_INTERVAL_MS           (250U)
#define CHAIN_ENV_MODE_PAYLOAD_SIZE                      (1U)
#define CHAIN_ENV_TEMP_THRESHOLD_PAYLOAD_SIZE            (3U)
#define CHAIN_ENV_HUMI_THRESHOLD_PAYLOAD_SIZE            (2U)
#define CHAIN_ENV_PRESS_THRESHOLD_PAYLOAD_SIZE           (4U)
#define CHAIN_ENV_ALTI_THRESHOLD_PAYLOAD_SIZE            (8U)
#define CHAIN_ENV_I2C_MAX_LENGTH                         (32U)

#define CHAIN_ENV_ALARM_MODE_TEMP_MASK            (0x01U)
#define CHAIN_ENV_ALARM_MODE_HUMI_MASK            (0x02U)
#define CHAIN_ENV_ALARM_MODE_PRESS_MASK           (0x04U)
#define CHAIN_ENV_ALARM_MODE_ALTI_MASK            (0x08U)
#define CHAIN_ENV_DEFAULT_SEA_LEVEL_PRESS_HPA_X10 (10133U)
#define CHAIN_ENV_SEA_LEVEL_PRESS_MIN_HPA_X10     (3000U)
#define CHAIN_ENV_SEA_LEVEL_PRESS_MAX_HPA_X10     (11000U)
#define CHAIN_ENV_ALARM_PRESS_MIN_HPA_X10         (7000U)
#define CHAIN_ENV_ALARM_PRESS_MAX_HPA_X10         (12000U)
#define CHAIN_ENV_ALARM_ALTI_DEFAULT_LOW_CM       (0)
#define CHAIN_ENV_ALARM_ALTI_DEFAULT_HIGH_CM      (100000)

#ifndef LIGHT_SENSOR_I2C_TIMEOUT_MS
#define LIGHT_SENSOR_I2C_TIMEOUT_MS (100U)
#endif

static uint8_t s_spa06_temp_rate                = SPA06_4HZ;
static uint8_t s_spa06_press_rate               = SPA06_4HZ;
static uint16_t s_spa06_sea_level_press_hpa_x10 = CHAIN_ENV_DEFAULT_SEA_LEVEL_PRESS_HPA_X10;
static double s_spa06_sea_level_pressure_pa     = SPA06_DEFAULT_SEA_LEVEL_PRESSURE_PA;
static uint8_t s_env_alarm_mode                 = 0x0FU;
static uint8_t s_alarm_temp_device              = CHAIN_ENV_TEMP_DEVICE_AHT20;
static uint8_t s_alarm_temp_low                 = 10U;
static uint8_t s_alarm_temp_high                = 40U;
static uint8_t s_alarm_humi_low                 = 20U;
static uint8_t s_alarm_humi_high                = 80U;
static uint16_t s_alarm_press_low_hpa_x10       = 9500U;
static uint16_t s_alarm_press_high_hpa_x10      = 10500U;
static int32_t s_alarm_alti_low_cm              = CHAIN_ENV_ALARM_ALTI_DEFAULT_LOW_CM;
static int32_t s_alarm_alti_high_cm             = CHAIN_ENV_ALARM_ALTI_DEFAULT_HIGH_CM;

static uint8_t s_temp_zone                              = 0U;
static uint8_t s_humi_zone                              = 0U;
static uint8_t s_press_zone                             = 0U;
static uint8_t s_alti_zone                              = 0U;
static uint8_t s_alarm_event_status                     = 0U;
static uint8_t s_ret_buf[CHAIN_ENV_I2C_MAX_LENGTH + 1U] = {0};
static uint8_t s_ret_buf_size                           = 0U;

HAL_StatusTypeDef LightSensor_I2cMemRead(uint8_t i2c_addr, uint16_t reg_addr, uint16_t reg_addr_size, uint8_t *buf,
                                         uint16_t len);
HAL_StatusTypeDef LightSensor_I2cMemWrite(uint8_t i2c_addr, uint16_t reg_addr, uint16_t reg_addr_size, uint8_t *buf,
                                          uint16_t len);

static uint8_t chain_env_i2c_length_is_valid(uint8_t length)
{
    return (length > 0U && length <= CHAIN_ENV_I2C_MAX_LENGTH) ? 1U : 0U;
}

static uint8_t chain_env_i2c_addr_is_valid(uint8_t i2c_addr)
{
    return (i2c_addr <= 127U) ? 1U : 0U;
}

static uint8_t chain_env_i2c_make_hal_addr(uint8_t i2c_addr)
{
    return (uint8_t)(i2c_addr << 1);
}

static uint8_t chain_env_i2c_get_reg_addr_size(uint8_t reg_addr_length, uint16_t *reg_addr_size)
{
    if (reg_addr_size == NULL) {
        return 0U;
    }

    if (reg_addr_length == 1U) {
        *reg_addr_size = I2C_MEMADD_SIZE_8BIT;
        return 1U;
    }

    if (reg_addr_length == 2U) {
        *reg_addr_size = I2C_MEMADD_SIZE_16BIT;
        return 1U;
    }

    return 0U;
}
static void chain_env_put_u16_le(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFU);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFU);
}

static uint16_t chain_env_get_u16_le(const uint8_t *buffer)
{
    return (uint16_t)(((uint16_t)buffer[1] << 8) | buffer[0]);
}

static void chain_env_put_i32_le(uint8_t *buffer, int32_t value)
{
    uint32_t raw = (uint32_t)value;

    buffer[0] = (uint8_t)(raw & 0xFFU);
    buffer[1] = (uint8_t)((raw >> 8) & 0xFFU);
    buffer[2] = (uint8_t)((raw >> 16) & 0xFFU);
    buffer[3] = (uint8_t)((raw >> 24) & 0xFFU);
}

static int32_t chain_env_get_i32_le(const uint8_t *buffer)
{
    uint32_t raw =
        ((uint32_t)buffer[0]) | ((uint32_t)buffer[1] << 8) | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);

    return (int32_t)raw;
}

static uint16_t chain_env_pressure_pa_to_hpa_x10(uint32_t pressure_pa)
{
    uint32_t pressure_hpa_x10 = (pressure_pa + 5U) / 10U;

    return (pressure_hpa_x10 > UINT16_MAX) ? UINT16_MAX : (uint16_t)pressure_hpa_x10;
}

static double chain_env_pressure_hpa_x10_to_pa(uint16_t pressure_hpa_x10)
{
    return (double)pressure_hpa_x10 * 10.0;
}

static void chain_env_reply_status(uint8_t cmd, uint8_t status)
{
    chain_command_complete_return(cmd, &status, 1U);
}

static uint8_t chain_env_rate_is_valid(uint8_t rate)
{
    return (rate <= (uint8_t)SPA06_200HZ) ? 1U : 0U;
}

static uint8_t chain_env_is_spa06_addr(uint8_t i2c_addr)
{
    return ((i2c_addr == spa06_sensor.address) || (i2c_addr == SPA06_ADDR_DEFAULT) || (i2c_addr == SPA06_ADDR_ALT))
               ? 1U
               : 0U;
}

static uint8_t chain_env_i2c_write_touches_reg(uint16_t reg_addr, uint8_t write_len, uint8_t target_reg,
                                               uint8_t *offset)
{
    uint32_t start  = reg_addr;
    uint32_t target = target_reg;

    if ((offset == NULL) || (target < start) || (target >= (start + write_len))) {
        return 0U;
    }

    *offset = (uint8_t)(target - start);
    return 1U;
}

static uint8_t chain_env_i2c_write_touches_spa06_config(uint8_t i2c_addr, uint16_t reg_addr, uint8_t write_len)
{
    uint8_t offset = 0U;

    if (chain_env_is_spa06_addr(i2c_addr) == 0U) {
        return 0U;
    }

    return (chain_env_i2c_write_touches_reg(reg_addr, write_len, SPA06_REG_PRS_CFG, &offset) ||
            chain_env_i2c_write_touches_reg(reg_addr, write_len, SPA06_REG_TMP_CFG, &offset) ||
            chain_env_i2c_write_touches_reg(reg_addr, write_len, SPA06_REG_MEAS_CFG, &offset) ||
            chain_env_i2c_write_touches_reg(reg_addr, write_len, SPA06_REG_CFG_REG, &offset))
               ? 1U
               : 0U;
}

static uint8_t chain_env_calculate_sea_level_pressure(int32_t reference_altitude_cm, uint16_t *pressure_hpa_x10)
{
    uint64_t pressure_sum_pa = 0U;
    double measured_pressure_pa;
    double reference_altitude_m;
    double sea_level_pressure_pa;
    double sea_level_pressure_hpa_x10;

    if (pressure_hpa_x10 == NULL) {
        return 0U;
    }

    for (uint8_t sample = 0U; sample < CHAIN_ENV_CALIBRATE_SAMPLE_COUNT; sample++) {
        uint32_t pressure_pa = 0U;

        if (spa06_sensor_read_pressure(&pressure_pa) != 0U) {
            return 0U;
        }

        pressure_sum_pa += pressure_pa;
        LL_IWDG_ReloadCounter(IWDG);

        if (sample + 1U < CHAIN_ENV_CALIBRATE_SAMPLE_COUNT) {
            HAL_Delay(CHAIN_ENV_CALIBRATE_SAMPLE_INTERVAL_MS);
        }
    }

    measured_pressure_pa = (double)pressure_sum_pa / (double)CHAIN_ENV_CALIBRATE_SAMPLE_COUNT;
    reference_altitude_m = (double)reference_altitude_cm / 100.0;

    if (reference_altitude_m >= 44330.0) {
        return 0U;
    }

    sea_level_pressure_pa      = measured_pressure_pa / pow(1.0 - (reference_altitude_m / 44330.0), 5.255);
    sea_level_pressure_hpa_x10 = sea_level_pressure_pa / 10.0;

    if ((sea_level_pressure_hpa_x10 < (double)CHAIN_ENV_SEA_LEVEL_PRESS_MIN_HPA_X10) ||
        (sea_level_pressure_hpa_x10 > (double)CHAIN_ENV_SEA_LEVEL_PRESS_MAX_HPA_X10)) {
        return 0U;
    }

    *pressure_hpa_x10 = (uint16_t)(sea_level_pressure_hpa_x10 + 0.5);
    return 1U;
}

static uint8_t chain_env_update_sensor_params_from_i2c_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t write_len)
{
    if (chain_env_i2c_write_touches_spa06_config(i2c_addr, reg_addr, write_len) == 0U) {
        return 1U;
    }

    if (spa06_sensor_apply_config_from_registers() != 0U) {
        return 0U;
    }

    s_spa06_temp_rate  = (uint8_t)spa06_sensor.temperature_rate;
    s_spa06_press_rate = (uint8_t)spa06_sensor.pressure_rate;
    return 1U;
}

static uint8_t chain_env_i2c_write_need_readback(uint8_t i2c_addr, uint16_t reg_addr, uint8_t write_len)
{
    uint8_t offset = 0U;

    if (chain_env_is_spa06_addr(i2c_addr) == 0U) {
        return 0U;
    }

    return (chain_env_i2c_write_touches_reg(reg_addr, write_len, SPA06_REG_TMP_CFG, &offset) ||
            chain_env_i2c_write_touches_reg(reg_addr, write_len, SPA06_REG_PRS_CFG, &offset) ||
            chain_env_i2c_write_touches_reg(reg_addr, write_len, SPA06_REG_MEAS_CFG, &offset) ||
            chain_env_i2c_write_touches_reg(reg_addr, write_len, SPA06_REG_CFG_REG, &offset))
               ? 1U
               : 0U;
}

static uint8_t chain_env_i2c_readback_matches(uint8_t i2c_addr, uint16_t reg_addr, uint16_t reg_addr_size,
                                              uint8_t write_len, uint8_t *buffer)
{
    uint8_t readback[CHAIN_ENV_I2C_MAX_LENGTH] = {0};

    if ((buffer == NULL) || (LightSensor_I2cMemRead(chain_env_i2c_make_hal_addr(i2c_addr), reg_addr, reg_addr_size,
                                                    readback, write_len) != HAL_OK)) {
        return 0U;
    }

    for (uint8_t i = 0; i < write_len; i++) {
        uint8_t mask = 0xFFU;

        if ((reg_addr + i) == SPA06_REG_MEAS_CFG) {
            mask = 0x0FU;
        }

        if ((readback[i] & mask) != (buffer[i] & mask)) {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t chain_env_get_zone_i32(int32_t value, int32_t low, int32_t high)
{
    if (value > high) {
        return 1U;
    }

    if (value < low) {
        return 2U;
    }

    return 0U;
}

static uint8_t chain_env_get_zone_u16(uint16_t value, uint16_t low, uint16_t high)
{
    if (value > high) {
        return 1U;
    }

    if (value < low) {
        return 2U;
    }

    return 0U;
}

static uint8_t chain_env_zone_event_code(uint8_t old_zone, uint8_t new_zone, uint8_t *code)
{
    if ((code == NULL) || (old_zone == new_zone)) {
        return 0U;
    }

    if ((old_zone == 1U) && (new_zone == 0U)) {
        *code = 0U;
    } else if (new_zone == 1U) {
        *code = 1U;
    } else if (new_zone == 2U) {
        *code = 2U;
    } else if ((old_zone == 2U) && (new_zone == 0U)) {
        *code = 3U;
    } else {
        return 0U;
    }

    return 1U;
}

static void chain_env_set_alarm_rgb_state(uint8_t temp_zone, uint8_t humi_zone, uint8_t press_zone, uint8_t alti_zone)
{
    uint8_t alarm_mask = 0U;

    if (((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_TEMP_MASK) != 0U) && (temp_zone != 0U)) {
        alarm_mask |= RGB_ALARM_TEMP_MASK;
    }
    if (((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_HUMI_MASK) != 0U) && (humi_zone != 0U)) {
        alarm_mask |= RGB_ALARM_HUMI_MASK;
    }
    if (((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_PRESS_MASK) != 0U) && (press_zone != 0U)) {
        alarm_mask |= RGB_ALARM_PRESS_MASK;
    }
    if (((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_ALTI_MASK) != 0U) && (alti_zone != 0U)) {
        alarm_mask |= RGB_ALARM_ALTI_MASK;
    }

    rgb_set_alarm_overlay(alarm_mask);
}

HAL_StatusTypeDef LightSensor_I2cMemRead(uint8_t i2c_addr, uint16_t reg_addr, uint16_t reg_addr_size, uint8_t *buf,
                                         uint16_t len)
{
    if (buf == NULL || len == 0U) {
        return HAL_ERROR;
    }

    return HAL_I2C_Mem_Read(&hi2c2, i2c_addr, reg_addr, reg_addr_size, buf, len, LIGHT_SENSOR_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef LightSensor_I2cMemWrite(uint8_t i2c_addr, uint16_t reg_addr, uint16_t reg_addr_size, uint8_t *buf,
                                          uint16_t len)
{
    if (buf == NULL || len == 0U) {
        return HAL_ERROR;
    }

    return HAL_I2C_Mem_Write(&hi2c2, i2c_addr, reg_addr, reg_addr_size, buf, len, LIGHT_SENSOR_I2C_TIMEOUT_MS);
}

void chain_env_i2c_mem_read(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t read_len)
{
    uint8_t operation_result = OPERATION_FAIL;
    uint16_t reg_addr_size   = 0U;

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;

    if (chain_env_i2c_addr_is_valid(i2c_addr) && chain_env_i2c_length_is_valid(read_len) &&
        chain_env_i2c_get_reg_addr_size(reg_addr_length, &reg_addr_size) &&
        LightSensor_I2cMemRead(chain_env_i2c_make_hal_addr(i2c_addr), reg_addr, reg_addr_size, &s_ret_buf[1],
                               read_len) == HAL_OK) {
        s_ret_buf[0]   = OPERATION_SUCCESS;
        s_ret_buf_size = (uint8_t)(read_len + 1U);
    }

    chain_command_complete_return(CHAIN_ENV_I2C_MEM_READ, s_ret_buf, s_ret_buf_size);
}

void chain_env_i2c_mem_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t write_len,
                             uint8_t *buffer)
{
    uint8_t operation_result = OPERATION_FAIL;
    uint16_t reg_addr_size   = 0U;

    if (buffer != NULL && chain_env_i2c_addr_is_valid(i2c_addr) && chain_env_i2c_length_is_valid(write_len) &&
        chain_env_i2c_get_reg_addr_size(reg_addr_length, &reg_addr_size) &&
        LightSensor_I2cMemWrite(chain_env_i2c_make_hal_addr(i2c_addr), reg_addr, reg_addr_size, buffer, write_len) ==
            HAL_OK) {
        if ((chain_env_i2c_write_need_readback(i2c_addr, reg_addr, write_len) == 0U) ||
            chain_env_i2c_readback_matches(i2c_addr, reg_addr, reg_addr_size, write_len, buffer)) {
            if (chain_env_update_sensor_params_from_i2c_write(i2c_addr, reg_addr, write_len) != 0U) {
                operation_result = OPERATION_SUCCESS;
            }
        }
    }

    chain_env_reply_status(CHAIN_ENV_I2C_MEM_WRITE, operation_result);
}

void chain_env_get_aht20_data_handle(void)
{
    uint8_t data[3] = {0};

    chain_env_put_u16_le(data, (uint16_t)env_temperature_c_x100);
    data[2] = env_humidity_percent;
    chain_command_complete_return(CHAIN_ENV_GET_AHT20_DATA, data, sizeof(data));
}

void chain_env_get_spa06_data_handle(void)
{
    uint8_t data[8]           = {0};
    uint16_t pressure_hpa_x10 = chain_env_pressure_pa_to_hpa_x10(spa06_pressure_pa);

    chain_env_put_u16_le(&data[0], (uint16_t)spa06_temperature_c_x100);
    chain_env_put_u16_le(&data[2], pressure_hpa_x10);
    chain_env_put_i32_le(&data[4], spa06_altitude_cm);
    chain_command_complete_return(CHAIN_ENV_GET_SPA06_DATA, data, sizeof(data));
}

void chain_env_set_spa06_temperature_rate_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t status = OPERATION_FAIL;

    if ((buffer != NULL) && (size == CHAIN_ENV_RATE_PAYLOAD_SIZE) && chain_env_rate_is_valid(buffer[0]) &&
        (spa06_sensor_set_temperature_config((spa06_measure_rate_t)buffer[0], spa06_sensor.temperature_oversample) ==
         0U)) {
        s_spa06_temp_rate = buffer[0];
        status            = OPERATION_SUCCESS;
    }

    chain_env_reply_status(CHAIN_ENV_SET_SPA06_TEMPERATURE_RATE, status);
}

void chain_env_get_spa06_temperature_rate_handle(void)
{
    chain_command_complete_return(CHAIN_ENV_GET_SPA06_TEMPERATURE_RATE, &s_spa06_temp_rate, 1U);
}

void chain_env_set_spa06_pressure_rate_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t status = OPERATION_FAIL;

    if ((buffer != NULL) && (size == CHAIN_ENV_RATE_PAYLOAD_SIZE) && chain_env_rate_is_valid(buffer[0]) &&
        (spa06_sensor_set_pressure_config((spa06_measure_rate_t)buffer[0], spa06_sensor.pressure_oversample) == 0U)) {
        s_spa06_press_rate = buffer[0];
        status             = OPERATION_SUCCESS;
    }

    chain_env_reply_status(CHAIN_ENV_SET_SPA06_PRESSURE_RATE, status);
}

void chain_env_get_spa06_pressure_rate_handle(void)
{
    chain_command_complete_return(CHAIN_ENV_GET_SPA06_PRESSURE_RATE, &s_spa06_press_rate, 1U);
}

void chain_env_set_spa06_sea_level_pressure_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t status            = OPERATION_FAIL;
    uint16_t pressure_hpa_x10 = 0U;
    uint8_t save_to_flash     = 0U;

    if ((buffer != NULL) && (size == CHAIN_ENV_SEA_LEVEL_PRESS_PAYLOAD_SIZE)) {
        pressure_hpa_x10 = chain_env_get_u16_le(buffer);
        save_to_flash    = buffer[2];
        if ((pressure_hpa_x10 >= CHAIN_ENV_SEA_LEVEL_PRESS_MIN_HPA_X10) &&
            (pressure_hpa_x10 <= CHAIN_ENV_SEA_LEVEL_PRESS_MAX_HPA_X10) && (save_to_flash <= 1U)) {
            if ((save_to_flash == 0U) || (pressure_hpa_x10 == get_spa06_sea_level_pressure()) ||
                set_spa06_sea_level_pressure(pressure_hpa_x10)) {
                s_spa06_sea_level_press_hpa_x10 = pressure_hpa_x10;
                s_spa06_sea_level_pressure_pa   = chain_env_pressure_hpa_x10_to_pa(pressure_hpa_x10);
                spa06_sensor_set_sea_level_pressure(s_spa06_sea_level_pressure_pa);
                s_alti_zone = 0U;
                status      = OPERATION_SUCCESS;
            }
        }
    }

    chain_env_reply_status(CHAIN_ENV_SET_SPA06_SEA_LEVEL_PRESSURE, status);
}

void chain_env_restore_spa06_sea_level_pressure(void)
{
    uint16_t pressure_hpa_x10 = get_spa06_sea_level_pressure();

    if ((pressure_hpa_x10 >= CHAIN_ENV_SEA_LEVEL_PRESS_MIN_HPA_X10) &&
        (pressure_hpa_x10 <= CHAIN_ENV_SEA_LEVEL_PRESS_MAX_HPA_X10)) {
        s_spa06_sea_level_press_hpa_x10 = pressure_hpa_x10;
        s_spa06_sea_level_pressure_pa   = chain_env_pressure_hpa_x10_to_pa(pressure_hpa_x10);
        spa06_sensor_set_sea_level_pressure(s_spa06_sea_level_pressure_pa);
    }
}

void chain_env_get_spa06_sea_level_pressure_handle(void)
{
    uint8_t data[2] = {0};

    chain_env_put_u16_le(data, s_spa06_sea_level_press_hpa_x10);
    chain_command_complete_return(CHAIN_ENV_GET_SPA06_SEA_LEVEL_PRESSURE, data, sizeof(data));
}

void chain_env_calibrate_spa06_sea_level_pressure_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t status                = OPERATION_FAIL;
    uint8_t save_to_flash         = 0U;
    int32_t reference_altitude_cm = 0;
    uint16_t pressure_hpa_x10     = 0U;

    if ((buffer != NULL) && (size == CHAIN_ENV_CALIBRATE_SEA_LEVEL_PRESS_PAYLOAD_SIZE)) {
        reference_altitude_cm = chain_env_get_i32_le(buffer);
        save_to_flash         = buffer[4];

        if ((save_to_flash <= 1U) &&
            chain_env_calculate_sea_level_pressure(reference_altitude_cm, &pressure_hpa_x10) != 0U &&
            ((save_to_flash == 0U) || (pressure_hpa_x10 == get_spa06_sea_level_pressure()) ||
             set_spa06_sea_level_pressure(pressure_hpa_x10))) {
            s_spa06_sea_level_press_hpa_x10 = pressure_hpa_x10;
            s_spa06_sea_level_pressure_pa   = chain_env_pressure_hpa_x10_to_pa(pressure_hpa_x10);
            spa06_sensor_set_sea_level_pressure(s_spa06_sea_level_pressure_pa);
            s_alti_zone = 0U;
            status      = OPERATION_SUCCESS;
        }
    }

    chain_env_reply_status(CHAIN_ENV_CALIBRATE_SPA06_SEA_LEVEL_PRESSURE, status);
}

void chain_env_set_alarm_mode_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t status = OPERATION_FAIL;

    if ((buffer != NULL) && (size == CHAIN_ENV_MODE_PAYLOAD_SIZE) && ((buffer[0] & (uint8_t)~0x0FU) == 0U)) {
        s_env_alarm_mode = buffer[0];
        chain_env_set_alarm_rgb_state(s_temp_zone, s_humi_zone, s_press_zone, s_alti_zone);
        status = OPERATION_SUCCESS;
    }

    chain_env_reply_status(CHAIN_ENV_SET_ALARM_MODE, status);
}

void chain_env_get_alarm_mode_handle(void)
{
    chain_command_complete_return(CHAIN_ENV_GET_ALARM_MODE, &s_env_alarm_mode, 1U);
}

void chain_env_set_alarm_temperature_threshold_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t status = OPERATION_FAIL;

    if ((buffer != NULL) && (size == CHAIN_ENV_TEMP_THRESHOLD_PAYLOAD_SIZE) &&
        ((buffer[0] == CHAIN_ENV_TEMP_DEVICE_AHT20) || (buffer[0] == CHAIN_ENV_TEMP_DEVICE_SPA06)) &&
        (buffer[1] < buffer[2])) {
        s_alarm_temp_device = buffer[0];
        s_alarm_temp_low    = buffer[1];
        s_alarm_temp_high   = buffer[2];
        s_temp_zone         = 0U;
        status              = OPERATION_SUCCESS;
    }

    chain_env_reply_status(CHAIN_ENV_SET_ALARM_TEMP_THRESHOLD, status);
}

void chain_env_get_alarm_temperature_threshold_handle(void)
{
    uint8_t data[3] = {s_alarm_temp_device, s_alarm_temp_low, s_alarm_temp_high};

    chain_command_complete_return(CHAIN_ENV_GET_ALARM_TEMP_THRESHOLD, data, sizeof(data));
}

void chain_env_set_alarm_humidity_threshold_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t status = OPERATION_FAIL;

    if ((buffer != NULL) && (size == CHAIN_ENV_HUMI_THRESHOLD_PAYLOAD_SIZE) && (buffer[0] <= 100U) &&
        (buffer[1] <= 100U) && (buffer[0] < buffer[1])) {
        s_alarm_humi_low  = buffer[0];
        s_alarm_humi_high = buffer[1];
        s_humi_zone       = 0U;
        status            = OPERATION_SUCCESS;
    }

    chain_env_reply_status(CHAIN_ENV_SET_ALARM_HUMIDITY_THRESHOLD, status);
}

void chain_env_get_alarm_humidity_threshold_handle(void)
{
    uint8_t data[2] = {s_alarm_humi_low, s_alarm_humi_high};

    chain_command_complete_return(CHAIN_ENV_GET_ALARM_HUMIDITY_THRESHOLD, data, sizeof(data));
}

void chain_env_set_alarm_pressure_threshold_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t status = OPERATION_FAIL;
    uint16_t low   = 0U;
    uint16_t high  = 0U;

    if ((buffer != NULL) && (size == CHAIN_ENV_PRESS_THRESHOLD_PAYLOAD_SIZE)) {
        low  = chain_env_get_u16_le(&buffer[0]);
        high = chain_env_get_u16_le(&buffer[2]);
        if ((low >= CHAIN_ENV_ALARM_PRESS_MIN_HPA_X10) && (high <= CHAIN_ENV_ALARM_PRESS_MAX_HPA_X10) && (low < high)) {
            s_alarm_press_low_hpa_x10  = low;
            s_alarm_press_high_hpa_x10 = high;
            s_press_zone               = 0U;
            status                     = OPERATION_SUCCESS;
        }
    }

    chain_env_reply_status(CHAIN_ENV_SET_ALARM_PRESSURE_THRESHOLD, status);
}

void chain_env_get_alarm_pressure_threshold_handle(void)
{
    uint8_t data[4] = {0};

    chain_env_put_u16_le(&data[0], s_alarm_press_low_hpa_x10);
    chain_env_put_u16_le(&data[2], s_alarm_press_high_hpa_x10);
    chain_command_complete_return(CHAIN_ENV_GET_ALARM_PRESSURE_THRESHOLD, data, sizeof(data));
}

void chain_env_set_alarm_altitude_threshold_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t status = OPERATION_FAIL;
    int32_t low    = 0;
    int32_t high   = 0;

    if ((buffer != NULL) && (size == CHAIN_ENV_ALTI_THRESHOLD_PAYLOAD_SIZE)) {
        low  = chain_env_get_i32_le(&buffer[0]);
        high = chain_env_get_i32_le(&buffer[4]);
        if (low < high) {
            s_alarm_alti_low_cm  = low;
            s_alarm_alti_high_cm = high;
            s_alti_zone          = 0U;
            status               = OPERATION_SUCCESS;
        }
    }

    chain_env_reply_status(CHAIN_ENV_SET_ALARM_ALTITUDE_THRESHOLD, status);
}

void chain_env_get_alarm_altitude_threshold_handle(void)
{
    uint8_t data[8] = {0};

    chain_env_put_i32_le(&data[0], s_alarm_alti_low_cm);
    chain_env_put_i32_le(&data[4], s_alarm_alti_high_cm);
    chain_command_complete_return(CHAIN_ENV_GET_ALARM_ALTITUDE_THRESHOLD, data, sizeof(data));
}

void chain_env_alarm_update(void)
{
    uint8_t status         = s_alarm_event_status;
    uint8_t code           = 0U;
    uint8_t event_detected = 0U;
    int32_t temp_c_x100 =
        (s_alarm_temp_device == CHAIN_ENV_TEMP_DEVICE_SPA06) ? spa06_temperature_c_x100 : env_temperature_c_x100;
    uint8_t new_temp_zone =
        chain_env_get_zone_i32(temp_c_x100, (int32_t)s_alarm_temp_low * 100, (int32_t)s_alarm_temp_high * 100);
    uint8_t new_humi_zone     = chain_env_get_zone_i32(env_humidity_percent, s_alarm_humi_low, s_alarm_humi_high);
    uint16_t pressure_hpa_x10 = chain_env_pressure_pa_to_hpa_x10(spa06_pressure_pa);
    uint8_t new_press_zone =
        chain_env_get_zone_u16(pressure_hpa_x10, s_alarm_press_low_hpa_x10, s_alarm_press_high_hpa_x10);
    uint8_t new_alti_zone = chain_env_get_zone_i32(spa06_altitude_cm, s_alarm_alti_low_cm, s_alarm_alti_high_cm);

    if ((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_TEMP_MASK) == 0U) {
        s_temp_zone = new_temp_zone;
    }
    if ((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_HUMI_MASK) == 0U) {
        s_humi_zone = new_humi_zone;
    }
    if ((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_PRESS_MASK) == 0U) {
        s_press_zone = new_press_zone;
    }
    if ((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_ALTI_MASK) == 0U) {
        s_alti_zone = new_alti_zone;
    }

    if (((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_TEMP_MASK) != 0U) &&
        chain_env_zone_event_code(s_temp_zone, new_temp_zone, &code)) {
        status         = (uint8_t)((status & 0xFCU) | code);
        event_detected = 1U;
    }

    if (((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_HUMI_MASK) != 0U) &&
        chain_env_zone_event_code(s_humi_zone, new_humi_zone, &code)) {
        status         = (uint8_t)((status & 0xF3U) | (uint8_t)(code << 2));
        event_detected = 1U;
    }

    if (((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_PRESS_MASK) != 0U) &&
        chain_env_zone_event_code(s_press_zone, new_press_zone, &code)) {
        status         = (uint8_t)((status & 0xCFU) | (uint8_t)(code << 4));
        event_detected = 1U;
    }

    if (((s_env_alarm_mode & CHAIN_ENV_ALARM_MODE_ALTI_MASK) != 0U) &&
        chain_env_zone_event_code(s_alti_zone, new_alti_zone, &code)) {
        status         = (uint8_t)((status & 0x3FU) | (uint8_t)(code << 6));
        event_detected = 1U;
    }

    chain_env_set_alarm_rgb_state(new_temp_zone, new_humi_zone, new_press_zone, new_alti_zone);

    s_temp_zone  = new_temp_zone;
    s_humi_zone  = new_humi_zone;
    s_press_zone = new_press_zone;
    s_alti_zone  = new_alti_zone;

    if (event_detected != 0U) {
        uint8_t data[2] = {status, CHAIN_ENV_REPORT_TYPE};

        s_alarm_event_status = status;
        chain_command_complete_return(CHAIN_ENV_ALARM_EVENT, data, sizeof(data));
    }
}
