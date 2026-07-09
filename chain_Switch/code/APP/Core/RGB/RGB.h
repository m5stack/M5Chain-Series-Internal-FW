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

#define RGB_NUM   (1)  // Macro definition for the number of LEDs, indicating there is 1 LED
#define RGB_INDEX (0)  // Macro definition for the index of LEDs, indicating RGB index start at 0

#define CODE_1 (52)  // Timer count for 1 code, used for controlling timing behavior for '1'
#define CODE_0 (25)  // Timer count for 0 code, used for controlling timing behavior for '0'

#define RGB_FADE_FRAMES  100  // 共100帧
#define RGB_FADE_MS_STEP 10   // 每帧10ms
#define RGB_FADE_MAX     255

extern __IO uint8_t
    g_light;  // RGB brightness, an externally accessible variable representing the current brightness level

/* Structure to define the size of RGB color components for a single LED */
typedef struct {
    uint8_t R;        // Red component, range 0-255
    uint8_t G;        // Green component, range 0-255
    uint8_t B;        // Blue component, range 0-255
} RGB_Color_TypeDef;  // RGB_Color_TypeDef is used to store the RGB color values of the LED

typedef enum {
    RGB_OPERATION_FAIL = 0x00,  // Operation failed, indicating that the RGB operation was not successful
    RGB_OPERATION_SUCCESS       // Operation successful, indicating that the RGB operation was successful
} rgb_operation_t;              // rgb_operation_t is used to indicate the status of the operation

typedef enum {
    CHAIN_SET_RGB_VALUE = 0x20,  // Command to set RGB values
    CHAIN_GET_RGB_VALUE = 0x21,  // Command to get RGB values
    CHAIN_SET_RGB_LIGHT = 0x22,  // Command to set the brightness of the RGB light
    CHAIN_GET_RGB_LIGHT = 0x23,  // Command to get the brightness of the RGB light
} chain_rgb_cmd_t;               // chain_rgb_cmd_t defines various command types

void rgb_init(void);  // Function to initialize RGB settings, setting the initial state of the RGB LED
// void send_rgb_value_to_switch(uint8_t rgb_index, RGB_Color_TypeDef color);		// Function to set RGB LED color to
// switch
void chain_set_rgb_value(uint8_t *buffer,
                         uint16_t size);  // Function to set RGB values, accepting an array of color values and its size
void chain_get_rgb_value(uint8_t *buffer, uint16_t size);  // Function to retrieve the current RGB values
void chain_set_light_value(
    uint8_t g_light_value,
    uint8_t flag);                 // Function to set the brightness of the RGB light, accepting a brightness value
void chain_get_light_value(void);  // Function to retrieve the current brightness level of the RGB light
void rgb_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __RGB_H__ */
