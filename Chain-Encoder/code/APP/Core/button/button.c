/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "button.h"

static const uint16_t button_double_click_time[] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
static const uint16_t button_long_press_time[]   = {3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};

static uint32_t button_get_tick(void)
{
    return HAL_GetTick();
}

uint8_t button_read_pin(button_t* btn)
{
    uint8_t logic = (HAL_GPIO_ReadPin(btn->gpio_port, btn->gpio_pin) == GPIO_PIN_SET) ? 1 : 0;
    return (btn->config.active_level == BUTTON_ACTIVE_HIGH) ? logic : !logic;
}

void button_init(button_t* btn, GPIO_TypeDef* port, uint16_t pin, const button_config_t* pconfig)
{
    btn->gpio_port = port;
    btn->gpio_pin  = pin;
    if (pconfig)
        btn->config = *pconfig;
    else {
        btn->config.double_click_level = BUTTON_DOUBLE_CLICK_TIME_200MS;
        btn->config.long_press_level   = BUTTON_LONG_PRESS_TIME_3S;
        btn->config.active_level       = BUTTON_ACTIVE_LOW;
        btn->config.debounce_time_ms   = BUTTON_DEBOUNCE_DEFAULT;
    }
    btn->is_pressed            = 0;
    btn->prev_pressed          = 0;
    btn->stable_count          = 0;
    btn->press_time            = 0;
    btn->release_time          = 0;
    btn->click_count           = 0;
    btn->long_press_triggered  = 0;
    btn->paused                = 0;
    btn->single_click_callback = NULL;
    btn->double_click_callback = NULL;
    btn->long_press_callback   = NULL;
}

// 更新配置
void button_set_config(button_t* btn, const button_config_t* cfg)
{
    if (cfg) {
        button_config_t tmp = *cfg;
        btn->config         = tmp;
        button_reset_state(btn);
    }
}
// 设置回调
void button_set_callbacks(button_t* btn, void (*single_click_cb)(void), void (*double_click_cb)(void),
                          void (*long_press_cb)(void))
{
    btn->single_click_callback = single_click_cb;
    btn->double_click_callback = double_click_cb;
    btn->long_press_callback   = long_press_cb;
}

void button_pause(button_t* btn)
{
    btn->paused = 1;
    button_reset_state(btn);
}

void button_resume(button_t* btn)
{
    btn->paused = 0;
    button_reset_state(btn);
}

void button_reset_state(button_t* btn)
{
    btn->is_pressed           = 0;
    btn->prev_pressed         = 0;
    btn->stable_count         = 0;
    btn->press_time           = 0;
    btn->release_time         = 0;
    btn->click_count          = 0;
    btn->long_press_triggered = 0;
}

button_event_t button_scan(button_t* btn)
{
    if (btn->paused) return BUTTON_NONE;

    static uint8_t debounced      = 0;
    static uint32_t debounce_tick = 0;
    static uint8_t last_raw       = 0xFF;

    uint32_t now = button_get_tick();
    uint8_t raw  = button_read_pin(btn);

    if (raw != debounced) {
        if (raw != last_raw) {
            debounce_tick = now;
            last_raw      = raw;
        }
        if ((now - debounce_tick) >= btn->config.debounce_time_ms) {
            debounced = raw;
        }
    } else {
        last_raw = raw;
    }

    uint8_t stable       = debounced;
    button_event_t event = BUTTON_NONE;

    if (stable && btn->is_pressed == 0) {
        btn->is_pressed           = 1;
        btn->press_time           = now;
        btn->long_press_triggered = 0;
    }

    if (!stable && btn->is_pressed == 1) {
        btn->is_pressed = 0;
        if (!btn->long_press_triggered) {
            uint32_t pdur     = now - btn->press_time;
            uint16_t t_double = button_double_click_time[btn->config.double_click_level];
            if (pdur > btn->config.debounce_time_ms) {
                btn->click_count++;
                btn->release_time = now;
            }
        }
    }

    if (btn->is_pressed && !btn->long_press_triggered) {
        uint16_t t_long = button_long_press_time[btn->config.long_press_level];
        if ((now - btn->press_time) >= t_long) {
            btn->long_press_triggered = 1;
            btn->click_count          = 0;
            event                     = BUTTON_LONG_PRESS;
            if (btn->long_press_callback) btn->long_press_callback();
            return event;
        }
    }

    if (btn->click_count > 0 && btn->is_pressed == 0) {
        uint16_t t_double = button_double_click_time[btn->config.double_click_level];
        uint32_t idle     = now - btn->release_time;
        if (btn->click_count == 2) {
            btn->click_count = 0;
            event            = BUTTON_DOUBLE_CLICK;
            if (btn->double_click_callback) btn->double_click_callback();
            return event;
        } else if (btn->click_count == 1 && idle >= t_double) {
            btn->click_count = 0;
            event            = BUTTON_SINGLE_CLICK;
            if (btn->single_click_callback) btn->single_click_callback();
            return event;
        }
    }

    return BUTTON_NONE;
}