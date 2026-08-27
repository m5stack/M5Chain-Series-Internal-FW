/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
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
#include "user_pedal.h"

#define RGB_NUM (4)

#define CODE_1 (52)

#define CODE_0 (25)

#define RGB_FADE_FRAMES (100)

#define RGB_FADE_MS_STEP (5)

#define RGB_FADE_MAX (255)

typedef enum {
    RGB_TYPE_NONE                  = 0,
    RGB_TYPE_BLUE_FADE             = 3,
    RGB_TYPE_SWITCH_INDICATOR      = 4,
    RGB_TYPE_CONFIG_MODE           = 5,
    RGB_TYPE_SINGLE_MODE_INDICATOR = 6,
    RGB_TYPE_SWITCH_MODE_INDICATOR = 7
} rgb_fade_type_t;

#define IS_VALID_RANGE(start, count) ((start) < RGB_NUM && (count) > 0 && ((start) + (count)) <= RGB_NUM)

extern __IO uint8_t g_light;

typedef struct {
    uint8_t R;  // Red component, range 0-255
    uint8_t G;  // Green comzponent, range 0-255
    uint8_t B;  // Blue component, range 0-255
} RGB_Color_TypeDef;

typedef enum {
    CHAIN_SET_RGB_VALUE = 0x20,  // Command to set RGB values
    CHAIN_GET_RGB_VALUE = 0x21,  // Command to get RGB values
    CHAIN_SET_RGB_LIGHT = 0x22,  // Command to set the brightness of the RGB light
    CHAIN_GET_RGB_LIGHT = 0x23,  // Command to get the brightness of the RGB light
} chain_rgb_cmd_t;

void rgb_init(void);
void set_overlay_color(uint8_t led_id, uint8_t r, uint8_t g, uint8_t b);
void chain_set_rgb_value(uint8_t *buffer, uint8_t size);
void chain_get_rgb_value(uint8_t *buffer, uint8_t size);
void chain_set_light_value(uint8_t *buffer, uint8_t size);
void chain_get_light_value(void);
void rgb_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __RGB_H__ */
