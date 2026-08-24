#include "light_function.h"
#include "RGB.h"

#define CHAIN_DLIGHT_DEFAULT_LOW_THRESHOLD    (100U)
#define CHAIN_DLIGHT_DEFAULT_HIGH_THRESHOLD   (2047U)
#define CHAIN_DLIGHT_DEFAULT_GAIN             LIGHT_SENSOR_ALS_GAIN_3X
#define CHAIN_DLIGHT_DEFAULT_RESOLUTION       (2U)
#define CHAIN_DLIGHT_DEFAULT_RATE             (3U)
#define CHAIN_DLIGHT_DEFAULT_INTERRUPT_ENABLE (1U)
#define CHAIN_DLIGHT_DEFAULT_REPORT_ENABLE    (1U)

#define CHAIN_DLIGHT_I2C_MAX_LENGTH (64U)

#define CHAIN_DLIGHT_STATUS_LOW_TRIGGER  (0x00U)
#define CHAIN_DLIGHT_STATUS_HIGH_TRIGGER (0x01U)
#define CHAIN_DLIGHT_STATUS_LOW_RELEASE  (0x02U)
#define CHAIN_DLIGHT_STATUS_HIGH_RELEASE (0x03U)

#define CHAIN_DLIGHT_FLASH_MAGIC        (0xA6U)
#define CHAIN_DLIGHT_FLASH_OFFSET_MAGIC (2U)
#define CHAIN_DLIGHT_FLASH_OFFSET_LOW   (8U)
#define CHAIN_DLIGHT_FLASH_OFFSET_HIGH  (12U)

typedef struct {
    uint32_t low_threshold;
    uint32_t high_threshold;
    uint8_t gain;
    uint8_t resolution;
    uint8_t rate;
} chain_dlight_config_t;

static uint8_t s_ret_buf[CHAIN_DLIGHT_I2C_MAX_LENGTH + 1U] = {0};
static uint8_t s_ret_buf_size                              = 0;
static uint8_t s_report_enable                             = CHAIN_DLIGHT_DEFAULT_REPORT_ENABLE;
static uint8_t s_env_has_last_lux                          = 0U;
static uint32_t s_env_last_lux                             = 0U;
static uint32_t s_env_current_lux                          = 0U;
static uint8_t s_interrupt_enable                          = 0U;
static uint32_t s_low_threshold_lux                        = CHAIN_DLIGHT_DEFAULT_LOW_THRESHOLD;
static uint32_t s_high_threshold_lux                       = CHAIN_DLIGHT_DEFAULT_HIGH_THRESHOLD;
static uint8_t s_measure_gain                              = CHAIN_DLIGHT_DEFAULT_GAIN;
static uint8_t s_measure_resolution                        = CHAIN_DLIGHT_DEFAULT_RESOLUTION;
static uint8_t s_measure_rate                              = CHAIN_DLIGHT_DEFAULT_RATE;

static void chain_dlight_reset_lux_history(void);

static const uint8_t s_rate_reg_map[] = {
    LIGHT_SENSOR_ALS_RATE_25MS,  LIGHT_SENSOR_ALS_RATE_50MS,   LIGHT_SENSOR_ALS_RATE_100MS,
    LIGHT_SENSOR_ALS_RATE_500MS, LIGHT_SENSOR_ALS_RATE_1000MS, LIGHT_SENSOR_ALS_RATE_2000MS,
};

static void chain_dlight_reply_status(uint8_t cmd, uint8_t operation_result)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(cmd, s_ret_buf, s_ret_buf_size);
}

static void chain_dlight_put_uint32_le(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFU);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFU);
    buffer[2] = (uint8_t)((value >> 16) & 0xFFU);
    buffer[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static uint32_t chain_dlight_make_uint32_le(const uint8_t *buffer)
{
    return ((uint32_t)buffer[0]) | ((uint32_t)buffer[1] << 8) | ((uint32_t)buffer[2] << 16) |
           ((uint32_t)buffer[3] << 24);
}

static uint8_t chain_dlight_i2c_length_is_valid(uint8_t length)
{
    return (length > 0U && length <= CHAIN_DLIGHT_I2C_MAX_LENGTH) ? 1U : 0U;
}

static uint8_t chain_dlight_i2c_addr_is_valid(uint8_t i2c_addr)
{
    return (i2c_addr >= 0x08 && i2c_addr <= 0x77) ? 1U : 0U;
}

static uint8_t chain_dlight_i2c_make_hal_addr(uint8_t i2c_addr)
{
    return (uint8_t)(i2c_addr << 1);
}

static uint8_t chain_dlight_i2c_get_reg_addr_size(uint8_t reg_addr_length, uint16_t *reg_addr_size)
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

static uint8_t chain_dlight_lux_is_valid(uint32_t lux)
{
    return (lux <= LIGHT_SENSOR_LUX_MAX) ? 1U : 0U;
}

static uint8_t chain_dlight_gain_is_valid(uint8_t gain)
{
    return (gain <= LIGHT_SENSOR_ALS_GAIN_18X) ? 1U : 0U;
}

static uint8_t chain_dlight_resolution_to_reg(uint8_t resolution, uint8_t *reg)
{
    if (resolution > 4U || reg == NULL) {
        return 0U;
    }

    *reg = (uint8_t)(resolution << 4);

    return 1U;
}

static uint8_t chain_dlight_reg_to_resolution(uint8_t reg, uint8_t *resolution)
{
    uint8_t value = (reg & LIGHT_SENSOR_ALS_RES_MASK) >> 4;

    if (value > 4U || resolution == NULL) {
        return 0U;
    }

    *resolution = value;

    return 1U;
}

static uint8_t chain_dlight_rate_to_reg(uint8_t rate, uint8_t *reg)
{
    if (rate >= (sizeof(s_rate_reg_map) / sizeof(s_rate_reg_map[0])) || reg == NULL) {
        return 0U;
    }

    *reg = s_rate_reg_map[rate];

    return 1U;
}

static uint8_t chain_dlight_reg_to_rate(uint8_t reg, uint8_t *rate)
{
    if (rate == NULL) {
        return 0U;
    }

    for (uint8_t i = 0; i < (sizeof(s_rate_reg_map) / sizeof(s_rate_reg_map[0])); i++) {
        if ((reg & LIGHT_SENSOR_ALS_RATE_MASK) == s_rate_reg_map[i]) {
            *rate = i;
            return 1U;
        }
    }

    return 0U;
}

static uint8_t chain_dlight_gain_to_value(uint8_t gain, uint8_t *gain_value)
{
    if (gain_value == NULL) {
        return 0U;
    }

    switch (gain & LIGHT_SENSOR_ALS_GAIN_MASK) {
        case LIGHT_SENSOR_ALS_GAIN_1X:
            *gain_value = LIGHT_SENSOR_GAIN_VALUE_1X;
            return 1U;
        case LIGHT_SENSOR_ALS_GAIN_3X:
            *gain_value = LIGHT_SENSOR_GAIN_VALUE_3X;
            return 1U;
        case LIGHT_SENSOR_ALS_GAIN_6X:
            *gain_value = LIGHT_SENSOR_GAIN_VALUE_6X;
            return 1U;
        case LIGHT_SENSOR_ALS_GAIN_9X:
            *gain_value = LIGHT_SENSOR_GAIN_VALUE_9X;
            return 1U;
        case LIGHT_SENSOR_ALS_GAIN_18X:
            *gain_value = LIGHT_SENSOR_GAIN_VALUE_18X;
            return 1U;
        default:
            *gain_value = 0U;
            return 0U;
    }
}

static uint8_t chain_dlight_resolution_to_int_factor(uint8_t resolution, float *int_factor)
{
    if (int_factor == NULL) {
        return 0U;
    }

    switch (resolution & LIGHT_SENSOR_ALS_RES_MASK) {
        case LIGHT_SENSOR_ALS_RES_16BIT:
            *int_factor = LIGHT_SENSOR_INT_FACTOR_25MS;
            return 1U;
        case LIGHT_SENSOR_ALS_RES_17BIT:
            *int_factor = LIGHT_SENSOR_INT_FACTOR_50MS;
            return 1U;
        case LIGHT_SENSOR_ALS_RES_18BIT:
            *int_factor = LIGHT_SENSOR_INT_FACTOR_100MS;
            return 1U;
        case LIGHT_SENSOR_ALS_RES_19BIT:
            *int_factor = LIGHT_SENSOR_INT_FACTOR_200MS;
            return 1U;
        case LIGHT_SENSOR_ALS_RES_20BIT:
            *int_factor = LIGHT_SENSOR_INT_FACTOR_400MS;
            return 1U;
        default:
            *int_factor = 0.0f;
            return 0U;
    }
}

static const uint16_t s_resolution_conversion_time_ms[] = {400U, 200U, 100U, 50U, 25U};
static const uint16_t s_rate_time_ms[]                  = {25U, 50U, 100U, 500U, 1000U, 2000U};

static uint16_t chain_dlight_resolution_to_conversion_time_ms(uint8_t resolution)
{
    if (resolution >= (sizeof(s_resolution_conversion_time_ms) / sizeof(s_resolution_conversion_time_ms[0]))) {
        return 0U;
    }

    return s_resolution_conversion_time_ms[resolution];
}

static uint16_t chain_dlight_rate_to_measurement_time_ms(uint8_t rate)
{
    if (rate >= (sizeof(s_rate_time_ms) / sizeof(s_rate_time_ms[0]))) {
        return 0U;
    }

    return s_rate_time_ms[rate];
}

static HAL_StatusTypeDef chain_dlight_lux_threshold_to_raw(uint32_t lux, uint32_t *raw_threshold)
{
    uint8_t gain       = 0U;
    uint8_t gain_value = 0U;
    uint8_t resolution = 0U;
    float int_factor   = 0.0f;
    float raw_value    = 0.0f;

    if (raw_threshold == NULL) {
        return HAL_ERROR;
    }

    if (!chain_dlight_lux_is_valid(lux)) {
        return HAL_ERROR;
    }

    if (LightSensor_GetAlsGain(&gain) != HAL_OK || LightSensor_GetAlsResolution(&resolution) != HAL_OK ||
        !chain_dlight_gain_to_value(gain, &gain_value) ||
        !chain_dlight_resolution_to_int_factor(resolution, &int_factor)) {
        return HAL_ERROR;
    }

    raw_value = ((float)lux * (float)gain_value * int_factor / 0.6f) + 0.5f;
    if (raw_value > 1048575.0f) {
        *raw_threshold = 0xFFFFFU;
    } else {
        *raw_threshold = (uint32_t)raw_value;
    }

    return HAL_OK;
}

static uint32_t chain_dlight_raw_threshold_to_lux(uint32_t raw_threshold, uint8_t gain, uint8_t resolution)
{
    uint8_t gain_value = 0U;
    float int_factor   = 0.0f;
    float lux          = 0.0f;

    if (!chain_dlight_gain_to_value(gain, &gain_value) ||
        !chain_dlight_resolution_to_int_factor(resolution, &int_factor) || int_factor <= 0.0f) {
        return 0U;
    }

    lux = (0.6f * (float)raw_threshold / ((float)gain_value * int_factor)) + 0.5f;
    if (lux > (float)LIGHT_SENSOR_LUX_MAX) {
        return LIGHT_SENSOR_LUX_MAX;
    }

    return (uint32_t)lux;
}

static void chain_dlight_refresh_sensor_params_from_regs(void)
{
    uint8_t gain                = s_measure_gain;
    uint8_t resolution_reg      = 0U;
    uint8_t resolution          = s_measure_resolution;
    uint8_t rate_reg            = 0U;
    uint8_t rate                = s_measure_rate;
    uint8_t interrupt_enable    = s_interrupt_enable;
    uint32_t low_threshold_raw  = 0U;
    uint32_t high_threshold_raw = 0U;

    if (LightSensor_GetAlsGain(&gain) == HAL_OK && chain_dlight_gain_is_valid(gain)) {
        s_measure_gain = gain;
    } else {
        gain = s_measure_gain;
    }

    if (LightSensor_GetAlsResolution(&resolution_reg) == HAL_OK &&
        chain_dlight_reg_to_resolution(resolution_reg, &resolution)) {
        s_measure_resolution = resolution;
    } else {
        resolution = s_measure_resolution;
        (void)chain_dlight_resolution_to_reg(resolution, &resolution_reg);
    }

    if (LightSensor_GetAlsMeasurementRate(&rate_reg) == HAL_OK && chain_dlight_reg_to_rate(rate_reg, &rate)) {
        s_measure_rate = rate;
    }

    if (LightSensor_GetAlsInterruptConfig(&interrupt_enable, NULL) == HAL_OK) {
        s_interrupt_enable = interrupt_enable;
    }

    if (LightSensor_GetAlsThresholds(&low_threshold_raw, &high_threshold_raw) == HAL_OK) {
        s_low_threshold_lux  = chain_dlight_raw_threshold_to_lux(low_threshold_raw, gain, resolution_reg);
        s_high_threshold_lux = chain_dlight_raw_threshold_to_lux(high_threshold_raw, gain, resolution_reg);
    }

    chain_dlight_reset_lux_history();
}

static HAL_StatusTypeDef chain_dlight_set_lux_thresholds(uint32_t low_threshold_lux, uint32_t high_threshold_lux)
{
    uint32_t low_threshold_raw  = 0U;
    uint32_t high_threshold_raw = 0U;

    if (low_threshold_lux >= high_threshold_lux) {
        return HAL_ERROR;
    }

    if (chain_dlight_lux_threshold_to_raw(low_threshold_lux, &low_threshold_raw) != HAL_OK ||
        chain_dlight_lux_threshold_to_raw(high_threshold_lux, &high_threshold_raw) != HAL_OK) {
        return HAL_ERROR;
    }
    if (low_threshold_raw >= high_threshold_raw) {
        if (low_threshold_raw < 0xFFFFFU) {
            high_threshold_raw = low_threshold_raw + 1U;
        } else if (high_threshold_raw > 0U) {
            low_threshold_raw = high_threshold_raw - 1U;
        } else {
            return HAL_ERROR;
        }
    }

    if (LightSensor_SetAlsThresholds(low_threshold_raw, high_threshold_raw) != HAL_OK) {
        return HAL_ERROR;
    }

    s_low_threshold_lux  = low_threshold_lux;
    s_high_threshold_lux = high_threshold_lux;

    return HAL_OK;
}

typedef HAL_StatusTypeDef (*chain_dlight_write_u8_fn_t)(uint8_t value);

static HAL_StatusTypeDef chain_dlight_write_u8_with_als_restart(chain_dlight_write_u8_fn_t write_fn, uint8_t value)
{
    HAL_StatusTypeDef ret         = HAL_ERROR;
    HAL_StatusTypeDef restart_ret = HAL_ERROR;

    if (write_fn == NULL) {
        return HAL_ERROR;
    }

    ret = LightSensor_SetAlsEnable(0U);
    if (ret == HAL_OK) {
        ret = write_fn(value);
        if (ret == HAL_OK) {
            ret = chain_dlight_set_lux_thresholds(s_low_threshold_lux, s_high_threshold_lux);
        }
    }

    restart_ret = LightSensor_SetAlsEnable(1U);

    return (ret == HAL_OK) ? restart_ret : ret;
}

static HAL_StatusTypeDef chain_dlight_set_thresholds_with_als_restart(uint32_t low_threshold, uint32_t high_threshold)
{
    HAL_StatusTypeDef ret         = LightSensor_SetAlsEnable(0U);
    HAL_StatusTypeDef restart_ret = HAL_ERROR;

    if (ret == HAL_OK) {
        ret = chain_dlight_set_lux_thresholds(low_threshold, high_threshold);
    }

    restart_ret = LightSensor_SetAlsEnable(1U);

    return (ret == HAL_OK) ? restart_ret : ret;
}

static void chain_dlight_reset_lux_history(void)
{
    s_env_has_last_lux = 0U;
    s_env_last_lux     = 0U;
}

static void chain_dlight_load_config(chain_dlight_config_t *config)
{
    uint8_t magic = flash_config_get_byte(CHAIN_DLIGHT_FLASH_OFFSET_MAGIC);

    config->low_threshold  = CHAIN_DLIGHT_DEFAULT_LOW_THRESHOLD;
    config->high_threshold = CHAIN_DLIGHT_DEFAULT_HIGH_THRESHOLD;
    config->gain           = CHAIN_DLIGHT_DEFAULT_GAIN;
    config->resolution     = CHAIN_DLIGHT_DEFAULT_RESOLUTION;
    config->rate           = CHAIN_DLIGHT_DEFAULT_RATE;

    if (magic != CHAIN_DLIGHT_FLASH_MAGIC) {
        return;
    }

    {
        uint8_t low_threshold_buf[4]  = {0};
        uint8_t high_threshold_buf[4] = {0};
        uint32_t low_threshold        = 0U;
        uint32_t high_threshold       = 0U;

        flash_config_read_bytes(CHAIN_DLIGHT_FLASH_OFFSET_LOW, low_threshold_buf, sizeof(low_threshold_buf));
        flash_config_read_bytes(CHAIN_DLIGHT_FLASH_OFFSET_HIGH, high_threshold_buf, sizeof(high_threshold_buf));

        low_threshold  = chain_dlight_make_uint32_le(low_threshold_buf);
        high_threshold = chain_dlight_make_uint32_le(high_threshold_buf);

        if (low_threshold < high_threshold && chain_dlight_lux_is_valid(low_threshold) &&
            chain_dlight_lux_is_valid(high_threshold)) {
            config->low_threshold  = low_threshold;
            config->high_threshold = high_threshold;
        }
    }
}

static uint8_t chain_dlight_read_lux_sample(uint32_t *lux_integer)
{
    uint32_t lux_value = 0U;

    if (lux_integer == NULL || LightSensor_GetLux(&lux_value) != HAL_OK) {
        return 0U;
    }

    s_env_current_lux = lux_value;
    *lux_integer      = lux_value;

    return 1U;
}

static void chain_dlight_reset_or_seed_lux_history(void)
{
    chain_dlight_reset_lux_history();
}

static void chain_dlight_send_threshold_event(uint8_t status)
{
    uint8_t return_buf[2] = {0};

    return_buf[0] = (uint8_t)(status & 0x03U);
    return_buf[1] = (uint8_t)(CHAIN_DLIGHT_REPORT_TYPE & 0xFFU);
    chain_command_complete_return(CHAIN_DLIGHT_INTERRUPT_REPORT, return_buf, sizeof(return_buf));
}

static void chain_dlight_process_lux_sample(uint32_t lux_integer)
{
    uint32_t last_lux       = s_env_last_lux;
    uint32_t low_threshold  = s_low_threshold_lux;
    uint32_t high_threshold = s_high_threshold_lux;

    s_env_last_lux = lux_integer;

    if (!s_interrupt_enable || !s_report_enable) {
        rgb_set_env_threshold_overlay(0U);
        s_env_has_last_lux = 1U;
        return;
    }

    if (lux_integer > high_threshold) {
        rgb_set_env_threshold_overlay(1U);
    } else if (lux_integer < low_threshold) {
        rgb_set_env_threshold_overlay(2U);
    } else {
        rgb_set_env_threshold_overlay(0U);
    }

    if (!s_env_has_last_lux) {
        s_env_has_last_lux = 1U;
        if (lux_integer < low_threshold) {
            chain_dlight_send_threshold_event(CHAIN_DLIGHT_STATUS_LOW_TRIGGER);
        } else if (lux_integer > high_threshold) {
            chain_dlight_send_threshold_event(CHAIN_DLIGHT_STATUS_HIGH_TRIGGER);
        } else {
            return;
        }
    } else {
        if (last_lux < low_threshold && lux_integer >= low_threshold) {
            chain_dlight_send_threshold_event(CHAIN_DLIGHT_STATUS_LOW_RELEASE);
        }
        if (last_lux > high_threshold && lux_integer <= high_threshold) {
            chain_dlight_send_threshold_event(CHAIN_DLIGHT_STATUS_HIGH_RELEASE);
        }
        if (last_lux >= low_threshold && lux_integer < low_threshold) {
            chain_dlight_send_threshold_event(CHAIN_DLIGHT_STATUS_LOW_TRIGGER);
        }
        if (last_lux <= high_threshold && lux_integer > high_threshold) {
            chain_dlight_send_threshold_event(CHAIN_DLIGHT_STATUS_HIGH_TRIGGER);
        }
    }
}

void chain_dlight_init(void)
{
    chain_dlight_config_t config;
    uint8_t resolution_reg = 0;
    uint8_t rate_reg       = 0;

    chain_dlight_load_config(&config);
    (void)chain_dlight_resolution_to_reg(config.resolution, &resolution_reg);
    (void)chain_dlight_rate_to_reg(config.rate, &rate_reg);

    LightSensor_SetAlsEnable(0U);
    LightSensor_SetAlsGain(config.gain);
    LightSensor_SetAlsResolution(resolution_reg);
    LightSensor_SetAlsMeasurementRate(rate_reg);
    (void)chain_dlight_set_lux_thresholds(config.low_threshold, config.high_threshold);
    LightSensor_SetAlsInterruptEnableAndSelect(CHAIN_DLIGHT_DEFAULT_INTERRUPT_ENABLE, 1U);
    LightSensor_SetAlsEnable(1U);

    s_interrupt_enable   = CHAIN_DLIGHT_DEFAULT_INTERRUPT_ENABLE;
    s_report_enable      = CHAIN_DLIGHT_DEFAULT_REPORT_ENABLE;
    s_measure_gain       = config.gain;
    s_measure_resolution = config.resolution;
    s_measure_rate       = config.rate;
    chain_dlight_reset_lux_history();
}

/**
 * @brief Handle light sensor interrupt event.
 * @retval None
 */
void handle_dlight_trigger_event(void)
{
    uint32_t lux_integer  = 0;
    uint8_t int_triggered = 0;
    uint8_t data_ready    = 0;

    if (!s_interrupt_enable) {
        return;
    }

    if (LightSensor_GetAlsStatus(&int_triggered, &data_ready) != HAL_OK) {
        return;
    }

    if (!int_triggered || !data_ready) {
        return;
    }

    if (chain_dlight_read_lux_sample(&lux_integer)) {
        chain_dlight_process_lux_sample(lux_integer);
    }
}

void chain_dlight_poll_lux_data(void)
{
    uint32_t lux_integer = 0;

    if (chain_dlight_read_lux_sample(&lux_integer)) {
        chain_dlight_process_lux_sample(lux_integer);
    }
}

uint8_t chain_dlight_is_interrupt_enabled(void)
{
    return s_interrupt_enable;
}

void chain_dlight_i2c_mem_read(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t read_len)
{
    uint8_t operation_result = OPERATION_FAIL;
    uint16_t reg_addr_size   = 0U;

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;

    if (chain_dlight_i2c_addr_is_valid(i2c_addr) && chain_dlight_i2c_length_is_valid(read_len) &&
        chain_dlight_i2c_get_reg_addr_size(reg_addr_length, &reg_addr_size) &&
        LightSensor_I2cMemRead(chain_dlight_i2c_make_hal_addr(i2c_addr), reg_addr, reg_addr_size, &s_ret_buf[1],
                               read_len) == HAL_OK) {
        s_ret_buf[0]   = OPERATION_SUCCESS;
        s_ret_buf_size = (uint8_t)(read_len + 1U);
    }

    chain_command_complete_return(CHAIN_DLIGHT_I2C_MEM_READ, s_ret_buf, s_ret_buf_size);
}

void chain_dlight_i2c_mem_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t write_len,
                                uint8_t *buffer)
{
    uint8_t operation_result = OPERATION_FAIL;
    uint16_t reg_addr_size   = 0U;

    if (buffer != NULL && chain_dlight_i2c_addr_is_valid(i2c_addr) && chain_dlight_i2c_length_is_valid(write_len) &&
        chain_dlight_i2c_get_reg_addr_size(reg_addr_length, &reg_addr_size) &&
        LightSensor_I2cMemWrite(chain_dlight_i2c_make_hal_addr(i2c_addr), reg_addr, reg_addr_size, buffer, write_len) ==
            HAL_OK) {
        operation_result = OPERATION_SUCCESS;
        if (i2c_addr == (uint8_t)(LIGHT_SENSOR_I2C_ADDR >> 1)) {
            chain_dlight_refresh_sensor_params_from_regs();
        }
    }

    chain_dlight_reply_status(CHAIN_DLIGHT_I2C_MEM_WRITE, operation_result);
}

void chain_dlight_get_lux_data_handle(void)
{
    s_ret_buf_size = 0;
    chain_dlight_put_uint32_le(&s_ret_buf[s_ret_buf_size], s_env_current_lux);
    s_ret_buf_size += 4U;

    chain_command_complete_return(CHAIN_DLIGHT_GET_LUX_DATA, s_ret_buf, s_ret_buf_size);
}

void chain_dlight_set_interrupt_enable(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer == NULL || size < 1U) {
        chain_dlight_reply_status(CHAIN_DLIGHT_SET_INTERRUPT_ENABLE, operation_result);
        return;
    }

    uint8_t enable = buffer[0];

    if (enable > 1U) {
        chain_dlight_reply_status(CHAIN_DLIGHT_SET_INTERRUPT_ENABLE, operation_result);
        return;
    }

    if (LightSensor_SetAlsInterruptEnableAndSelect(enable, 1U) == HAL_OK) {
        s_interrupt_enable = enable;
        chain_dlight_reset_lux_history();
        if (!enable) {
            rgb_set_env_threshold_overlay(0U);
        }
        operation_result = OPERATION_SUCCESS;
    }

    chain_dlight_reply_status(CHAIN_DLIGHT_SET_INTERRUPT_ENABLE, operation_result);
}

void chain_dlight_get_interrupt_enable(void)
{
    uint8_t enable = 0;

    s_ret_buf_size = 0;
    if (LightSensor_GetAlsInterruptConfig(&enable, NULL) == HAL_OK) {
        s_interrupt_enable = enable;
    } else {
        enable = s_interrupt_enable;
    }

    s_ret_buf[s_ret_buf_size++] = enable;
    chain_command_complete_return(CHAIN_DLIGHT_GET_INTERRUPT_ENABLE, s_ret_buf, s_ret_buf_size);
}

void chain_dlight_set_interrupt_threshold(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer == NULL || size < 8U) {
        chain_dlight_reply_status(CHAIN_DLIGHT_SET_INTERRUPT_THRESHOLD, operation_result);
        return;
    }

    uint32_t high_threshold = chain_dlight_make_uint32_le(&buffer[0]);
    uint32_t low_threshold  = chain_dlight_make_uint32_le(&buffer[4]);

    if (low_threshold < high_threshold && chain_dlight_lux_is_valid(low_threshold) &&
        chain_dlight_lux_is_valid(high_threshold) &&
        chain_dlight_set_thresholds_with_als_restart(low_threshold, high_threshold) == HAL_OK) {
        operation_result = OPERATION_SUCCESS;
        chain_dlight_reset_or_seed_lux_history();
    }

    chain_dlight_reply_status(CHAIN_DLIGHT_SET_INTERRUPT_THRESHOLD, operation_result);
}

void chain_dlight_get_interrupt_threshold(void)
{
    s_ret_buf_size = 0;

    chain_dlight_put_uint32_le(&s_ret_buf[s_ret_buf_size], s_high_threshold_lux);
    s_ret_buf_size += 4U;
    chain_dlight_put_uint32_le(&s_ret_buf[s_ret_buf_size], s_low_threshold_lux);
    s_ret_buf_size += 4U;
    chain_command_complete_return(CHAIN_DLIGHT_GET_INTERRUPT_THRESHOLD, s_ret_buf, s_ret_buf_size);
}

void chain_dlight_set_measure_gain(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer == NULL || size < 1U) {
        chain_dlight_reply_status(CHAIN_DLIGHT_SET_MEASURE_GAIN, operation_result);
        return;
    }

    uint8_t gain = buffer[0];

    if (chain_dlight_gain_is_valid(gain) &&
        chain_dlight_write_u8_with_als_restart(LightSensor_SetAlsGain, gain) == HAL_OK) {
        s_measure_gain   = gain;
        operation_result = OPERATION_SUCCESS;
        chain_dlight_reset_or_seed_lux_history();
    }

    chain_dlight_reply_status(CHAIN_DLIGHT_SET_MEASURE_GAIN, operation_result);
}

void chain_dlight_get_measure_gain(void)
{
    uint8_t gain = s_measure_gain;

    if (LightSensor_GetAlsGain(&gain) != HAL_OK || !chain_dlight_gain_is_valid(gain)) {
        gain = s_measure_gain;
    } else {
        s_measure_gain = gain;
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = gain;
    chain_command_complete_return(CHAIN_DLIGHT_GET_MEASURE_GAIN, s_ret_buf, s_ret_buf_size);
}

void chain_dlight_set_measure_resolution(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;
    uint8_t resolution_reg   = 0;

    if (buffer == NULL || size < 1U) {
        chain_dlight_reply_status(CHAIN_DLIGHT_SET_MEASURE_RESOLUTION, operation_result);
        return;
    }

    uint8_t resolution = buffer[0];

    if (chain_dlight_resolution_to_reg(resolution, &resolution_reg) &&
        chain_dlight_rate_to_measurement_time_ms(s_measure_rate) >
            chain_dlight_resolution_to_conversion_time_ms(resolution) &&
        chain_dlight_write_u8_with_als_restart(LightSensor_SetAlsResolution, resolution_reg) == HAL_OK) {
        s_measure_resolution = resolution;
        operation_result     = OPERATION_SUCCESS;
        chain_dlight_reset_or_seed_lux_history();
    }

    chain_dlight_reply_status(CHAIN_DLIGHT_SET_MEASURE_RESOLUTION, operation_result);
}

void chain_dlight_get_measure_resolution(void)
{
    uint8_t resolution_reg = 0;
    uint8_t resolution     = s_measure_resolution;

    if (LightSensor_GetAlsResolution(&resolution_reg) != HAL_OK ||
        !chain_dlight_reg_to_resolution(resolution_reg, &resolution)) {
        resolution = s_measure_resolution;
    } else {
        s_measure_resolution = resolution;
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = resolution;
    chain_command_complete_return(CHAIN_DLIGHT_GET_MEASURE_RESOLUTION, s_ret_buf, s_ret_buf_size);
}

void chain_dlight_set_measure_rate(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;
    uint8_t rate_reg         = 0;

    if (buffer == NULL || size < 1U) {
        chain_dlight_reply_status(CHAIN_DLIGHT_SET_MEASURE_RATE, operation_result);
        return;
    }

    uint8_t rate = buffer[0];

    if (chain_dlight_rate_to_reg(rate, &rate_reg) &&
        chain_dlight_rate_to_measurement_time_ms(rate) >
            chain_dlight_resolution_to_conversion_time_ms(s_measure_resolution) &&
        chain_dlight_write_u8_with_als_restart(LightSensor_SetAlsMeasurementRate, rate_reg) == HAL_OK) {
        s_measure_rate   = rate;
        operation_result = OPERATION_SUCCESS;
        chain_dlight_reset_or_seed_lux_history();
    }

    chain_dlight_reply_status(CHAIN_DLIGHT_SET_MEASURE_RATE, operation_result);
}

void chain_dlight_get_measure_rate(void)
{
    uint8_t rate_reg = 0;
    uint8_t rate     = s_measure_rate;

    if (LightSensor_GetAlsMeasurementRate(&rate_reg) != HAL_OK || !chain_dlight_reg_to_rate(rate_reg, &rate)) {
        rate = s_measure_rate;
    } else {
        s_measure_rate = rate;
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = rate;
    chain_command_complete_return(CHAIN_DLIGHT_GET_MEASURE_RATE, s_ret_buf, s_ret_buf_size);
}

void chain_dlight_set_report_enable(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer == NULL || size < 1U) {
        chain_dlight_reply_status(CHAIN_DLIGHT_SET_INTERRUPT_REPORT_ENABLE, operation_result);
        return;
    }

    uint8_t enable = buffer[0];

    if (enable <= 1U) {
        s_report_enable = enable;
        if (!enable) {
            rgb_set_env_threshold_overlay(0U);
        }
        operation_result = OPERATION_SUCCESS;
    }

    chain_dlight_reply_status(CHAIN_DLIGHT_SET_INTERRUPT_REPORT_ENABLE, operation_result);
}

void chain_dlight_get_report_enable(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = s_report_enable;
    chain_command_complete_return(CHAIN_DLIGHT_GET_INTERRUPT_REPORT_ENABLE, s_ret_buf, s_ret_buf_size);
}
