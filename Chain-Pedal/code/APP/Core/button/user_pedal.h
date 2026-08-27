/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _USER_PEDAL_H_
#define _USER_PEDAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "button.h"
#include "main.h"
#include "base_function.h"
#include "myflash.h"
#include "RGB.h"

#define PEDAL_EVENT_SINGLE_CLICK (0x0000)
#define PEDAL_STATUS_SWITCH_OFF  (0x0400)
#define PEDAL_STATUS_SWITCH_ON   (0x0401)

typedef enum {
    CHAIN_PEDAL_TRIGGER_EVENT         = 0xE0,
    CHAIN_PEDAL_GET_BUTTON_STATUS     = 0xE1,
    CHAIN_PEDAL_GET_SWITCH_STATUS     = 0xE2,
    CHAIN_PEDAL_SET_WORK_MODE         = 0xE3,
    CHAIN_PEDAL_GET_WORK_MODE         = 0xE4,
    CHAIN_PEDAL_SET_BUTTON_MODE       = 0xE5,
    CHAIN_PEDAL_GET_BUTTON_MODE       = 0xE6,
    CHAIN_PEDAL_SET_ENTER_CONFIG_TIME = 0xE7,
    CHAIN_PEDAL_GET_ENTER_CONFIG_TIME = 0xE8,
    CHAIN_PEDAL_SET_RGB_OVERLAY_VALUE = 0xE9,
    CHAIN_PEDAL_GET_RGB_OVERLAY_VALUE = 0xEA,
} chain_pedal_cmd_t;

void user_button_init(void);
void user_button_scan(void);
void user_get_pedal_button_status(void);
void user_get_pedal_switch_status(void);
void user_set_pedal_work_mode(uint8_t *buffer, uint8_t size);
void user_get_pedal_work_mode(void);
void user_set_pedal_button_mode(uint8_t *buffer, uint8_t size);
void user_get_pedal_button_mode(void);
void user_set_pedal_enter_config_time(uint8_t *buffer, uint8_t size);
void user_get_pedal_enter_config_time(void);
void user_set_pedal_rgb_overlay_value(uint8_t *buffer, uint8_t size);
void user_get_pedal_rgb_overlay_value(void);

#ifdef __cplusplus
}
#endif
#endif
