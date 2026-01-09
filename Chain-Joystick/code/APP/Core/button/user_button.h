/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _USER_BUTTON_H_
#define _USER_BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "button.h"
#include "main.h"
#include "base_function.h"
#include "RGB.h"

#define KEY_EVENT_SINGLE_CLICK (0x0000)
#define KEY_EVENT_DOUBLE_CLICK (0x0001)
#define KEY_EVENT_LONG_PRESS   (0x0002)

typedef enum {
    CHAIN_BUTTON_TRIGGER_EVENT       = 0xE0,
    CHAIN_BUTTON_GET_STATUS          = 0xE1,
    CHAIN_BUTTON_SET_TRIGGER_TIMEOUT = 0xE2,
    CHAIN_BUTTON_GET_TRIGGER_TIMEOUT = 0xE3,
    CHAIN_BUTTON_SET_MODE            = 0xE4,
    CHAIN_BUTTON_GET_MODE            = 0xE5,
} chain_button_cmd_t;

void user_button_init(void);
void user_button_scan(void);
void user_get_button_status(void);
void user_set_button_mode(uint8_t mode);
void user_get_button_mode(void);
void user_set_button_trigger_timeout(uint8_t *buffer, uint8_t len);
void user_get_button_trigger_timeout(void);

#ifdef __cplusplus
}
#endif
#endif
