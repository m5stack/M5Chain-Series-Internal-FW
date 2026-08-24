/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef IMU_FUNCTION_H_
#define IMU_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "base_function.h"
#include "myflash.h"
#include "imu_sensor.h"
#include "RGB.h"

#define CHAIN_IMU_REPORT_TYPE                  (0x10)
#define CHAIN_IMU_EVENT_MODE_ACCEL             (0x01U)
#define CHAIN_IMU_EVENT_MODE_GYRO              (0x02U)
#define CHAIN_IMU_EVENT_MODE_TEMP              (0x04U)
#define CHAIN_IMU_INT_STATUS_ACCEL             (0x01U)
#define CHAIN_IMU_INT_STATUS_GYRO              (0x02U)
#define CHAIN_IMU_INT_STATUS_TEMP_MASK         (0x0CU)
#define CHAIN_IMU_INT_STATUS_TEMP_HIGH_RELEASE (0x00U)
#define CHAIN_IMU_INT_STATUS_TEMP_HIGH_ACTIVE  (0x04U)
#define CHAIN_IMU_INT_STATUS_TEMP_LOW_ACTIVE   (0x08U)
#define CHAIN_IMU_INT_STATUS_TEMP_LOW_RELEASE  (0x0CU)
#define CHAIN_IMU_INT_STATUS_TEMP_HIGH         CHAIN_IMU_INT_STATUS_TEMP_HIGH_ACTIVE
#define CHAIN_IMU_INT_STATUS_TEMP_LOW          CHAIN_IMU_INT_STATUS_TEMP_LOW_ACTIVE

extern volatile imu_sensor_data_t g_imu_sensor_data;
extern volatile uint16_t imu_temperature_c_x100;

typedef enum {
    CHAIN_IMU_I2C_MEM_READ         = 0x13,  // I2C transparent register read
    CHAIN_IMU_I2C_MEM_WRITE        = 0x14,  // I2C transparent register write
    CHAIN_IMU_GET_ACCEL_DATA       = 0x30,  // Get 3-axis int16_t acceleration data in mg, little endian
    CHAIN_IMU_GET_GYRO_DATA        = 0x31,  // Get 3-axis int16_t gyroscope data in dps, little endian
    CHAIN_IMU_GET_ACCEL_RAW_DATA   = 0x32,  // Get 3-axis int16_t acceleration raw data, little endian
    CHAIN_IMU_GET_GYRO_RAW_DATA    = 0x33,  // Get 3-axis int16_t gyroscope raw data, little endian
    CHAIN_IMU_GET_TEMPERATURE_DATA = 0x34,  // Get the latest IMU temperature data
    CHAIN_IMU_SET_ACCEL_RANGE      = 0x35,  // Set acceleration range
    CHAIN_IMU_GET_ACCEL_RANGE      = 0x36,  // Get acceleration range
    CHAIN_IMU_SET_GYRO_RANGE       = 0x37,  // Set gyroscope range
    CHAIN_IMU_GET_GYRO_RANGE       = 0x38,  // Get gyroscope range
    CHAIN_IMU_SET_ACCEL_ODR        = 0x39,  // Set acceleration output data rate
    CHAIN_IMU_GET_ACCEL_ODR        = 0x3A,  // Get acceleration output data rate
    CHAIN_IMU_SET_GYRO_ODR         = 0x3B,  // Set gyroscope output data rate
    CHAIN_IMU_GET_GYRO_ODR         = 0x3C,  // Get gyroscope output data rate
    CHAIN_IMU_EVENT_REPORT         = 0xE0,  // IMU event report
    CHAIN_IMU_SET_EVENT_MODE       = 0xE1,  // Set IMU event trigger mode
    CHAIN_IMU_GET_EVENT_MODE       = 0xE2,  // Get IMU event trigger mode
    CHAIN_IMU_SET_AXIS_THRESHOLD   = 0xE3,  // Set acceleration and gyroscope thresholds
    CHAIN_IMU_GET_AXIS_THRESHOLD   = 0xE4,  // Get acceleration and gyroscope thresholds
    CHAIN_IMU_SET_TEMP_THRESHOLD   = 0xE5,  // Set temperature threshold
    CHAIN_IMU_GET_TEMP_THRESHOLD   = 0xE6,  // Get temperature threshold
} chain_imu_cmd_t;                          // chain_imu_cmd_t is the command type for IMU measurement operations

void chain_imu_i2c_mem_read(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t read_len);
void chain_imu_i2c_mem_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t write_len,
                             uint8_t *buffer);
void chain_imu_get_accel_data_handle(void);
void chain_imu_get_gyro_data_handle(void);
void chain_imu_get_accel_raw_data_handle(void);
void chain_imu_get_gyro_raw_data_handle(void);
void chain_imu_get_temperature_data_handle(void);
void chain_imu_set_accel_range_handle(uint8_t accel_range);
void chain_imu_get_accel_range_handle(void);
void chain_imu_set_gyro_range_handle(uint8_t gyro_range);
void chain_imu_get_gyro_range_handle(void);
void chain_imu_set_accel_odr_handle(uint8_t accel_odr);
void chain_imu_get_accel_odr_handle(void);
void chain_imu_set_gyro_odr_handle(uint8_t gyro_odr);
void chain_imu_get_gyro_odr_handle(void);
void chain_imu_event_report_handle(uint8_t int_status);
void chain_imu_set_event_mode_handle(uint8_t mode);
void chain_imu_get_event_mode_handle(void);
void chain_imu_set_axis_threshold_handle(uint8_t accel_threshold, uint8_t gyro_threshold);
void chain_imu_get_axis_threshold_handle(void);
void chain_imu_set_temp_threshold_handle(uint8_t temp_threshold_low, uint8_t temp_threshold_high);
void chain_imu_get_temp_threshold_handle(void);
void chain_imu_event_process(void);

#ifdef __cplusplus
}
#endif

#endif /* IMU_FUNCTION_H_ */
