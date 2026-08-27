/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
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
    BUTTON_SINGLE_MODE = 0,
    BUTTON_SWITCH_MODE = 1,
} button_work_mode_t;

typedef enum {
    BUTTON_NONE = 0,
    BUTTON_SINGLE_CLICK,
    BUTTON_DOUBLE_CLICK,
    BUTTON_LONG_PRESS,
    BUTTON_CONFIG_ENTER,
    BUTTON_CONFIG_EXIT,
} button_event_t;

typedef enum {
    BUTTON_ACTIVE_LOW  = 0,
    BUTTON_ACTIVE_HIGH = 1,
} button_active_level_t;

typedef struct {
    uint16_t double_click_time;
    uint16_t long_press_time;
    button_active_level_t active_level;
    uint16_t debounce_time_ms;
} button_config_t;

// 按键对象结构体
typedef struct {
    GPIO_TypeDef* gpio_port;
    uint16_t gpio_pin;
    button_config_t config;

    uint8_t is_pressed;  // 稳态“按下” 1, 未按下0
    uint8_t prev_pressed;
    uint8_t stable_count;
    uint8_t click_count;
    bool long_press_triggered;
    bool paused;

    uint32_t press_time;    // 按下系统tick
    uint32_t release_time;  // 松开系统tick

    // 用户回调
    void (*single_click_callback)(void);
    void (*double_click_callback)(void);
    void (*long_press_callback)(void);

    uint8_t in_config_mode;
    button_work_mode_t work_mode;
    void (*mode_change_callback)(void);
} button_t;

void button_init(button_t* btn, GPIO_TypeDef* port, uint16_t pin, const button_config_t* pconfig);
void button_set_config(button_t* btn, const button_config_t* pconfig);
void button_set_callbacks(button_t* btn, void (*single_click_cb)(void), void (*double_click_cb)(void),
                          void (*long_press_cb)(void), void (*mode_change_cb)(void));
void button_pause(button_t* btn);
void button_resume(button_t* btn);
void button_reset_state(button_t* btn);
uint8_t button_read_pin(button_t* btn);
button_event_t button_scan(button_t* btn);

#ifdef __cplusplus
}
#endif
#endif
