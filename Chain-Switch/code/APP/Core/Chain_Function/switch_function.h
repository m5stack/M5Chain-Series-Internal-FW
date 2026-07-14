/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SWITCH_FUNCTION_H_
#define _SWTICH_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "base_function.h"
#include "myflash.h"

#define CHAIN_SWITCH_REPORT_TYPE (0x04)

extern __IO uint32_t g_threshold;
extern __IO uint8_t
    g_slip_mode_status;  // External variable to store the current direction status (clockwise or counter-clockwise)
extern __IO uint8_t g_switch_status;  // External variable to store the current switch slip status (open or close)

extern RGB_Color_TypeDef USER_COLOR;
typedef enum {
    CHAIN_SWITCH_GET_12ADC            = 0x30,  // Get 12-bit ADC value
    CHAIN_SWITCH_GET_8ADC             = 0x31,  // Get 8-bit ADC value
    CHAIN_SWITCH_SET_SLIP_MODE        = 0x32,  // Set slip mode
    CHAIN_SWITCH_GET_SLIP_MODE        = 0x33,  // Get slip mode
    CHAIN_SWITCH_SET_SWITCH_THRESHOLD = 0x34,  // Set switch threshold
    CHAIN_SWITCH_GET_SWITCH_THRESHOLD = 0x35,  // Get switch threshold
    CHAIN_SWITCH_GET_SWITCH_STATUS    = 0x36,  // Get switch status
    CHAIN_SWITCH_REPORT_SWITCH_STATUS = 0xE0,  // Report switch status to uart when switch status change
    CHAIN_SWITCH_SET_REPORT_SWITCH_STATUS =
        0xE1,  // Control function: Report switch status to uart when switch status change
    CHAIN_SWITCH_GET_REPORT_SWITCH_STATUS = 0xE2,  // Acquire Control function Status
} chain_switch_cmd_t;                              // Fader command types

typedef enum {
    CHAIN_SWITCH_DOWNUP_DEC = 0x00,  // Clockwise decreasing
    CHAIN_SWITCH_DOWNUP_INC = 0x01,  // Clockwise increasing
} switch_work_mode_t;                // Clockwise status

typedef enum {
    OPEN   = 0,              // at the threshold of max ADC value
    CLOSE  = 1,              // at the threshold of min ADCvalue
    MIDDLE = 2,              // between max and min ADC value threshold
    NONE   = 3               // initial status
} switch_gradient_status_t;  // Gradient status

typedef enum {
    LED_GRADIENT_IDLE,
    LED_GRADIENT_FADING,
    LED_GRADIENT_HOLD_USER,
} LedGradientState;

typedef enum {
    COLOR_NONE,
    COLOR_GREEN,
    COLOR_RED,
} FadeColorType;

typedef struct {
    LedGradientState state;
    FadeColorType fadeColor;
    uint32_t startTime;
    uint8_t step;
} LedGradientCtx;

void adc_value_update(void);

void switch_status_update(void);

void set_rgb_slip_gradient(void);

void chain_switch_get_12value(void);  // Get 12-bit ADC value

void chain_switch_get_8value(void);  // Get 8-bit ADC value

void chain_switch_set_slip_mode(uint8_t sta, uint8_t flag);  // Set clockwise status

void chain_switch_get_slip_mode(void);  // Get clockwise status

void chain_switch_get_slip_status(void);  // Get switch slip status

void chain_switch_set_switch_threshold(uint8_t open_low_values, uint8_t open_high_values, uint8_t close_low_values,
                                       uint8_t close_high_values, uint8_t flag);  // Set switch threshold

void chain_switch_get_switch_threshold(void);  // Get switch threshold

void chain_switch_set_auto_send_status(uint8_t status);

void chain_switch_get_auto_send_status(void);

#ifdef __cplusplus
}
#endif

#endif /* SWTICH_FUNCTION_H_ */
