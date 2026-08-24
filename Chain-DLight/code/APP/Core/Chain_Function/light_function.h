/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LIGHT_FUNCTION_H_
#define LIGHT_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "base_function.h"
#include "myflash.h"
#include "light_sensor.h"
#include "RGB.h"

#define CHAIN_DLIGHT_REPORT_TYPE (0x11)

typedef enum {
    CHAIN_DLIGHT_I2C_MEM_READ                = 0x13,  // I2C transparent register read
    CHAIN_DLIGHT_I2C_MEM_WRITE               = 0x14,  // I2C transparent register write
    CHAIN_DLIGHT_GET_LUX_DATA                = 0x30,  // Get lux data
    CHAIN_DLIGHT_SET_INTERRUPT_ENABLE        = 0x31,  // Set interrupt enable
    CHAIN_DLIGHT_GET_INTERRUPT_ENABLE        = 0x32,  // Get interrupt enable status
    CHAIN_DLIGHT_SET_INTERRUPT_THRESHOLD     = 0x33,  // Set interrupt threshold
    CHAIN_DLIGHT_GET_INTERRUPT_THRESHOLD     = 0x34,  // Get interrupt threshold
    CHAIN_DLIGHT_SET_MEASURE_GAIN            = 0x35,  // Set measurement gain
    CHAIN_DLIGHT_GET_MEASURE_GAIN            = 0x36,  // Get measurement gain
    CHAIN_DLIGHT_SET_MEASURE_RESOLUTION      = 0x37,  // Set measurement rate
    CHAIN_DLIGHT_GET_MEASURE_RESOLUTION      = 0x38,  // Get measurement rate
    CHAIN_DLIGHT_SET_MEASURE_RATE            = 0x39,  // Set measurement rate
    CHAIN_DLIGHT_GET_MEASURE_RATE            = 0x3A,  // Get measurement rate
    CHAIN_DLIGHT_INTERRUPT_REPORT            = 0xE0,  // Report interrupt event with data
    CHAIN_DLIGHT_SET_INTERRUPT_REPORT_ENABLE = 0xE1,  // Set whether to report interrupts
    CHAIN_DLIGHT_GET_INTERRUPT_REPORT_ENABLE = 0xE2,  // Get whether interrupt

} chain_dlight_cmd_t;  // chain_dlight_cmd_t is the command type for environment operations

void chain_dlight_init(void);
void handle_dlight_trigger_event(void);
void chain_dlight_poll_lux_data(void);
uint8_t chain_dlight_is_interrupt_enabled(void);
void chain_dlight_i2c_mem_read(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t read_len);
void chain_dlight_i2c_mem_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_addr_length, uint8_t write_len,
                                uint8_t *buffer);
void chain_dlight_get_lux_data_handle(void);
void chain_dlight_set_interrupt_enable(uint8_t *buffer, uint8_t size);
void chain_dlight_get_interrupt_enable(void);
void chain_dlight_set_interrupt_threshold(uint8_t *buffer, uint8_t size);

void chain_dlight_get_interrupt_threshold(void);
void chain_dlight_set_measure_gain(uint8_t *buffer, uint8_t size);
void chain_dlight_get_measure_gain(void);
void chain_dlight_set_measure_resolution(uint8_t *buffer, uint8_t size);
void chain_dlight_get_measure_resolution(void);
void chain_dlight_set_measure_rate(uint8_t *buffer, uint8_t size);
void chain_dlight_get_measure_rate(void);

void chain_dlight_set_report_enable(uint8_t *buffer, uint8_t size);
void chain_dlight_get_report_enable(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHT_FUNCTION_H_ */
