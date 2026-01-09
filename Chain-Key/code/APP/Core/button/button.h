/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g0xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define BUTTON_DEBOUNCE_DEFAULT (20)

typedef enum {
    BUTTON_NONE = 0,
    BUTTON_SINGLE_CLICK,
    BUTTON_DOUBLE_CLICK,
    BUTTON_LONG_PRESS,
} button_event_t;

typedef enum {
    BUTTON_DOUBLE_CLICK_TIME_100MS  = 0x00,
    BUTTON_DOUBLE_CLICK_TIME_200MS  = 0x01,
    BUTTON_DOUBLE_CLICK_TIME_300MS  = 0x02,
    BUTTON_DOUBLE_CLICK_TIME_400MS  = 0x03,
    BUTTON_DOUBLE_CLICK_TIME_500MS  = 0x04,
    BUTTON_DOUBLE_CLICK_TIME_600MS  = 0x05,
    BUTTON_DOUBLE_CLICK_TIME_700MS  = 0x06,
    BUTTON_DOUBLE_CLICK_TIME_800MS  = 0x07,
    BUTTON_DOUBLE_CLICK_TIME_900MS  = 0x08,
    BUTTON_DOUBLE_CLICK_TIME_1000MS = 0x09,
} button_double_click_time_t;

typedef enum {
    BUTTON_LONG_PRESS_TIME_3S  = 0x00,
    BUTTON_LONG_PRESS_TIME_4S  = 0x01,
    BUTTON_LONG_PRESS_TIME_5S  = 0x02,
    BUTTON_LONG_PRESS_TIME_6S  = 0x03,
    BUTTON_LONG_PRESS_TIME_7S  = 0x04,
    BUTTON_LONG_PRESS_TIME_8S  = 0x05,
    BUTTON_LONG_PRESS_TIME_9S  = 0x06,
    BUTTON_LONG_PRESS_TIME_10S = 0x07,
} button_long_press_time_t;

typedef enum { BUTTON_ACTIVE_LOW = 0, BUTTON_ACTIVE_HIGH = 1 } button_active_level_t;

typedef struct {
    button_double_click_time_t double_click_level;
    button_long_press_time_t long_press_level;
    button_active_level_t active_level;
    uint16_t debounce_time_ms;
} button_config_t;

typedef struct {
    GPIO_TypeDef* gpio_port;
    uint16_t gpio_pin;
    button_config_t config;

    uint8_t is_pressed;
    uint8_t prev_pressed;
    uint8_t stable_count;
    uint8_t click_count;
    bool long_press_triggered;
    bool paused;

    uint32_t press_time;
    uint32_t release_time;

    void (*single_click_callback)(void);
    void (*double_click_callback)(void);
    void (*long_press_callback)(void);
} button_t;

void button_init(button_t* btn, GPIO_TypeDef* port, uint16_t pin, const button_config_t* pconfig);
void button_set_config(button_t* btn, const button_config_t* pconfig);
void button_set_callbacks(button_t* btn, void (*single_click_cb)(void), void (*double_click_cb)(void),
                          void (*long_press_cb)(void));
void button_pause(button_t* btn);
void button_resume(button_t* btn);
void button_reset_state(button_t* btn);
uint8_t button_read_pin(button_t* btn);
button_event_t button_scan(button_t* btn);

#ifdef __cplusplus
}
#endif
#endif
