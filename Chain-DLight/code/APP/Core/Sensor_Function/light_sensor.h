#ifndef _LIGHT_SENSOR_H_
#define _LIGHT_SENSOR_H_

#include <stdint.h>
#include "i2c.h"

#define LIGHT_SENSOR_I2C_ADDR 0xA6

/* Register addresses */
#define LIGHT_SENSOR_REG_MAIN_CTRL       0x00
#define LIGHT_SENSOR_REG_ALS_MEAS_RATE   0x04
#define LIGHT_SENSOR_REG_ALS_GAIN        0x05
#define LIGHT_SENSOR_REG_PART_ID         0x06
#define LIGHT_SENSOR_REG_MAIN_STATUS     0x07
#define LIGHT_SENSOR_REG_ALS_DATA_0      0x0D
#define LIGHT_SENSOR_REG_ALS_DATA_1      0x0E
#define LIGHT_SENSOR_REG_ALS_DATA_2      0x0F
#define LIGHT_SENSOR_REG_INT_CFG         0x19
#define LIGHT_SENSOR_REG_INT_PST         0x1A
#define LIGHT_SENSOR_REG_ALS_THRES_UP_0  0x21
#define LIGHT_SENSOR_REG_ALS_THRES_UP_1  0x22
#define LIGHT_SENSOR_REG_ALS_THRES_UP_2  0x23
#define LIGHT_SENSOR_REG_ALS_THRES_LOW_0 0x24
#define LIGHT_SENSOR_REG_ALS_THRES_LOW_1 0x25
#define LIGHT_SENSOR_REG_ALS_THRES_LOW_2 0x26

/* MAIN_CTRL bits */
#define LIGHT_SENSOR_MAIN_CTRL_SW_RESET 0x10
#define LIGHT_SENSOR_MAIN_CTRL_ALS_EN   0x02

/* ALS_GAIN bits */
#define LIGHT_SENSOR_ALS_GAIN_MASK 0x07
#define LIGHT_SENSOR_ALS_GAIN_1X   0x00
#define LIGHT_SENSOR_ALS_GAIN_3X   0x01
#define LIGHT_SENSOR_ALS_GAIN_6X   0x02
#define LIGHT_SENSOR_ALS_GAIN_9X   0x03
#define LIGHT_SENSOR_ALS_GAIN_18X  0x04

#define LIGHT_SENSOR_GAIN_VALUE_1X  1U
#define LIGHT_SENSOR_GAIN_VALUE_3X  3U
#define LIGHT_SENSOR_GAIN_VALUE_6X  6U
#define LIGHT_SENSOR_GAIN_VALUE_9X  9U
#define LIGHT_SENSOR_GAIN_VALUE_18X 18U

/* MAIN_STATUS bits */
#define LIGHT_SENSOR_MAIN_STATUS_POWER_ON 0x20
#define LIGHT_SENSOR_MAIN_STATUS_ALS_INT  0x10
#define LIGHT_SENSOR_MAIN_STATUS_ALS_DATA 0x08

/* INT_CFG bits */
#define LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_MASK     0x30
#define LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_RESERVED 0x00
#define LIGHT_SENSOR_INT_CFG_ALS_INT_SEL_ALS      0x10
#define LIGHT_SENSOR_INT_CFG_ALS_INT_PIN_EN       0x04

/* INT_PST bits */
#define LIGHT_SENSOR_INT_PST_ALS_PERSIST_MASK 0xF0

/* ALS_DATA_2 bits */
#define LIGHT_SENSOR_ALS_DATA_2_MASK 0x0F

/* ALS_THRES bits */
#define LIGHT_SENSOR_ALS_THRES_HIGH_MASK 0x0F

#define LIGHT_SENSOR_INT_FACTOR_25MS  0.25f
#define LIGHT_SENSOR_INT_FACTOR_50MS  0.5f
#define LIGHT_SENSOR_INT_FACTOR_100MS 1.0f
#define LIGHT_SENSOR_INT_FACTOR_200MS 2.0f
#define LIGHT_SENSOR_INT_FACTOR_400MS 4.0f
#define LIGHT_SENSOR_LUX_MAX          2516580U

/* ALS_MEAS_RATE bits */
#define LIGHT_SENSOR_ALS_RES_MASK  0x70
#define LIGHT_SENSOR_ALS_RES_20BIT 0x00
#define LIGHT_SENSOR_ALS_RES_19BIT 0x10
#define LIGHT_SENSOR_ALS_RES_18BIT 0x20
#define LIGHT_SENSOR_ALS_RES_17BIT 0x30
#define LIGHT_SENSOR_ALS_RES_16BIT 0x40

#define LIGHT_SENSOR_ALS_RATE_MASK   0x07
#define LIGHT_SENSOR_ALS_RATE_25MS   0x00
#define LIGHT_SENSOR_ALS_RATE_50MS   0x01
#define LIGHT_SENSOR_ALS_RATE_100MS  0x02
#define LIGHT_SENSOR_ALS_RATE_500MS  0x03
#define LIGHT_SENSOR_ALS_RATE_1000MS 0x05
#define LIGHT_SENSOR_ALS_RATE_2000MS 0x06

HAL_StatusTypeDef LightSensor_I2cMemRead(uint8_t i2c_addr, uint16_t reg_addr, uint16_t reg_addr_size, uint8_t *buf,
                                         uint16_t len);
HAL_StatusTypeDef LightSensor_I2cMemWrite(uint8_t i2c_addr, uint16_t reg_addr, uint16_t reg_addr_size, uint8_t *buf,
                                          uint16_t len);
HAL_StatusTypeDef LightSensor_SwReset(void);
HAL_StatusTypeDef LightSensor_SetAlsEnable(uint8_t enable);
HAL_StatusTypeDef LightSensor_SetAlsGain(uint8_t gain);
HAL_StatusTypeDef LightSensor_GetAlsGain(uint8_t *gain);
HAL_StatusTypeDef LightSensor_GetPowerOnStatus(uint8_t *status);
HAL_StatusTypeDef LightSensor_GetAlsStatus(uint8_t *als_int, uint8_t *als_data);
HAL_StatusTypeDef LightSensor_SetAlsInterruptEnable(uint8_t enable);
HAL_StatusTypeDef LightSensor_SetAlsInterruptSelect(uint8_t select);
HAL_StatusTypeDef LightSensor_SetAlsInterruptEnableAndSelect(uint8_t enable, uint8_t select);
HAL_StatusTypeDef LightSensor_SetAlsInterruptPersist(uint8_t persist);
HAL_StatusTypeDef LightSensor_GetAlsInterruptPersist(uint8_t *persist);
HAL_StatusTypeDef LightSensor_GetAlsInterruptConfig(uint8_t *enable, uint8_t *select);
HAL_StatusTypeDef LightSensor_ConfigAlsInterrupt(uint8_t enable, uint8_t select);
HAL_StatusTypeDef LightSensor_ReadAlsData(uint32_t *als_data);
HAL_StatusTypeDef LightSensor_GetLux(uint32_t *lux);
HAL_StatusTypeDef LightSensor_SetAlsResolution(uint8_t resolution);
HAL_StatusTypeDef LightSensor_SetAlsMeasurementRate(uint8_t rate);
HAL_StatusTypeDef LightSensor_GetAlsResolution(uint8_t *resolution);
HAL_StatusTypeDef LightSensor_GetAlsMeasurementRate(uint8_t *rate);
HAL_StatusTypeDef LightSensor_SetAlsThresholds(uint32_t low_thres, uint32_t high_thres);
HAL_StatusTypeDef LightSensor_GetAlsThresholds(uint32_t *low_thres, uint32_t *high_thres);

#endif
