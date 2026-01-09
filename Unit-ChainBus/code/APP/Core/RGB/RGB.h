/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __RGB_H__
#define __RGB_H__

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdio.h"
#include "base_function.h"
#include "myflash.h"

#define RGB_NUM (1)  // Number of LEDs

extern __IO uint8_t g_light;  // RGB brightness

typedef enum {
    CHAIN_SET_RGB_VALUE = 0x20,  // Set RGB value
    CHAIN_GET_RGB_VALUE = 0x21,  // Get RGB value
    CHAIN_SET_RGB_LIGHT = 0x22,  // Set RGB brightness
    CHAIN_GET_RGB_LIGHT = 0x23,  // Obtain RGB brightness
} chain_rgb_cmd_t;               // RGB instruction type

void rgb_init(void);
void chain_set_rgb_value(uint8_t *buffer, uint8_t size);
void chain_get_rgb_value(uint8_t *buffer, uint8_t size);
void chain_set_light_value(uint8_t g_light_value, uint8_t flag);
void chain_get_light_value(void);
void rgb_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __RGB_H__ */
