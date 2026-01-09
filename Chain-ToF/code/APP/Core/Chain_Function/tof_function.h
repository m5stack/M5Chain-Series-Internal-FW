/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TOF_FUNCTION_H_
#define TOF_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "base_function.h"
#include "myflash.h"

typedef enum {
    CHAIN_TOF_GET_DISTANCE              = 0x50,  // Command to get the distance measurement
    CHAIN_TOF_SET_MEASURE_TIME          = 0x51,  // Command to set the mode of the TOF sensor
    CHAIN_TOF_GET_MEASURE_TIME          = 0x52,  // Command to get the current mode of the TOF sensor
    CHAIN_TOF_SET_MEASURE_MODE          = 0x53,  // Command to set the measurement timing budget
    CHAIN_TOF_GET_MEASURE_MODE          = 0x54,  // Command to get the current measurement timing budget
    CHAIN_TOF_SET_MEASURE_STATUS        = 0x55,  // Command to start measurement
    CHAIN_TOF_GET_MEASURE_STATUS        = 0x56,  // Command to get the measurement status
    CHAIN_TOF_GET_MEASURE_COMPLETE_FLAG = 0x57,
} chain_tof_cmd_t;

typedef enum {
    CHAIN_TOF_MODE_STOP       = 0,  // 停止模式
    CHAIN_TOF_MODE_SINGLE     = 1,  // 单次模式
    CHAIN_TOF_MODE_CONTINUOUS = 2   // 连续模式
} chain_tof_mode_t;

void chain_tof_init(void);
void chain_tof_update_distance(void);
void chain_tof_get_distance(void);
void chain_tof_set_measurement_time(uint8_t time);
void chain_tof_get_measurement_time(void);
void chain_tof_set_measurement_mode(uint8_t mode);
void chain_tof_get_measurement_mode(void);
void chain_tof_set_measurement_status(uint8_t status);
void chain_tof_get_measurement_status(void);
void chain_tof_get_measurement_complete_flag(void);

#ifdef __cplusplus
}
#endif

#endif /* TOF_FUNCTION_H_ */
