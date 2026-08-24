#include "../Sensor_Function/imu_sensor.h"

stmdev_ctx_t dev_ctx;
volatile imu_sensor_data_t g_imu_sensor_data;
volatile int32_t g_imu_sensor_last_status = 0;
volatile uint16_t imu_temperature_c_x100  = 0U;

#define IMU_SENSOR_ACCEL_FS  LSM6DS3TR_C_2g
#define IMU_SENSOR_GYRO_FS   LSM6DS3TR_C_250dps
#define IMU_SENSOR_ACCEL_ODR LSM6DS3TR_C_XL_ODR_104Hz
#define IMU_SENSOR_GYRO_ODR  LSM6DS3TR_C_GY_ODR_104Hz

static lsm6ds3tr_c_fs_xl_t s_accel_fs = IMU_SENSOR_ACCEL_FS;
static lsm6ds3tr_c_fs_g_t s_gyro_fs   = IMU_SENSOR_GYRO_FS;

static int32_t IMU_WriteRegister(void *handle, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Write((I2C_HandleTypeDef *)handle, LSM6DS3TR_C_I2C_ADD_H, reg, I2C_MEMADD_SIZE_8BIT,
                             (uint8_t *)buf, len, 100);
}

static int32_t IMU_ReadRegister(void *handle, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read((I2C_HandleTypeDef *)handle, LSM6DS3TR_C_I2C_ADD_H, reg, I2C_MEMADD_SIZE_8BIT, buf, len,
                            100);
}

static void IMU_Delay(uint32_t millisec)
{
    HAL_Delay(millisec);
}

static float IMU_ConvertAccelRawToMg(int16_t raw)
{
    switch (s_accel_fs) {
        case LSM6DS3TR_C_4g:
            return lsm6ds3tr_c_from_fs4g_to_mg(raw);
        case LSM6DS3TR_C_8g:
            return lsm6ds3tr_c_from_fs8g_to_mg(raw);
        case LSM6DS3TR_C_16g:
            return lsm6ds3tr_c_from_fs16g_to_mg(raw);
        case LSM6DS3TR_C_2g:
        default:
            return lsm6ds3tr_c_from_fs2g_to_mg(raw);
    }
}

static float IMU_ConvertGyroRawToMdps(int16_t raw)
{
    switch (s_gyro_fs) {
        case LSM6DS3TR_C_125dps:
            return lsm6ds3tr_c_from_fs125dps_to_mdps(raw);
        case LSM6DS3TR_C_500dps:
            return lsm6ds3tr_c_from_fs500dps_to_mdps(raw);
        case LSM6DS3TR_C_1000dps:
            return lsm6ds3tr_c_from_fs1000dps_to_mdps(raw);
        case LSM6DS3TR_C_2000dps:
            return lsm6ds3tr_c_from_fs2000dps_to_mdps(raw);
        case LSM6DS3TR_C_250dps:
        default:
            return lsm6ds3tr_c_from_fs250dps_to_mdps(raw);
    }
}

static float IMU_ConvertTemperatureRawToCelsius(int16_t raw)
{
    return lsm6ds3tr_c_from_lsb_to_celsius(raw);
}

static uint16_t IMU_ConvertTemperatureCelsiusToX100(float temperature_c)
{
    if (temperature_c <= 0.0f) {
        return 0U;
    }

    if (temperature_c >= 655.35f) {
        return UINT16_MAX;
    }

    return (uint16_t)(temperature_c * 100.0f);
}

static void IMU_ConvertRawDataToUnitData(const imu_sensor_raw_data_t *raw_data, imu_sensor_unit_data_t *unit_data)
{
    if ((raw_data == NULL) || (unit_data == NULL)) {
        return;
    }

    for (uint8_t i = 0; i < 3U; i++) {
        unit_data->accel_mg[i]  = IMU_ConvertAccelRawToMg(raw_data->accel_raw[i]);
        unit_data->gyro_mdps[i] = IMU_ConvertGyroRawToMdps(raw_data->gyro_raw[i]);
    }

    unit_data->temperature_c          = IMU_ConvertTemperatureRawToCelsius(raw_data->temperature_raw);
    unit_data->accel_data_ready       = raw_data->accel_data_ready;
    unit_data->gyro_data_ready        = raw_data->gyro_data_ready;
    unit_data->temperature_data_ready = raw_data->temperature_data_ready;
}

static int32_t IMU_UpdateStatus(int32_t ret)
{
    g_imu_sensor_last_status = ret;
    return ret;
}

static int32_t IMU_MapAccelRange(imu_sensor_accel_range_t range, lsm6ds3tr_c_fs_xl_t *fs)
{
    if (fs == NULL) {
        return -1;
    }

    switch (range) {
        case IMU_SENSOR_ACCEL_RANGE_2G:
            *fs = LSM6DS3TR_C_2g;
            break;
        case IMU_SENSOR_ACCEL_RANGE_4G:
            *fs = LSM6DS3TR_C_4g;
            break;
        case IMU_SENSOR_ACCEL_RANGE_8G:
            *fs = LSM6DS3TR_C_8g;
            break;
        case IMU_SENSOR_ACCEL_RANGE_16G:
            *fs = LSM6DS3TR_C_16g;
            break;
        default:
            return -1;
    }

    return 0;
}

static int32_t IMU_MapGyroRange(imu_sensor_gyro_range_t range, lsm6ds3tr_c_fs_g_t *fs)
{
    if (fs == NULL) {
        return -1;
    }

    switch (range) {
        case IMU_SENSOR_GYRO_RANGE_125DPS:
            *fs = LSM6DS3TR_C_125dps;
            break;
        case IMU_SENSOR_GYRO_RANGE_250DPS:
            *fs = LSM6DS3TR_C_250dps;
            break;
        case IMU_SENSOR_GYRO_RANGE_500DPS:
            *fs = LSM6DS3TR_C_500dps;
            break;
        case IMU_SENSOR_GYRO_RANGE_1000DPS:
            *fs = LSM6DS3TR_C_1000dps;
            break;
        case IMU_SENSOR_GYRO_RANGE_2000DPS:
            *fs = LSM6DS3TR_C_2000dps;
            break;
        default:
            return -1;
    }

    return 0;
}

static int32_t IMU_MapAccelOdr(imu_sensor_accel_odr_t odr, lsm6ds3tr_c_odr_xl_t *xl_odr)
{
    if (xl_odr == NULL) {
        return -1;
    }

    switch (odr) {
        case IMU_SENSOR_ACCEL_ODR_OFF:
            *xl_odr = LSM6DS3TR_C_XL_ODR_OFF;
            break;
        case IMU_SENSOR_ACCEL_ODR_12HZ5:
            *xl_odr = LSM6DS3TR_C_XL_ODR_12Hz5;
            break;
        case IMU_SENSOR_ACCEL_ODR_26HZ:
            *xl_odr = LSM6DS3TR_C_XL_ODR_26Hz;
            break;
        case IMU_SENSOR_ACCEL_ODR_52HZ:
            *xl_odr = LSM6DS3TR_C_XL_ODR_52Hz;
            break;
        case IMU_SENSOR_ACCEL_ODR_104HZ:
            *xl_odr = LSM6DS3TR_C_XL_ODR_104Hz;
            break;
        case IMU_SENSOR_ACCEL_ODR_208HZ:
            *xl_odr = LSM6DS3TR_C_XL_ODR_208Hz;
            break;
        case IMU_SENSOR_ACCEL_ODR_416HZ:
            *xl_odr = LSM6DS3TR_C_XL_ODR_416Hz;
            break;
        case IMU_SENSOR_ACCEL_ODR_833HZ:
            *xl_odr = LSM6DS3TR_C_XL_ODR_833Hz;
            break;
        case IMU_SENSOR_ACCEL_ODR_1660HZ:
            *xl_odr = LSM6DS3TR_C_XL_ODR_1k66Hz;
            break;
        case IMU_SENSOR_ACCEL_ODR_3330HZ:
            *xl_odr = LSM6DS3TR_C_XL_ODR_3k33Hz;
            break;
        case IMU_SENSOR_ACCEL_ODR_6660HZ:
            *xl_odr = LSM6DS3TR_C_XL_ODR_6k66Hz;
            break;
        case IMU_SENSOR_ACCEL_ODR_1HZ6:
            *xl_odr = LSM6DS3TR_C_XL_ODR_1Hz6;
            break;
        default:
            return -1;
    }

    return 0;
}

static int32_t IMU_MapGyroOdr(imu_sensor_gyro_odr_t odr, lsm6ds3tr_c_odr_g_t *gy_odr)
{
    if (gy_odr == NULL) {
        return -1;
    }

    switch (odr) {
        case IMU_SENSOR_GYRO_ODR_OFF:
            *gy_odr = LSM6DS3TR_C_GY_ODR_OFF;
            break;
        case IMU_SENSOR_GYRO_ODR_12HZ5:
            *gy_odr = LSM6DS3TR_C_GY_ODR_12Hz5;
            break;
        case IMU_SENSOR_GYRO_ODR_26HZ:
            *gy_odr = LSM6DS3TR_C_GY_ODR_26Hz;
            break;
        case IMU_SENSOR_GYRO_ODR_52HZ:
            *gy_odr = LSM6DS3TR_C_GY_ODR_52Hz;
            break;
        case IMU_SENSOR_GYRO_ODR_104HZ:
            *gy_odr = LSM6DS3TR_C_GY_ODR_104Hz;
            break;
        case IMU_SENSOR_GYRO_ODR_208HZ:
            *gy_odr = LSM6DS3TR_C_GY_ODR_208Hz;
            break;
        case IMU_SENSOR_GYRO_ODR_416HZ:
            *gy_odr = LSM6DS3TR_C_GY_ODR_416Hz;
            break;
        case IMU_SENSOR_GYRO_ODR_833HZ:
            *gy_odr = LSM6DS3TR_C_GY_ODR_833Hz;
            break;
        case IMU_SENSOR_GYRO_ODR_1660HZ:
            *gy_odr = LSM6DS3TR_C_GY_ODR_1k66Hz;
            break;
        case IMU_SENSOR_GYRO_ODR_3330HZ:
            *gy_odr = LSM6DS3TR_C_GY_ODR_3k33Hz;
            break;
        case IMU_SENSOR_GYRO_ODR_6660HZ:
            *gy_odr = LSM6DS3TR_C_GY_ODR_6k66Hz;
            break;
        default:
            return -1;
    }

    return 0;
}

static int32_t IMU_MapAccelRangeFromReg(lsm6ds3tr_c_fs_xl_t fs, imu_sensor_accel_range_t *range)
{
    if (range == NULL) {
        return -1;
    }

    switch (fs) {
        case LSM6DS3TR_C_2g:
            *range = IMU_SENSOR_ACCEL_RANGE_2G;
            break;
        case LSM6DS3TR_C_4g:
            *range = IMU_SENSOR_ACCEL_RANGE_4G;
            break;
        case LSM6DS3TR_C_8g:
            *range = IMU_SENSOR_ACCEL_RANGE_8G;
            break;
        case LSM6DS3TR_C_16g:
            *range = IMU_SENSOR_ACCEL_RANGE_16G;
            break;
        default:
            return -1;
    }

    return 0;
}

static int32_t IMU_MapGyroRangeFromReg(lsm6ds3tr_c_fs_g_t fs, imu_sensor_gyro_range_t *range)
{
    if (range == NULL) {
        return -1;
    }

    switch (fs) {
        case LSM6DS3TR_C_125dps:
            *range = IMU_SENSOR_GYRO_RANGE_125DPS;
            break;
        case LSM6DS3TR_C_250dps:
            *range = IMU_SENSOR_GYRO_RANGE_250DPS;
            break;
        case LSM6DS3TR_C_500dps:
            *range = IMU_SENSOR_GYRO_RANGE_500DPS;
            break;
        case LSM6DS3TR_C_1000dps:
            *range = IMU_SENSOR_GYRO_RANGE_1000DPS;
            break;
        case LSM6DS3TR_C_2000dps:
            *range = IMU_SENSOR_GYRO_RANGE_2000DPS;
            break;
        default:
            return -1;
    }

    return 0;
}

static int32_t IMU_MapAccelOdrFromReg(lsm6ds3tr_c_odr_xl_t odr, imu_sensor_accel_odr_t *xl_odr)
{
    if (xl_odr == NULL) {
        return -1;
    }

    switch (odr) {
        case LSM6DS3TR_C_XL_ODR_OFF:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_OFF;
            break;
        case LSM6DS3TR_C_XL_ODR_12Hz5:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_12HZ5;
            break;
        case LSM6DS3TR_C_XL_ODR_26Hz:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_26HZ;
            break;
        case LSM6DS3TR_C_XL_ODR_52Hz:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_52HZ;
            break;
        case LSM6DS3TR_C_XL_ODR_104Hz:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_104HZ;
            break;
        case LSM6DS3TR_C_XL_ODR_208Hz:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_208HZ;
            break;
        case LSM6DS3TR_C_XL_ODR_416Hz:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_416HZ;
            break;
        case LSM6DS3TR_C_XL_ODR_833Hz:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_833HZ;
            break;
        case LSM6DS3TR_C_XL_ODR_1k66Hz:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_1660HZ;
            break;
        case LSM6DS3TR_C_XL_ODR_3k33Hz:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_3330HZ;
            break;
        case LSM6DS3TR_C_XL_ODR_6k66Hz:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_6660HZ;
            break;
        case LSM6DS3TR_C_XL_ODR_1Hz6:
            *xl_odr = IMU_SENSOR_ACCEL_ODR_1HZ6;
            break;
        default:
            return -1;
    }

    return 0;
}

static int32_t IMU_MapGyroOdrFromReg(lsm6ds3tr_c_odr_g_t odr, imu_sensor_gyro_odr_t *gy_odr)
{
    if (gy_odr == NULL) {
        return -1;
    }

    switch (odr) {
        case LSM6DS3TR_C_GY_ODR_OFF:
            *gy_odr = IMU_SENSOR_GYRO_ODR_OFF;
            break;
        case LSM6DS3TR_C_GY_ODR_12Hz5:
            *gy_odr = IMU_SENSOR_GYRO_ODR_12HZ5;
            break;
        case LSM6DS3TR_C_GY_ODR_26Hz:
            *gy_odr = IMU_SENSOR_GYRO_ODR_26HZ;
            break;
        case LSM6DS3TR_C_GY_ODR_52Hz:
            *gy_odr = IMU_SENSOR_GYRO_ODR_52HZ;
            break;
        case LSM6DS3TR_C_GY_ODR_104Hz:
            *gy_odr = IMU_SENSOR_GYRO_ODR_104HZ;
            break;
        case LSM6DS3TR_C_GY_ODR_208Hz:
            *gy_odr = IMU_SENSOR_GYRO_ODR_208HZ;
            break;
        case LSM6DS3TR_C_GY_ODR_416Hz:
            *gy_odr = IMU_SENSOR_GYRO_ODR_416HZ;
            break;
        case LSM6DS3TR_C_GY_ODR_833Hz:
            *gy_odr = IMU_SENSOR_GYRO_ODR_833HZ;
            break;
        case LSM6DS3TR_C_GY_ODR_1k66Hz:
            *gy_odr = IMU_SENSOR_GYRO_ODR_1660HZ;
            break;
        case LSM6DS3TR_C_GY_ODR_3k33Hz:
            *gy_odr = IMU_SENSOR_GYRO_ODR_3330HZ;
            break;
        case LSM6DS3TR_C_GY_ODR_6k66Hz:
            *gy_odr = IMU_SENSOR_GYRO_ODR_6660HZ;
            break;
        default:
            return -1;
    }

    return 0;
}

static void IMU_MergeInt1Route(lsm6ds3tr_c_int1_route_t *route, uint16_t events, uint8_t enable)
{
    uint8_t value = (enable != 0U) ? 1U : 0U;

    if (route == NULL) {
        return;
    }

    if ((events & IMU_SENSOR_EVENT_GYRO_DRDY) != 0U) {
        route->int1_drdy_g = value;
    }

    if ((events & IMU_SENSOR_EVENT_ACCEL_DRDY) != 0U) {
        route->int1_drdy_xl = value;
    }

    if ((events & IMU_SENSOR_EVENT_WAKE_UP) != 0U) {
        route->int1_wu = value;
    }
}

static void IMU_MergeInt2Route(lsm6ds3tr_c_int2_route_t *route, uint16_t events, uint8_t enable)
{
    uint8_t value = (enable != 0U) ? 1U : 0U;

    if (route == NULL) {
        return;
    }

    if ((events & IMU_SENSOR_EVENT_ACCEL_DRDY) != 0U) {
        route->int2_drdy_xl = value;
    }

    if ((events & IMU_SENSOR_EVENT_GYRO_DRDY) != 0U) {
        route->int2_drdy_g = value;
    }

    if ((events & IMU_SENSOR_EVENT_TEMP_DRDY) != 0U) {
        route->int2_drdy_temp = value;
    }

    if ((events & IMU_SENSOR_EVENT_WAKE_UP) != 0U) {
        route->int2_wu = value;
    }
}

static int32_t IMU_ConfigureWakeUpDefaults(void)
{
    int32_t ret = 0;

    ret = lsm6ds3tr_c_filter_settling_mask_set(&dev_ctx, PROPERTY_ENABLE);
    if (ret == 0) {
        ret = lsm6ds3tr_c_xl_hp_path_internal_set(&dev_ctx, LSM6DS3TR_C_USE_SLOPE);
    }
    if (ret == 0) {
        ret = lsm6ds3tr_c_wkup_threshold_set(&dev_ctx, 0x02U);
    }
    if (ret == 0) {
        ret = lsm6ds3tr_c_wkup_dur_set(&dev_ctx, 0x00U);
    }

    return ret;
}

int32_t imu_sensor_init(void)
{
    uint8_t who_am_i = 0;
    int32_t ret      = 0;

    dev_ctx.write_reg = IMU_WriteRegister;
    dev_ctx.read_reg  = IMU_ReadRegister;
    dev_ctx.mdelay    = IMU_Delay;
    dev_ctx.handle    = &hi2c2;
    dev_ctx.priv_data = NULL;

    ret = lsm6ds3tr_c_device_id_get(&dev_ctx, &who_am_i);
    if (ret != 0) {
        g_imu_sensor_last_status = ret;
        return ret;
    }

    if (who_am_i != LSM6DS3TR_C_ID) {
        g_imu_sensor_last_status = -1;
        return g_imu_sensor_last_status;
    }

    ret = lsm6ds3tr_c_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
    if (ret == 0) {
        ret = lsm6ds3tr_c_auto_increment_set(&dev_ctx, PROPERTY_ENABLE);
    }
    if (ret == 0) {
        ret = lsm6ds3tr_c_xl_full_scale_set(&dev_ctx, IMU_SENSOR_ACCEL_FS);
    }
    if (ret == 0) {
        ret = lsm6ds3tr_c_gy_full_scale_set(&dev_ctx, IMU_SENSOR_GYRO_FS);
    }
    if (ret == 0) {
        ret = lsm6ds3tr_c_xl_data_rate_set(&dev_ctx, IMU_SENSOR_ACCEL_ODR);
    }
    if (ret == 0) {
        ret = lsm6ds3tr_c_gy_data_rate_set(&dev_ctx, IMU_SENSOR_GYRO_ODR);
    }
    if (ret == 0) {
        s_accel_fs = IMU_SENSOR_ACCEL_FS;
        s_gyro_fs  = IMU_SENSOR_GYRO_FS;
    }

    g_imu_sensor_last_status = ret;
    return ret;
}

int32_t imu_sensor_set_accel_range(imu_sensor_accel_range_t range)
{
    int32_t ret            = 0;
    lsm6ds3tr_c_fs_xl_t fs = LSM6DS3TR_C_2g;

    ret = IMU_MapAccelRange(range, &fs);
    if (ret != 0) {
        return IMU_UpdateStatus(ret);
    }

    ret = lsm6ds3tr_c_xl_full_scale_set(&dev_ctx, fs);
    if (ret == 0) {
        s_accel_fs = fs;
    }

    return IMU_UpdateStatus(ret);
}

int32_t imu_sensor_set_gyro_range(imu_sensor_gyro_range_t range)
{
    int32_t ret           = 0;
    lsm6ds3tr_c_fs_g_t fs = LSM6DS3TR_C_250dps;

    ret = IMU_MapGyroRange(range, &fs);
    if (ret != 0) {
        return IMU_UpdateStatus(ret);
    }

    ret = lsm6ds3tr_c_gy_full_scale_set(&dev_ctx, fs);
    if (ret == 0) {
        s_gyro_fs = fs;
    }

    return IMU_UpdateStatus(ret);
}

int32_t imu_sensor_set_accel_odr(imu_sensor_accel_odr_t odr)
{
    int32_t ret                 = 0;
    lsm6ds3tr_c_odr_xl_t xl_odr = LSM6DS3TR_C_XL_ODR_OFF;

    ret = IMU_MapAccelOdr(odr, &xl_odr);
    if (ret != 0) {
        return IMU_UpdateStatus(ret);
    }

    ret = lsm6ds3tr_c_xl_data_rate_set(&dev_ctx, xl_odr);
    return IMU_UpdateStatus(ret);
}

int32_t imu_sensor_set_gyro_odr(imu_sensor_gyro_odr_t odr)
{
    int32_t ret                = 0;
    lsm6ds3tr_c_odr_g_t gy_odr = LSM6DS3TR_C_GY_ODR_OFF;

    ret = IMU_MapGyroOdr(odr, &gy_odr);
    if (ret != 0) {
        return IMU_UpdateStatus(ret);
    }

    ret = lsm6ds3tr_c_gy_data_rate_set(&dev_ctx, gy_odr);
    return IMU_UpdateStatus(ret);
}

int32_t imu_sensor_get_accel_range(imu_sensor_accel_range_t *range)
{
    int32_t ret            = 0;
    lsm6ds3tr_c_fs_xl_t fs = LSM6DS3TR_C_2g;

    if (range == NULL) {
        return IMU_UpdateStatus(-1);
    }

    ret = lsm6ds3tr_c_xl_full_scale_get(&dev_ctx, &fs);
    if (ret != 0) {
        return IMU_UpdateStatus(ret);
    }

    if (IMU_MapAccelRangeFromReg(fs, range) != 0) {
        return IMU_UpdateStatus(-1);
    }

    s_accel_fs = fs;
    return IMU_UpdateStatus(0);
}

int32_t imu_sensor_get_gyro_range(imu_sensor_gyro_range_t *range)
{
    int32_t ret           = 0;
    lsm6ds3tr_c_fs_g_t fs = LSM6DS3TR_C_250dps;

    if (range == NULL) {
        return IMU_UpdateStatus(-1);
    }

    ret = lsm6ds3tr_c_gy_full_scale_get(&dev_ctx, &fs);
    if (ret != 0) {
        return IMU_UpdateStatus(ret);
    }

    if (IMU_MapGyroRangeFromReg(fs, range) != 0) {
        return IMU_UpdateStatus(-1);
    }

    s_gyro_fs = fs;
    return IMU_UpdateStatus(0);
}

int32_t imu_sensor_get_accel_odr(imu_sensor_accel_odr_t *odr)
{
    int32_t ret                 = 0;
    lsm6ds3tr_c_odr_xl_t xl_odr = LSM6DS3TR_C_XL_ODR_OFF;

    if (odr == NULL) {
        return IMU_UpdateStatus(-1);
    }

    ret = lsm6ds3tr_c_xl_data_rate_get(&dev_ctx, &xl_odr);
    if (ret != 0) {
        return IMU_UpdateStatus(ret);
    }

    if (IMU_MapAccelOdrFromReg(xl_odr, odr) != 0) {
        return IMU_UpdateStatus(-1);
    }

    return IMU_UpdateStatus(0);
}

int32_t imu_sensor_get_gyro_odr(imu_sensor_gyro_odr_t *odr)
{
    int32_t ret                = 0;
    lsm6ds3tr_c_odr_g_t gy_odr = LSM6DS3TR_C_GY_ODR_OFF;

    if (odr == NULL) {
        return IMU_UpdateStatus(-1);
    }

    ret = lsm6ds3tr_c_gy_data_rate_get(&dev_ctx, &gy_odr);
    if (ret != 0) {
        return IMU_UpdateStatus(ret);
    }

    if (IMU_MapGyroOdrFromReg(gy_odr, odr) != 0) {
        return IMU_UpdateStatus(-1);
    }

    return IMU_UpdateStatus(0);
}

int32_t imu_sensor_config_interrupt_pin(imu_sensor_int_pin_mode_t mode, imu_sensor_int_polarity_t polarity,
                                        imu_sensor_int_latch_mode_t latch_mode)
{
    int32_t ret                          = 0;
    lsm6ds3tr_c_pp_od_t pin_mode         = LSM6DS3TR_C_PUSH_PULL;
    lsm6ds3tr_c_h_lactive_t pin_polarity = LSM6DS3TR_C_ACTIVE_HIGH;
    lsm6ds3tr_c_lir_t notification       = LSM6DS3TR_C_INT_PULSED;

    pin_mode     = (mode == IMU_SENSOR_INT_OPEN_DRAIN) ? LSM6DS3TR_C_OPEN_DRAIN : LSM6DS3TR_C_PUSH_PULL;
    pin_polarity = (polarity == IMU_SENSOR_INT_ACTIVE_LOW) ? LSM6DS3TR_C_ACTIVE_LOW : LSM6DS3TR_C_ACTIVE_HIGH;
    notification = (latch_mode == IMU_SENSOR_INT_LATCHED) ? LSM6DS3TR_C_INT_LATCHED : LSM6DS3TR_C_INT_PULSED;

    ret = lsm6ds3tr_c_pin_mode_set(&dev_ctx, pin_mode);
    if (ret == 0) {
        ret = lsm6ds3tr_c_pin_polarity_set(&dev_ctx, pin_polarity);
    }
    if (ret == 0) {
        ret = lsm6ds3tr_c_int_notification_set(&dev_ctx, notification);
    }

    return IMU_UpdateStatus(ret);
}

int32_t imu_sensor_enable_interrupt(imu_sensor_int_pin_t pin, uint16_t events)
{
    int32_t ret                         = 0;
    lsm6ds3tr_c_int1_route_t int1_route = {0};
    lsm6ds3tr_c_int2_route_t int2_route = {0};

    if ((ret == 0) && ((events & IMU_SENSOR_EVENT_WAKE_UP) != 0U)) {
        ret = IMU_ConfigureWakeUpDefaults();
    }

    if ((ret == 0) && (pin == IMU_SENSOR_INT1)) {
        if ((events & IMU_SENSOR_EVENT_TEMP_DRDY) != 0U) {
            return IMU_UpdateStatus(-1);
        }

        ret = lsm6ds3tr_c_pin_int1_route_get(&dev_ctx, &int1_route);
        if (ret == 0) {
            IMU_MergeInt1Route(&int1_route, events, 1U);
            ret = lsm6ds3tr_c_pin_int1_route_set(&dev_ctx, int1_route);
        }
    }

    if ((ret == 0) && (pin == IMU_SENSOR_INT2)) {
        ret = lsm6ds3tr_c_pin_int2_route_get(&dev_ctx, &int2_route);
        if (ret == 0) {
            IMU_MergeInt2Route(&int2_route, events, 1U);
            ret = lsm6ds3tr_c_pin_int2_route_set(&dev_ctx, int2_route);
        }
    }

    if ((ret == 0) && (pin != IMU_SENSOR_INT1) && (pin != IMU_SENSOR_INT2)) {
        ret = -1;
    }

    return IMU_UpdateStatus(ret);
}

int32_t imu_sensor_disable_interrupt(imu_sensor_int_pin_t pin, uint16_t events)
{
    int32_t ret                         = 0;
    lsm6ds3tr_c_int1_route_t int1_route = {0};
    lsm6ds3tr_c_int2_route_t int2_route = {0};

    if ((pin == IMU_SENSOR_INT1) && (events == IMU_SENSOR_EVENT_NONE)) {
        ret = lsm6ds3tr_c_pin_int1_route_get(&dev_ctx, &int1_route);
        if (ret == 0) {
            IMU_MergeInt1Route(
                &int1_route,
                (uint16_t)(IMU_SENSOR_EVENT_ACCEL_DRDY | IMU_SENSOR_EVENT_GYRO_DRDY | IMU_SENSOR_EVENT_WAKE_UP), 0U);
            ret = lsm6ds3tr_c_pin_int1_route_set(&dev_ctx, int1_route);
        }
        return IMU_UpdateStatus(ret);
    }

    if ((pin == IMU_SENSOR_INT2) && (events == IMU_SENSOR_EVENT_NONE)) {
        ret = lsm6ds3tr_c_pin_int2_route_get(&dev_ctx, &int2_route);
        if (ret == 0) {
            IMU_MergeInt2Route(&int2_route,
                               (uint16_t)(IMU_SENSOR_EVENT_ACCEL_DRDY | IMU_SENSOR_EVENT_GYRO_DRDY |
                                          IMU_SENSOR_EVENT_TEMP_DRDY | IMU_SENSOR_EVENT_WAKE_UP),
                               0U);
            ret = lsm6ds3tr_c_pin_int2_route_set(&dev_ctx, int2_route);
        }
        return IMU_UpdateStatus(ret);
    }

    if (pin == IMU_SENSOR_INT1) {
        ret = lsm6ds3tr_c_pin_int1_route_get(&dev_ctx, &int1_route);
        if (ret == 0) {
            IMU_MergeInt1Route(&int1_route, events, 0U);
            ret = lsm6ds3tr_c_pin_int1_route_set(&dev_ctx, int1_route);
        }
    } else if (pin == IMU_SENSOR_INT2) {
        ret = lsm6ds3tr_c_pin_int2_route_get(&dev_ctx, &int2_route);
        if (ret == 0) {
            IMU_MergeInt2Route(&int2_route, events, 0U);
            ret = lsm6ds3tr_c_pin_int2_route_set(&dev_ctx, int2_route);
        }
    } else {
        ret = -1;
    }

    return IMU_UpdateStatus(ret);
}

int32_t imu_sensor_get_interrupt_source(uint16_t *events)
{
    int32_t ret                       = 0;
    uint16_t detected_events          = IMU_SENSOR_EVENT_NONE;
    lsm6ds3tr_c_all_sources_t sources = {0};

    if (events == NULL) {
        return IMU_UpdateStatus(-1);
    }

    ret = lsm6ds3tr_c_all_sources_get(&dev_ctx, &sources);
    if (ret != 0) {
        *events = IMU_SENSOR_EVENT_NONE;
        return IMU_UpdateStatus(ret);
    }

    if (sources.status_reg.xlda != 0U) {
        detected_events |= IMU_SENSOR_EVENT_ACCEL_DRDY;
    }
    if (sources.status_reg.gda != 0U) {
        detected_events |= IMU_SENSOR_EVENT_GYRO_DRDY;
    }
    if (sources.status_reg.tda != 0U) {
        detected_events |= IMU_SENSOR_EVENT_TEMP_DRDY;
    }
    if (sources.wake_up_src.wu_ia != 0U) {
        detected_events |= IMU_SENSOR_EVENT_WAKE_UP;
    }

    *events = detected_events;
    return IMU_UpdateStatus(0);
}

int32_t imu_sensor_read(void)
{
    imu_sensor_data_t data;

    g_imu_sensor_last_status = imu_sensor_read_data(&data);
    if (g_imu_sensor_last_status == 0) {
        g_imu_sensor_data = data;
    }

    return g_imu_sensor_last_status;
}

int32_t imu_sensor_read_raw(void)
{
    imu_sensor_raw_data_t data;

    g_imu_sensor_last_status = imu_sensor_read_raw_data(&data);
    if (g_imu_sensor_last_status == 0) {
        g_imu_sensor_data.raw = data;
        IMU_ConvertRawDataToUnitData(&g_imu_sensor_data.raw, &g_imu_sensor_data.unit);
    }

    return g_imu_sensor_last_status;
}

int32_t imu_sensor_read_raw_data(imu_sensor_raw_data_t *data)
{
    int32_t ret = 0;

    if (data == NULL) {
        return -1;
    }

    ret = lsm6ds3tr_c_xl_flag_data_ready_get(&dev_ctx, &data->accel_data_ready);
    if (ret != 0) {
        return ret;
    }

    ret = lsm6ds3tr_c_gy_flag_data_ready_get(&dev_ctx, &data->gyro_data_ready);
    if (ret != 0) {
        return ret;
    }

    ret = lsm6ds3tr_c_temp_flag_data_ready_get(&dev_ctx, &data->temperature_data_ready);
    if (ret != 0) {
        return ret;
    }

    ret = lsm6ds3tr_c_acceleration_raw_get(&dev_ctx, data->accel_raw);
    if (ret != 0) {
        return ret;
    }

    ret = lsm6ds3tr_c_angular_rate_raw_get(&dev_ctx, data->gyro_raw);
    if (ret != 0) {
        return ret;
    }
    ret = lsm6ds3tr_c_temperature_raw_get(&dev_ctx, &data->temperature_raw);
    if (ret != 0) {
        return ret;
    }

    return 0;
}

int32_t imu_sensor_read_unit_data(imu_sensor_unit_data_t *data)
{
    int32_t ret = 0;
    imu_sensor_raw_data_t raw_data;

    if (data == NULL) {
        return -1;
    }

    ret = imu_sensor_read_raw_data(&raw_data);
    if (ret != 0) {
        return ret;
    }

    IMU_ConvertRawDataToUnitData(&raw_data, data);
    return 0;
}

int32_t imu_sensor_read_data(imu_sensor_data_t *data)
{
    int32_t ret = 0;

    if (data == NULL) {
        return -1;
    }

    ret = imu_sensor_read_raw_data(&data->raw);
    if (ret != 0) {
        return ret;
    }

    IMU_ConvertRawDataToUnitData(&data->raw, &data->unit);
    return 0;
}

int32_t imu_sensor_read_raw_temperature(int16_t *temperature_raw)
{
    int16_t raw = 0;
    int32_t ret = 0;

    if (temperature_raw == NULL) {
        return -1;
    }

    ret = lsm6ds3tr_c_temperature_raw_get(&dev_ctx, &raw);
    if (ret != 0) {
        return ret;
    }

    *temperature_raw = raw;
    return 0;
}

int32_t imu_sensor_read_unit_temperature(float *temperature_c)
{
    int16_t raw = 0;
    int32_t ret = 0;

    if (temperature_c == NULL) {
        return -1;
    }

    ret = imu_sensor_read_raw_temperature(&raw);
    if (ret != 0) {
        return ret;
    }

    *temperature_c = IMU_ConvertTemperatureRawToCelsius(raw);
    return 0;
}

int32_t imu_sensor_read_temperature(float *temperature_c)
{
    return imu_sensor_read_unit_temperature(temperature_c);
}

int32_t imu_sensor_read_temperature_data(int16_t *temperature_raw, float *temperature_c)
{
    int32_t ret = 0;

    if ((temperature_raw == NULL) && (temperature_c == NULL)) {
        return -1;
    }

    if (temperature_raw != NULL) {
        ret = imu_sensor_read_raw_temperature(temperature_raw);
        if (ret != 0) {
            return ret;
        }
    }

    if (temperature_c != NULL) {
        if (temperature_raw != NULL) {
            *temperature_c = IMU_ConvertTemperatureRawToCelsius(*temperature_raw);
        } else {
            ret = imu_sensor_read_unit_temperature(temperature_c);
            if (ret != 0) {
                return ret;
            }
        }
    }

    return 0;
}

int32_t imu_data_update(void)
{
    int32_t ret = imu_sensor_read();

    if (ret != 0) {
        return ret;
    }

    imu_temperature_c_x100   = IMU_ConvertTemperatureCelsiusToX100(g_imu_sensor_data.unit.temperature_c);
    g_imu_sensor_last_status = 0;

    return 0;
}
