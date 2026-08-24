#include "imu_function.h"

#define CHAIN_IMU_EVENT_STABLE_SAMPLES (5U)
#define CHAIN_IMU_I2C_MAX_LENGTH       (64U)
#define CHAIN_IMU_SENSOR_I2C_ADDR      ((uint8_t)(LSM6DS3TR_C_I2C_ADD_H >> 1))

static uint8_t s_ret_buf[CHAIN_IMU_I2C_MAX_LENGTH + 1U] = {0};
static uint8_t s_ret_buf_size                           = 0;

__IO uint8_t s_accel_range     = IMU_SENSOR_ACCEL_RANGE_2G;
__IO uint8_t s_gyro_range      = IMU_SENSOR_GYRO_RANGE_250DPS;
__IO uint8_t s_accel_odr       = IMU_SENSOR_ACCEL_ODR_104HZ;
__IO uint8_t s_gyro_odr        = IMU_SENSOR_GYRO_ODR_104HZ;
__IO uint8_t s_event_mode      = (CHAIN_IMU_EVENT_MODE_ACCEL | CHAIN_IMU_EVENT_MODE_GYRO | CHAIN_IMU_EVENT_MODE_TEMP);
__IO uint8_t s_accel_threshold = 50U;
__IO uint8_t s_gyro_threshold  = 50U;
__IO uint8_t s_temp_threshold_low           = 10U;
__IO uint8_t s_temp_threshold_high          = 40U;
__IO uint8_t s_event_sample_valid           = 0U;
__IO int16_t s_motion_reference_accel_mg[3] = {0};
__IO int16_t s_motion_reference_gyro_dps[3] = {0};
__IO uint8_t s_last_int_status              = 0U;
__IO uint8_t s_accel_event_active           = 0U;
__IO uint8_t s_gyro_event_active            = 0U;
__IO uint8_t s_temp_event_active            = 0U;
__IO uint8_t s_temp_event_code              = 0U;
__IO uint8_t s_accel_active_count           = 0U;
__IO uint8_t s_accel_inactive_count         = 0U;
__IO uint8_t s_gyro_active_count            = 0U;
__IO uint8_t s_gyro_inactive_count          = 0U;
__IO uint8_t s_temp_active_count            = 0U;
__IO uint8_t s_temp_inactive_count          = 0U;

static uint8_t chain_imu_i2c_length_is_valid(uint8_t length)
{
    return (length > 0U && length <= CHAIN_IMU_I2C_MAX_LENGTH) ? 1U : 0U;
}

static uint8_t chain_imu_i2c_addr_is_valid(uint8_t i2c_addr)
{
    return (i2c_addr >= 0x08 && i2c_addr <= 0x77) ? 1U : 0U;
}

static uint8_t chain_imu_i2c_make_hal_addr(uint8_t i2c_addr)
{
    return (uint8_t)(i2c_addr << 1);
}

static uint8_t chain_imu_i2c_get_reg_addr_size(uint8_t reg_addr_length, uint16_t *reg_addr_size)
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

static uint8_t chain_imu_i2c_access_touches_reg(uint16_t access_addr, uint8_t access_len, uint8_t reg_addr)
{
    uint16_t access_end = (uint16_t)(access_addr + (uint16_t)access_len);

    return ((uint16_t)reg_addr >= access_addr && (uint16_t)reg_addr < access_end) ? 1U : 0U;
}

static uint8_t chain_imu_i2c_access_touches_sensor_params(uint16_t reg_addr, uint8_t length)
{
    if (chain_imu_i2c_access_touches_reg(reg_addr, length, LSM6DS3TR_C_CTRL1_XL) != 0U) {
        return 1U;
    }

    return chain_imu_i2c_access_touches_reg(reg_addr, length, LSM6DS3TR_C_CTRL2_G);
}

static int32_t chain_imu_refresh_sensor_params_from_regs(uint16_t reg_addr, uint8_t length)
{
    int32_t ret = 0;

    if (chain_imu_i2c_access_touches_reg(reg_addr, length, LSM6DS3TR_C_CTRL1_XL) != 0U) {
        imu_sensor_accel_range_t accel_range = IMU_SENSOR_ACCEL_RANGE_2G;
        imu_sensor_accel_odr_t accel_odr     = IMU_SENSOR_ACCEL_ODR_OFF;
        int32_t current_ret                  = imu_sensor_get_accel_range(&accel_range);

        if (current_ret == 0) {
            s_accel_range = (uint8_t)accel_range;
        } else {
            ret = current_ret;
        }

        current_ret = imu_sensor_get_accel_odr(&accel_odr);
        if (current_ret == 0) {
            s_accel_odr = (uint8_t)accel_odr;
        } else {
            ret = current_ret;
        }
    }

    if (chain_imu_i2c_access_touches_reg(reg_addr, length, LSM6DS3TR_C_CTRL2_G) != 0U) {
        imu_sensor_gyro_range_t gyro_range = IMU_SENSOR_GYRO_RANGE_250DPS;
        imu_sensor_gyro_odr_t gyro_odr     = IMU_SENSOR_GYRO_ODR_OFF;
        int32_t current_ret                = imu_sensor_get_gyro_range(&gyro_range);

        if (current_ret == 0) {
            s_gyro_range = (uint8_t)gyro_range;
        } else {
            ret = current_ret;
        }

        current_ret = imu_sensor_get_gyro_odr(&gyro_odr);
        if (current_ret == 0) {
            s_gyro_odr = (uint8_t)gyro_odr;
        } else {
            ret = current_ret;
        }
    }

    return ret;
}

static void chain_imu_put_int16_le(uint8_t *buffer, uint8_t index, int16_t value)
{
    uint16_t raw = (uint16_t)value;

    buffer[index]      = (uint8_t)(raw & 0xFFU);
    buffer[index + 1U] = (uint8_t)((raw >> 8) & 0xFFU);
}

static int16_t chain_imu_float_to_int16(float value)
{
    if (value >= (float)INT16_MAX) {
        return INT16_MAX;
    }

    if (value <= (float)INT16_MIN) {
        return INT16_MIN;
    }

    if (value >= 0.0f) {
        return (int16_t)(value + 0.5f);
    }

    return (int16_t)(value - 0.5f);
}

static void chain_imu_operation_status_return(uint8_t cmd, uint8_t status)
{
    uint8_t ret[1] = {status};

    chain_command_complete_return(cmd, ret, 1);
}

static int16_t chain_imu_abs_int16_delta(int16_t current, int16_t reference)
{
    int32_t delta = (int32_t)current - (int32_t)reference;

    if (delta < 0) {
        delta = -delta;
    }

    if (delta > INT16_MAX) {
        return INT16_MAX;
    }

    return (int16_t)delta;
}

static void chain_imu_set_motion_reference(const int16_t *accel_mg, const int16_t *gyro_dps)
{
    for (uint8_t i = 0; i < 3U; i++) {
        s_motion_reference_accel_mg[i] = accel_mg[i];
        s_motion_reference_gyro_dps[i] = gyro_dps[i];
    }
}

static void chain_imu_initialize_motion_reference(const int16_t *accel_mg, const int16_t *gyro_dps)
{
    chain_imu_set_motion_reference(accel_mg, gyro_dps);
    s_event_sample_valid = 1U;
}

static void chain_imu_set_accel_motion_reference(const int16_t *accel_mg)
{
    for (uint8_t i = 0; i < 3U; i++) {
        s_motion_reference_accel_mg[i] = accel_mg[i];
    }
}

static void chain_imu_set_gyro_motion_reference(const int16_t *gyro_dps)
{
    for (uint8_t i = 0; i < 3U; i++) {
        s_motion_reference_gyro_dps[i] = gyro_dps[i];
    }
}

static uint8_t chain_imu_check_axis_delta(const int16_t *current, const int16_t *previous, uint8_t threshold)
{
    for (uint8_t i = 0; i < 3U; i++) {
        if (chain_imu_abs_int16_delta(current[i], previous[i]) > (int16_t)threshold) {
            return 1U;
        }
    }

    return 0U;
}

static void chain_imu_update_stable_state(uint8_t instant_active, volatile uint8_t *stable_active,
                                          volatile uint8_t *active_count, volatile uint8_t *inactive_count)
{
    if ((stable_active == NULL) || (active_count == NULL) || (inactive_count == NULL)) {
        return;
    }

    if (instant_active != 0U) {
        *inactive_count = 0U;

        if (*stable_active == 0U) {
            if (*active_count < CHAIN_IMU_EVENT_STABLE_SAMPLES) {
                (*active_count)++;
            }

            if (*active_count >= CHAIN_IMU_EVENT_STABLE_SAMPLES) {
                *stable_active = 1U;
                *active_count  = 0U;
            }
        } else {
            *active_count = 0U;
        }
    } else {
        *active_count = 0U;

        if (*stable_active != 0U) {
            if (*inactive_count < CHAIN_IMU_EVENT_STABLE_SAMPLES) {
                (*inactive_count)++;
            }

            if (*inactive_count >= CHAIN_IMU_EVENT_STABLE_SAMPLES) {
                *stable_active  = 0U;
                *inactive_count = 0U;
            }
        } else {
            *inactive_count = 0U;
        }
    }
}

static uint8_t chain_imu_temperature_status_is_active(uint8_t status)
{
    uint8_t temp_status = status & CHAIN_IMU_INT_STATUS_TEMP_MASK;

    return (temp_status == CHAIN_IMU_INT_STATUS_TEMP_HIGH_ACTIVE || temp_status == CHAIN_IMU_INT_STATUS_TEMP_LOW_ACTIVE)
               ? 1U
               : 0U;
}

static void chain_imu_trigger_event_rgb(uint8_t current_status, uint8_t last_status)
{
    uint8_t current_motion_active = (current_status & (CHAIN_IMU_INT_STATUS_ACCEL | CHAIN_IMU_INT_STATUS_GYRO)) != 0U;
    uint8_t last_motion_active    = (last_status & (CHAIN_IMU_INT_STATUS_ACCEL | CHAIN_IMU_INT_STATUS_GYRO)) != 0U;
    uint8_t current_temp_active   = chain_imu_temperature_status_is_active(current_status);
    uint8_t last_temp_active      = chain_imu_temperature_status_is_active(last_status);

    if ((current_temp_active != 0U) && (last_temp_active == 0U)) {
        rgb_event_trigger(RGB_EVENT_RED);
    } else if ((current_temp_active == 0U) && (last_temp_active != 0U)) {
        rgb_event_clear(RGB_EVENT_RED);
    }

    if ((current_motion_active != 0U) && (last_motion_active == 0U)) {
        rgb_event_trigger(RGB_EVENT_BLUE);
    } else if ((current_motion_active == 0U) && (last_motion_active != 0U)) {
        rgb_event_clear(RGB_EVENT_BLUE);
    }
}

static uint8_t chain_imu_should_report_event(uint8_t current_status, uint8_t last_status)
{
    return (current_status != last_status) ? 1U : 0U;
}

void chain_imu_i2c_mem_read(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t read_len)
{
    uint8_t operation_result = OPERATION_FAIL;
    uint16_t reg_addr_size   = 0U;

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;

    if (chain_imu_i2c_addr_is_valid(i2c_addr) && chain_imu_i2c_length_is_valid(read_len) &&
        chain_imu_i2c_get_reg_addr_size(reg_addr_length, &reg_addr_size) &&
        HAL_I2C_Mem_Read(&hi2c2, chain_imu_i2c_make_hal_addr(i2c_addr), reg_addr, reg_addr_size, &s_ret_buf[1],
                         read_len, 100) == HAL_OK) {
        s_ret_buf[0]   = OPERATION_SUCCESS;
        s_ret_buf_size = (uint8_t)(read_len + 1U);
    }

    chain_command_complete_return(CHAIN_IMU_I2C_MEM_READ, s_ret_buf, s_ret_buf_size);
}

void chain_imu_i2c_mem_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t write_len,
                             uint8_t *buffer)
{
    uint8_t operation_result = OPERATION_FAIL;
    uint16_t reg_addr_size   = 0U;

    if (buffer != NULL && chain_imu_i2c_addr_is_valid(i2c_addr) && chain_imu_i2c_length_is_valid(write_len) &&
        chain_imu_i2c_get_reg_addr_size(reg_addr_length, &reg_addr_size) &&
        HAL_I2C_Mem_Write(&hi2c2, chain_imu_i2c_make_hal_addr(i2c_addr), reg_addr, reg_addr_size, buffer, write_len,
                          100) == HAL_OK) {
        operation_result = OPERATION_SUCCESS;
        if (i2c_addr == CHAIN_IMU_SENSOR_I2C_ADDR && reg_addr_length == 1U &&
            chain_imu_i2c_access_touches_sensor_params((uint8_t)reg_addr, write_len) != 0U) {
            (void)chain_imu_refresh_sensor_params_from_regs(reg_addr, write_len);
            (void)imu_data_update();
        }
    }

    chain_command_complete_return(CHAIN_IMU_I2C_MEM_WRITE, &operation_result, 1);
}

void chain_imu_get_accel_data_handle(void)
{
    uint8_t accel[6] = {0};

    (void)imu_data_update();
    for (uint8_t i = 0; i < 3U; i++) {
        chain_imu_put_int16_le(accel, (uint8_t)(i * 2U), chain_imu_float_to_int16(g_imu_sensor_data.unit.accel_mg[i]));
    }

    chain_command_complete_return(CHAIN_IMU_GET_ACCEL_DATA, accel, 6);
}

void chain_imu_get_gyro_data_handle(void)
{
    uint8_t gyro[6] = {0};

    (void)imu_data_update();
    for (uint8_t i = 0; i < 3U; i++) {
        chain_imu_put_int16_le(gyro, (uint8_t)(i * 2U),
                               chain_imu_float_to_int16(g_imu_sensor_data.unit.gyro_mdps[i] / 1000.0f));
    }

    chain_command_complete_return(CHAIN_IMU_GET_GYRO_DATA, gyro, 6);
}

void chain_imu_get_accel_raw_data_handle(void)
{
    uint8_t accel[6] = {0};

    (void)imu_data_update();
    for (uint8_t i = 0; i < 3U; i++) {
        chain_imu_put_int16_le(accel, (uint8_t)(i * 2U), g_imu_sensor_data.raw.accel_raw[i]);
    }

    chain_command_complete_return(CHAIN_IMU_GET_ACCEL_RAW_DATA, accel, 6);
}

void chain_imu_get_gyro_raw_data_handle(void)
{
    uint8_t gyro[6] = {0};

    (void)imu_data_update();
    for (uint8_t i = 0; i < 3U; i++) {
        chain_imu_put_int16_le(gyro, (uint8_t)(i * 2U), g_imu_sensor_data.raw.gyro_raw[i]);
    }

    chain_command_complete_return(CHAIN_IMU_GET_GYRO_RAW_DATA, gyro, 6);
}

void chain_imu_get_temperature_data_handle(void)
{
    uint8_t temperature[2] = {0};

    (void)imu_data_update();
    temperature[0] = (uint8_t)(imu_temperature_c_x100 & 0xFF);
    temperature[1] = (uint8_t)((imu_temperature_c_x100 >> 8) & 0xFF);
    chain_command_complete_return(CHAIN_IMU_GET_TEMPERATURE_DATA, temperature, 2);
}

void chain_imu_set_accel_range_handle(uint8_t accel_range)
{
    uint8_t operation_result          = OPERATION_FAIL;
    imu_sensor_accel_range_t readback = IMU_SENSOR_ACCEL_RANGE_2G;

    if ((imu_sensor_set_accel_range((imu_sensor_accel_range_t)accel_range) == 0) &&
        (imu_sensor_get_accel_range(&readback) == 0) && ((uint8_t)readback == accel_range)) {
        s_accel_range    = accel_range;
        operation_result = OPERATION_SUCCESS;
    }

    chain_imu_operation_status_return(CHAIN_IMU_SET_ACCEL_RANGE, operation_result);
}

void chain_imu_get_accel_range_handle(void)
{
    uint8_t accel_range[1] = {s_accel_range};

    chain_command_complete_return(CHAIN_IMU_GET_ACCEL_RANGE, accel_range, 1);
}

void chain_imu_set_gyro_range_handle(uint8_t gyro_range)
{
    uint8_t operation_result         = OPERATION_FAIL;
    imu_sensor_gyro_range_t readback = IMU_SENSOR_GYRO_RANGE_250DPS;

    if ((imu_sensor_set_gyro_range((imu_sensor_gyro_range_t)gyro_range) == 0) &&
        (imu_sensor_get_gyro_range(&readback) == 0) && ((uint8_t)readback == gyro_range)) {
        s_gyro_range     = gyro_range;
        operation_result = OPERATION_SUCCESS;
    }

    chain_imu_operation_status_return(CHAIN_IMU_SET_GYRO_RANGE, operation_result);
}

void chain_imu_get_gyro_range_handle(void)
{
    uint8_t gyro_range[1] = {s_gyro_range};

    chain_command_complete_return(CHAIN_IMU_GET_GYRO_RANGE, gyro_range, 1);
}

void chain_imu_set_accel_odr_handle(uint8_t accel_odr)
{
    uint8_t operation_result        = OPERATION_FAIL;
    imu_sensor_accel_odr_t readback = IMU_SENSOR_ACCEL_ODR_OFF;

    if ((imu_sensor_set_accel_odr((imu_sensor_accel_odr_t)accel_odr) == 0) &&
        (imu_sensor_get_accel_odr(&readback) == 0) && ((uint8_t)readback == accel_odr)) {
        s_accel_odr      = accel_odr;
        operation_result = OPERATION_SUCCESS;
    }

    chain_imu_operation_status_return(CHAIN_IMU_SET_ACCEL_ODR, operation_result);
}

void chain_imu_get_accel_odr_handle(void)
{
    uint8_t accel_odr[1] = {s_accel_odr};

    chain_command_complete_return(CHAIN_IMU_GET_ACCEL_ODR, accel_odr, 1);
}

void chain_imu_set_gyro_odr_handle(uint8_t gyro_odr)
{
    uint8_t operation_result       = OPERATION_FAIL;
    imu_sensor_gyro_odr_t readback = IMU_SENSOR_GYRO_ODR_OFF;

    if ((imu_sensor_set_gyro_odr((imu_sensor_gyro_odr_t)gyro_odr) == 0) && (imu_sensor_get_gyro_odr(&readback) == 0) &&
        ((uint8_t)readback == gyro_odr)) {
        s_gyro_odr       = gyro_odr;
        operation_result = OPERATION_SUCCESS;
    }

    chain_imu_operation_status_return(CHAIN_IMU_SET_GYRO_ODR, operation_result);
}

void chain_imu_get_gyro_odr_handle(void)
{
    uint8_t gyro_odr[1] = {s_gyro_odr};

    chain_command_complete_return(CHAIN_IMU_GET_GYRO_ODR, gyro_odr, 1);
}

void chain_imu_event_report_handle(uint8_t int_status)
{
    uint8_t report[2] = {int_status, CHAIN_IMU_REPORT_TYPE};

    chain_command_complete_return(CHAIN_IMU_EVENT_REPORT, report, 2);
}

void chain_imu_set_event_mode_handle(uint8_t mode)
{
    uint8_t operation_result = OPERATION_FAIL;

    if ((mode & 0xF8U) == 0U) {
        s_event_mode     = mode;
        operation_result = OPERATION_SUCCESS;
    }

    chain_imu_operation_status_return(CHAIN_IMU_SET_EVENT_MODE, operation_result);
}

void chain_imu_get_event_mode_handle(void)
{
    uint8_t mode[1] = {s_event_mode};

    chain_command_complete_return(CHAIN_IMU_GET_EVENT_MODE, mode, 1);
}

void chain_imu_set_axis_threshold_handle(uint8_t accel_threshold, uint8_t gyro_threshold)
{
    s_accel_threshold = accel_threshold;
    s_gyro_threshold  = gyro_threshold;

    chain_imu_operation_status_return(CHAIN_IMU_SET_AXIS_THRESHOLD, OPERATION_SUCCESS);
}

void chain_imu_get_axis_threshold_handle(void)
{
    uint8_t threshold[2] = {s_accel_threshold, s_gyro_threshold};

    chain_command_complete_return(CHAIN_IMU_GET_AXIS_THRESHOLD, threshold, 2);
}

void chain_imu_set_temp_threshold_handle(uint8_t temp_threshold_low, uint8_t temp_threshold_high)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (temp_threshold_low < temp_threshold_high) {
        s_temp_threshold_low  = temp_threshold_low;
        s_temp_threshold_high = temp_threshold_high;
        operation_result      = OPERATION_SUCCESS;
    }

    chain_imu_operation_status_return(CHAIN_IMU_SET_TEMP_THRESHOLD, operation_result);
}

void chain_imu_get_temp_threshold_handle(void)
{
    uint8_t threshold[2] = {s_temp_threshold_low, s_temp_threshold_high};

    chain_command_complete_return(CHAIN_IMU_GET_TEMP_THRESHOLD, threshold, 2);
}

void chain_imu_event_process(void)
{
    uint8_t int_status               = (uint8_t)(s_last_int_status & CHAIN_IMU_INT_STATUS_TEMP_MASK);
    uint16_t temp_c_x100             = imu_temperature_c_x100;
    int16_t current_accel_mg[3]      = {0};
    int16_t current_gyro_dps[3]      = {0};
    uint8_t instant_accel_active     = 0U;
    uint8_t instant_gyro_active      = 0U;
    uint8_t instant_temp_code        = 0U;
    uint8_t previous_temp_event_code = s_temp_event_code;

    for (uint8_t i = 0; i < 3U; i++) {
        current_accel_mg[i] = chain_imu_float_to_int16(g_imu_sensor_data.unit.accel_mg[i]);
        current_gyro_dps[i] = chain_imu_float_to_int16(g_imu_sensor_data.unit.gyro_mdps[i] / 1000.0f);
    }

    if (s_event_sample_valid == 0U) {
        chain_imu_initialize_motion_reference((const int16_t *)current_accel_mg, (const int16_t *)current_gyro_dps);
        return;
    }

    if ((s_event_mode & CHAIN_IMU_EVENT_MODE_ACCEL) != 0U) {
        instant_accel_active = chain_imu_check_axis_delta(
            (const int16_t *)current_accel_mg, (const int16_t *)s_motion_reference_accel_mg, s_accel_threshold);

        chain_imu_update_stable_state(instant_accel_active, &s_accel_event_active, &s_accel_active_count,
                                      &s_accel_inactive_count);

        if ((instant_accel_active != 0U) && (s_accel_event_active != 0U)) {
            chain_imu_set_accel_motion_reference((const int16_t *)current_accel_mg);
        } else if ((instant_accel_active == 0U) && (s_accel_event_active == 0U) && (s_accel_active_count == 0U)) {
            chain_imu_set_accel_motion_reference((const int16_t *)current_accel_mg);
        }
    } else {
        s_accel_event_active   = 0U;
        s_accel_active_count   = 0U;
        s_accel_inactive_count = 0U;
        chain_imu_set_accel_motion_reference((const int16_t *)current_accel_mg);
    }

    if (s_accel_event_active != 0U) {
        int_status |= CHAIN_IMU_INT_STATUS_ACCEL;
    }

    if ((s_event_mode & CHAIN_IMU_EVENT_MODE_GYRO) != 0U) {
        instant_gyro_active = chain_imu_check_axis_delta(
            (const int16_t *)current_gyro_dps, (const int16_t *)s_motion_reference_gyro_dps, s_gyro_threshold);

        chain_imu_update_stable_state(instant_gyro_active, &s_gyro_event_active, &s_gyro_active_count,
                                      &s_gyro_inactive_count);

        if ((instant_gyro_active != 0U) && (s_gyro_event_active != 0U)) {
            chain_imu_set_gyro_motion_reference((const int16_t *)current_gyro_dps);
        } else if ((instant_gyro_active == 0U) && (s_gyro_event_active == 0U) && (s_gyro_active_count == 0U)) {
            chain_imu_set_gyro_motion_reference((const int16_t *)current_gyro_dps);
        }
    } else {
        s_gyro_event_active   = 0U;
        s_gyro_active_count   = 0U;
        s_gyro_inactive_count = 0U;
        chain_imu_set_gyro_motion_reference((const int16_t *)current_gyro_dps);
    }

    if (s_gyro_event_active != 0U) {
        int_status |= CHAIN_IMU_INT_STATUS_GYRO;
    }

    if ((s_event_mode & CHAIN_IMU_EVENT_MODE_TEMP) != 0U) {
        if (temp_c_x100 > ((uint16_t)s_temp_threshold_high * 100U)) {
            instant_temp_code = CHAIN_IMU_INT_STATUS_TEMP_HIGH_ACTIVE;
        } else if (temp_c_x100 < ((uint16_t)s_temp_threshold_low * 100U)) {
            instant_temp_code = CHAIN_IMU_INT_STATUS_TEMP_LOW_ACTIVE;
        }

        chain_imu_update_stable_state((instant_temp_code != 0U) ? 1U : 0U, &s_temp_event_active, &s_temp_active_count,
                                      &s_temp_inactive_count);

        if (s_temp_event_active != 0U) {
            if (instant_temp_code != 0U) {
                s_temp_event_code = instant_temp_code;
            }

            int_status &= (uint8_t)(~CHAIN_IMU_INT_STATUS_TEMP_MASK);
            int_status |= s_temp_event_code;
        } else {
            if (previous_temp_event_code == CHAIN_IMU_INT_STATUS_TEMP_LOW_ACTIVE) {
                int_status &= (uint8_t)(~CHAIN_IMU_INT_STATUS_TEMP_MASK);
                int_status |= CHAIN_IMU_INT_STATUS_TEMP_LOW_RELEASE;
            } else if (previous_temp_event_code == CHAIN_IMU_INT_STATUS_TEMP_HIGH_ACTIVE) {
                int_status &= (uint8_t)(~CHAIN_IMU_INT_STATUS_TEMP_MASK);
                int_status |= CHAIN_IMU_INT_STATUS_TEMP_HIGH_RELEASE;
            }

            s_temp_event_code = 0U;
        }
    } else {
        int_status &= (uint8_t)(~CHAIN_IMU_INT_STATUS_TEMP_MASK);
        s_temp_event_active   = 0U;
        s_temp_event_code     = 0U;
        s_temp_active_count   = 0U;
        s_temp_inactive_count = 0U;
    }

    if (chain_imu_should_report_event(int_status, s_last_int_status) != 0U) {
        chain_imu_trigger_event_rgb(int_status, s_last_int_status);
        chain_imu_event_report_handle(int_status);
        s_last_int_status = int_status;
    }
}
