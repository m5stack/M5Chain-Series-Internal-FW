#include "../Sensor_Function/light_sensor.h"
#define LIGHT_SENSOR_I2C_TIMEOUT_MS 100U

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

static HAL_StatusTypeDef LTR308_WriteReg(uint8_t reg, uint8_t data)
{
    return LightSensor_I2cMemWrite(LIGHT_SENSOR_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &data, 1);
}

static HAL_StatusTypeDef LTR308_WriteRegMulti(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return LightSensor_I2cMemWrite(LIGHT_SENSOR_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len);
}

static HAL_StatusTypeDef LTR308_ReadReg(uint8_t reg, uint8_t *data)
{
    return LightSensor_I2cMemRead(LIGHT_SENSOR_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, 1);
}

static HAL_StatusTypeDef LTR308_ReadMulti(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return LightSensor_I2cMemRead(LIGHT_SENSOR_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len);
}

static uint8_t LightSensor_GainSettingToValue(uint8_t gain_setting, uint8_t *gain_value)
{
    switch (gain_setting & LIGHT_SENSOR_ALS_GAIN_MASK) {
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

static uint8_t LightSensor_ResolutionToIntFactor(uint8_t resolution, float *int_factor)
{
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

static uint8_t LightSensor_ReadGainSetting(uint8_t *gain_setting)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_ALS_GAIN, &reg);

    if (ret != HAL_OK) {
        return 0U;
    }

    *gain_setting = reg & LIGHT_SENSOR_ALS_GAIN_MASK;

    return 1U;
}

HAL_StatusTypeDef LightSensor_SwReset(void)
{
    return LTR308_WriteReg(LIGHT_SENSOR_REG_MAIN_CTRL, LIGHT_SENSOR_MAIN_CTRL_SW_RESET);
}

HAL_StatusTypeDef LightSensor_SetAlsEnable(uint8_t enable)
{
    uint8_t value = enable ? LIGHT_SENSOR_MAIN_CTRL_ALS_EN : 0x00;

    return LTR308_WriteReg(LIGHT_SENSOR_REG_MAIN_CTRL, value);
}

HAL_StatusTypeDef LightSensor_SetAlsGain(uint8_t gain)
{
    uint8_t value = gain & LIGHT_SENSOR_ALS_GAIN_MASK;

    return LTR308_WriteReg(LIGHT_SENSOR_REG_ALS_GAIN, value);
}

HAL_StatusTypeDef LightSensor_GetAlsGain(uint8_t *gain)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_ALS_GAIN, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    *gain = reg & LIGHT_SENSOR_ALS_GAIN_MASK;

    return HAL_OK;
}

HAL_StatusTypeDef LightSensor_GetPowerOnStatus(uint8_t *status)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_MAIN_STATUS, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    *status = (reg & LIGHT_SENSOR_MAIN_STATUS_POWER_ON) ? 1U : 0U;

    return HAL_OK;
}

HAL_StatusTypeDef LightSensor_GetAlsStatus(uint8_t *als_int, uint8_t *als_data)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_MAIN_STATUS, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    if (als_int != NULL) {
        *als_int = (reg & LIGHT_SENSOR_MAIN_STATUS_ALS_INT) ? 1U : 0U;
    }

    if (als_data != NULL) {
        *als_data = (reg & LIGHT_SENSOR_MAIN_STATUS_ALS_DATA) ? 1U : 0U;
    }

    return HAL_OK;
}

HAL_StatusTypeDef LightSensor_SetAlsInterruptEnable(uint8_t enable)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_INT_CFG, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    if (enable) {
        reg |= LIGHT_SENSOR_INT_CFG_ALS_INT_PIN_EN;
    } else {
        reg &= (uint8_t)~LIGHT_SENSOR_INT_CFG_ALS_INT_PIN_EN;
    }

    reg &= (LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_MASK | LIGHT_SENSOR_INT_CFG_ALS_INT_PIN_EN);

    return LTR308_WriteReg(LIGHT_SENSOR_REG_INT_CFG, reg);
}

HAL_StatusTypeDef LightSensor_SetAlsInterruptSelect(uint8_t select)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_INT_CFG, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    if (select > 0x01) {
        return HAL_ERROR;
    }

    reg &= (uint8_t)~LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_MASK;
    if (select) {
        reg |= LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_ALS;
    }

    reg &= (LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_MASK | LIGHT_SENSOR_INT_CFG_ALS_INT_PIN_EN);

    return LTR308_WriteReg(LIGHT_SENSOR_REG_INT_CFG, reg);
}

HAL_StatusTypeDef LightSensor_SetAlsInterruptEnableAndSelect(uint8_t enable, uint8_t select)
{
    uint8_t reg = 0;

    if (select > 0x01) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_INT_CFG, &reg);

    if (ret != HAL_OK) {
        return ret;
    }
    reg &= (uint8_t) ~(LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_MASK | LIGHT_SENSOR_INT_CFG_ALS_INT_PIN_EN);
    if (select) {
        reg |= LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_ALS;
    }

    if (enable) {
        reg |= LIGHT_SENSOR_INT_CFG_ALS_INT_PIN_EN;
    }

    reg &= (LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_MASK | LIGHT_SENSOR_INT_CFG_ALS_INT_PIN_EN);

    return LTR308_WriteReg(LIGHT_SENSOR_REG_INT_CFG, reg);
}

HAL_StatusTypeDef LightSensor_SetAlsInterruptPersist(uint8_t persist)
{
    uint8_t reg = (uint8_t)((persist << 4) & LIGHT_SENSOR_INT_PST_ALS_PERSIST_MASK);

    return LTR308_WriteReg(LIGHT_SENSOR_REG_INT_PST, reg);
}

HAL_StatusTypeDef LightSensor_GetAlsInterruptPersist(uint8_t *persist)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_INT_PST, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    *persist = (reg & LIGHT_SENSOR_INT_PST_ALS_PERSIST_MASK) >> 4;

    return HAL_OK;
}

HAL_StatusTypeDef LightSensor_GetAlsInterruptConfig(uint8_t *enable, uint8_t *select)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_INT_CFG, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    if (enable != NULL) {
        *enable = (reg & LIGHT_SENSOR_INT_CFG_ALS_INT_PIN_EN) ? 1U : 0U;
    }

    if (select != NULL) {
        *select = (reg & LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_MASK) ? 1U : 0U;
    }

    return HAL_OK;
}

HAL_StatusTypeDef LightSensor_ConfigAlsInterrupt(uint8_t enable, uint8_t select)
{
    uint8_t reg = select & LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_MASK;

    if (enable) {
        reg |= LIGHT_SENSOR_INT_CFG_ALS_INT_PIN_EN;
    }

    return LTR308_WriteReg(LIGHT_SENSOR_REG_INT_CFG, reg);
}

HAL_StatusTypeDef LightSensor_ReadAlsData(uint32_t *als_data)
{
    uint8_t buf[3]        = {0};
    HAL_StatusTypeDef ret = LTR308_ReadMulti(LIGHT_SENSOR_REG_ALS_DATA_0, buf, 3);

    if (ret != HAL_OK) {
        return ret;
    }

    *als_data = ((uint32_t)(buf[2] & LIGHT_SENSOR_ALS_DATA_2_MASK) << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[0];

    return HAL_OK;
}

HAL_StatusTypeDef LightSensor_SetAlsResolution(uint8_t resolution)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_ALS_MEAS_RATE, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    reg &= (uint8_t)~LIGHT_SENSOR_ALS_RES_MASK;
    reg |= (resolution & LIGHT_SENSOR_ALS_RES_MASK);
    reg &= (LIGHT_SENSOR_ALS_RES_MASK | LIGHT_SENSOR_ALS_RATE_MASK);

    return LTR308_WriteReg(LIGHT_SENSOR_REG_ALS_MEAS_RATE, reg);
}

HAL_StatusTypeDef LightSensor_SetAlsMeasurementRate(uint8_t rate)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_ALS_MEAS_RATE, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    reg &= (uint8_t)~LIGHT_SENSOR_ALS_RATE_MASK;
    reg |= (rate & LIGHT_SENSOR_ALS_RATE_MASK);
    reg &= (LIGHT_SENSOR_ALS_RES_MASK | LIGHT_SENSOR_ALS_RATE_MASK);

    return LTR308_WriteReg(LIGHT_SENSOR_REG_ALS_MEAS_RATE, reg);
}

HAL_StatusTypeDef LightSensor_GetAlsResolution(uint8_t *resolution)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_ALS_MEAS_RATE, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    *resolution = reg & LIGHT_SENSOR_ALS_RES_MASK;

    return HAL_OK;
}

HAL_StatusTypeDef LightSensor_GetAlsMeasurementRate(uint8_t *rate)
{
    uint8_t reg           = 0;
    HAL_StatusTypeDef ret = LTR308_ReadReg(LIGHT_SENSOR_REG_ALS_MEAS_RATE, &reg);

    if (ret != HAL_OK) {
        return ret;
    }

    *rate = reg & LIGHT_SENSOR_ALS_RATE_MASK;

    return HAL_OK;
}

HAL_StatusTypeDef LightSensor_SetAlsThresholds(uint32_t low_thres, uint32_t high_thres)
{
    if (low_thres > 0xFFFFFU || high_thres > 0xFFFFFU || low_thres > high_thres) {
        return HAL_ERROR;
    }

    uint8_t buf[6] = {0};

    buf[0] = (uint8_t)(high_thres & 0xFF);
    buf[1] = (uint8_t)((high_thres >> 8) & 0xFF);
    buf[2] = (uint8_t)((high_thres >> 16) & LIGHT_SENSOR_ALS_THRES_HIGH_MASK);
    buf[3] = (uint8_t)(low_thres & 0xFF);
    buf[4] = (uint8_t)((low_thres >> 8) & 0xFF);
    buf[5] = (uint8_t)((low_thres >> 16) & LIGHT_SENSOR_ALS_THRES_HIGH_MASK);

    return LTR308_WriteRegMulti(LIGHT_SENSOR_REG_ALS_THRES_UP_0, buf, 6);
}

HAL_StatusTypeDef LightSensor_GetAlsThresholds(uint32_t *low_thres, uint32_t *high_thres)
{
    uint8_t buf[6]        = {0};
    HAL_StatusTypeDef ret = LTR308_ReadMulti(LIGHT_SENSOR_REG_ALS_THRES_UP_0, buf, 6);

    if (ret != HAL_OK) {
        return ret;
    }

    if (high_thres != NULL) {
        *high_thres =
            ((uint32_t)(buf[2] & LIGHT_SENSOR_ALS_THRES_HIGH_MASK) << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[0];
    }

    if (low_thres != NULL) {
        *low_thres =
            ((uint32_t)(buf[5] & LIGHT_SENSOR_ALS_THRES_HIGH_MASK) << 16) | ((uint32_t)buf[4] << 8) | (uint32_t)buf[3];
    }

    return HAL_OK;
}

HAL_StatusTypeDef LightSensor_GetLux(uint32_t *lux)
{
    uint32_t als_data         = 0;
    uint8_t gain_value        = 0;
    uint8_t gain_setting      = 0;
    uint8_t resolution        = 0;
    float resolved_int_factor = 0.0f;
    float lux_value           = 0.0f;
    HAL_StatusTypeDef ret     = HAL_ERROR;

    if (lux == NULL) {
        return HAL_ERROR;
    }

    ret = LightSensor_ReadAlsData(&als_data);

    if (ret != HAL_OK) {
        return ret;
    }

    if (!LightSensor_ReadGainSetting(&gain_setting)) {
        return HAL_ERROR;
    }

    if (!LightSensor_GainSettingToValue(gain_setting, &gain_value)) {
        return HAL_ERROR;
    }

    ret = LightSensor_GetAlsResolution(&resolution);

    if (ret != HAL_OK) {
        return ret;
    }

    if (!LightSensor_ResolutionToIntFactor(resolution, &resolved_int_factor)) {
        return HAL_ERROR;
    }

    lux_value = (0.6f * (float)als_data / ((float)gain_value * resolved_int_factor)) + 0.5f;
    if (lux_value > (float)LIGHT_SENSOR_LUX_MAX) {
        *lux = LIGHT_SENSOR_LUX_MAX;
    } else {
        *lux = (uint32_t)lux_value;
    }

    return HAL_OK;
}
