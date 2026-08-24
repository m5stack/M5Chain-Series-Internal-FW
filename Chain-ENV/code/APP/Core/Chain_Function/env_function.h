/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ENV_FUNCTION_H_
#define ENV_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "base_function.h"
#include "myflash.h"
#include "driver_aht20_basic.h"
#include "spa06_sensor.h"

#define CHAIN_ENV_REPORT_TYPE (0x0F)  // Environment data report type

typedef enum {
    CHAIN_ENV_I2C_MEM_READ                 = 0x13,  // I2C transparent register read
    CHAIN_ENV_I2C_MEM_WRITE                = 0x14,  // I2C transparent register write
    CHAIN_ENV_GET_AHT20_DATA               = 0x30,
    CHAIN_ENV_GET_SPA06_DATA               = 0x31,
    CHAIN_ENV_SET_SPA06_TEMPERATURE_RATE   = 0x32,
    CHAIN_ENV_GET_SPA06_TEMPERATURE_RATE   = 0x33,
    CHAIN_ENV_SET_SPA06_PRESSURE_RATE      = 0x34,
    CHAIN_ENV_GET_SPA06_PRESSURE_RATE      = 0x35,
    CHAIN_ENV_SET_SPA06_SEA_LEVEL_PRESSURE = 0x36,
    CHAIN_ENV_GET_SPA06_SEA_LEVEL_PRESSURE = 0x37,
    CHAIN_ENV_ALARM_EVENT                  = 0xE0,
    CHAIN_ENV_SET_ALARM_MODE               = 0xE1,
    CHAIN_ENV_GET_ALARM_MODE               = 0xE2,
    CHAIN_ENV_SET_ALARM_TEMP_THRESHOLD     = 0xE3,
    CHAIN_ENV_GET_ALARM_TEMP_THRESHOLD     = 0xE4,
    CHAIN_ENV_SET_ALARM_HUMIDITY_THRESHOLD = 0xE5,
    CHAIN_ENV_GET_ALARM_HUMIDITY_THRESHOLD = 0xE6,
    CHAIN_ENV_SET_ALARM_PRESSURE_THRESHOLD = 0xE7,
    CHAIN_ENV_GET_ALARM_PRESSURE_THRESHOLD = 0xE8,
    CHAIN_ENV_SET_ALARM_ALTITUDE_THRESHOLD = 0xE9,
    CHAIN_ENV_GET_ALARM_ALTITUDE_THRESHOLD = 0xEA,
} chain_env_cmd_t;  // chain_env_cmd_t is the command type for environment operations

typedef enum {
    CHAIN_ENV_TEMP_DEVICE_AHT20 = 0,
    CHAIN_ENV_TEMP_DEVICE_SPA06 = 1,
} chain_env_temp_device_t;

void chain_env_i2c_mem_read(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t read_len);
void chain_env_i2c_mem_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t write_len,
                             uint8_t *buffer);

void chain_env_get_aht20_data_handle(void);
void chain_env_get_spa06_data_handle(void);
void chain_env_set_spa06_temperature_rate_handle(uint8_t *buffer, uint8_t size);
void chain_env_get_spa06_temperature_rate_handle(void);
void chain_env_set_spa06_pressure_rate_handle(uint8_t *buffer, uint8_t size);
void chain_env_get_spa06_pressure_rate_handle(void);
void chain_env_set_spa06_sea_level_pressure_handle(uint8_t *buffer, uint8_t size);
void chain_env_get_spa06_sea_level_pressure_handle(void);
void chain_env_set_alarm_mode_handle(uint8_t *buffer, uint8_t size);
void chain_env_get_alarm_mode_handle(void);
void chain_env_set_alarm_temperature_threshold_handle(uint8_t *buffer, uint8_t size);
void chain_env_get_alarm_temperature_threshold_handle(void);
void chain_env_set_alarm_humidity_threshold_handle(uint8_t *buffer, uint8_t size);
void chain_env_get_alarm_humidity_threshold_handle(void);
void chain_env_set_alarm_pressure_threshold_handle(uint8_t *buffer, uint8_t size);
void chain_env_get_alarm_pressure_threshold_handle(void);
void chain_env_set_alarm_altitude_threshold_handle(uint8_t *buffer, uint8_t size);
void chain_env_get_alarm_altitude_threshold_handle(void);
void chain_env_alarm_update(void);

#ifdef __cplusplus
}
#endif

#endif /* ENV_FUNCTION_H_ */
