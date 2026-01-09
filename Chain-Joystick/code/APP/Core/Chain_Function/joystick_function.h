/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef JOYSTICK_FUNCTION_H_
#define JOYSTICK_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "base_function.h"
#include "myflash.h"
#include "usart.h"

// Default values for mapping range
// #define MAP_RANGE_DEFAULT {5000, 32000, 36000, 64500, 5000, 30000, 34000, 58500}  // Default mapping range values

// Debounce delay in milliseconds
#define DEBOUNCE_TIME_MS (150)  // 50ms debounce time

// Number of ADC channels
#define ADC_CHANNEL_NUMS (2)  // Total number of ADC channels

// Debug mode flag: 0 for disabled, 1 for enabled
#define DEBUG (0)  // Debug mode indicator

// Command enumeration for joystick operations
typedef enum {
    CHAIN_JOYSTICK_GET_16ADC                     = 0x30,  // Command to get 16-bit ADC values
    CHAIN_JOYSTICK_GET_8ADC                      = 0x31,  // Command to get 8-bit ADC values
    CHAIN_JOYSTICK_GET_ADC_XY_MAPPED_RANGE       = 0x32,  // Command to get mapped range for X and Y axes
    CHAIN_JOYSTICK_SET_ADC_XY_MAPPED_RANGE       = 0x33,  // Command to set mapped range for X and Y axes
    CHAIN_JOYSTICK_GET_ADC_XY_MAPPED_INT16_VALUE = 0x34,  // Command to get 16-bit mapped values for X and Y
    CHAIN_JOYSTICK_GET_ADC_XY_MAPPED_INT8_VALUE  = 0x35,  // Command to get 8-bit mapped values for X and Y
    CHAIN_JOYSTICK_BUTTON_PRESS                  = 0xE0,  // Command for joystick button press event
} chain_joystick_cmd_t;                                   // Enumeration for joystick commands

// Structure for ADC calibration parameters
typedef struct {
    uint16_t x_negative_min_adc_16bits;  // Minimum 16-bit ADC value for negative X
                                         // direction
    uint16_t x_negative_max_adc_16bits;  // Maximum 16-bit ADC value for negative X
                                         // direction
    uint16_t x_positive_min_adc_16bits;  // Minimum 16-bit ADC value for positive X
                                         // direction
    uint16_t x_positive_max_adc_16bits;  // Maximum 16-bit ADC value for positive X
                                         // direction
    uint16_t y_negative_min_adc_16bits;  // Minimum 16-bit ADC value for negative Y
                                         // direction
    uint16_t y_negative_max_adc_16bits;  // Maximum 16-bit ADC value for negative Y
                                         // direction
    uint16_t y_positive_min_adc_16bits;  // Minimum 16-bit ADC value for positive Y
                                         // direction
    uint16_t y_positive_max_adc_16bits;  // Maximum 16-bit ADC value for positive Y
                                         // direction
} adc_cal_t;                             // Structure for ADC calibration parameters

typedef enum {
    AXIS_X = 0x00,  // X-axis identifier
    AXIS_Y = 0x01,  // Y-axis identifier
} axis_t;           // Axis identifiers

// Function declarations
void joystick_init(void);                                                     // Initialize the joystick
void update_adc_cal_value(void);                                              // Update ADC calibration values
void chain_joystick_get_16adc(void);                                          // Get 16-bit ADC values from joystick
void chain_joystick_get_8adc(void);                                           // Get 8-bit ADC values from joystick
void chain_joystick_get_adc_xy_mapped_range(void);                            // Get mapped range for X and Y axes
void chain_joystick_set_adc_xy_mapped_range(uint8_t *buffer, uint16_t size);  // Set mapped range for X and Y axes
void chain_joystick_get_adc_xy_mapped_int16_value(void);                      // Get 16-bit mapped values for X and Y
void chain_joystick_get_adc_xy_mapped_int8_value(void);                       // Get 8-bit mapped values for X and Y

#ifdef __cplusplus
}
#endif

#endif /* JOYSTICK_FUNCTION_H_ */
