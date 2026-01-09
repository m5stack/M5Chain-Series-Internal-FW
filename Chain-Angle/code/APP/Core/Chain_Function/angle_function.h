/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ANGLE_FUNCTION_H_
#define ANGLE_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "base_function.h"
#include "myflash.h"

typedef enum {
    CHAIN_ANGLE_GET_12ADC = 0x30,  // CHAIN_ANGLE_GET_12ADC is a command to get the latest 12-bit ADC value
    CHAIN_ANGLE_GET_8ADC  = 0x31,  // CHAIN_ANGLE_GET_8ADC is a command to get the latest 8-bit mapped ADC value
    CHAIN_ANGLE_SET_CLOCKWISE_STATUS =
        0x32,  // CHAIN_ANGLE_SET_CLOCKWISE_STATUS is a command to set the clockwise direction status
    CHAIN_ANGLE_GET_CLOCKWISE_STATUS =
        0x33,         // CHAIN_ANGLE_GET_CLOCKWISE_STATUS is a command to get the current clockwise direction status
} chain_angle_cmd_t;  // chain_angle_cmd_t is the command type for angle measurement operations

typedef enum {
    CHAIN_ANGLE_CLOCKWISE_DEC =
        0x00,  // CHAIN_ANGLE_CLOCKWISE_DEC is a status indicating the clockwise direction is decreasing
    CHAIN_ANGLE_CLOCKWISE_INC =
        0x01,         // CHAIN_ANGLE_CLOCKWISE_INC is a status indicating the clockwise direction is increasing
} angle_clockwise_t;  // angle_clockwise_t is the clockwise status indicating direction changes

void adc_value_update(void);
void chain_angle_get_12value(void);  // chain_angle_get_12value is a function to retrieve the latest 12-bit ADC value
void chain_angle_get_8value(
    void);  // chain_angle_get_8value is a function to retrieve the latest 8-bit mapped ADC value

void chain_angle_set_clockwise(
    uint8_t sta, uint8_t flag);  // chain_angle_set_clockwise is a function to set the clockwise direction status
void chain_angle_get_clockwise(
    void);  // chain_angle_get_clockwise is a function to retrieve the current clockwise direction status

#ifdef __cplusplus
}
#endif

#endif /* ANGLE_FUNCTION_H_ */
