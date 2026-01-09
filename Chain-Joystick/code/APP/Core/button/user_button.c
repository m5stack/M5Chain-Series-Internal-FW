/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_button.h"

extern __IO uint8_t event_overlay[3];
extern __IO uint8_t fade_type;
extern __IO uint8_t fade_frames_left;

static button_t user_button;
static button_config_t button_cfg = {
    .double_click_level = BUTTON_DOUBLE_CLICK_TIME_200MS,
    .long_press_level   = BUTTON_LONG_PRESS_TIME_3S,
    .active_level       = BUTTON_ACTIVE_LOW,
    .debounce_time_ms   = BUTTON_DEBOUNCE_DEFAULT,
};

static uint8_t button_mode = 1;

static uint8_t s_ret_buf[20] = {0};

static void on_single_click(void)
{
    uint16_t key_status = KEY_EVENT_SINGLE_CLICK;
    chain_command_complete_return(CHAIN_BUTTON_TRIGGER_EVENT, (uint8_t *)&key_status, 2);
    fade_type        = 2;  // G分量
    fade_frames_left = RGB_FADE_FRAMES;
    event_overlay[0] = 0;
    event_overlay[1] = RGB_FADE_MAX;
    event_overlay[2] = 0;
}

static void on_double_click(void)
{
    uint16_t key_status = KEY_EVENT_DOUBLE_CLICK;
    chain_command_complete_return(CHAIN_BUTTON_TRIGGER_EVENT, (uint8_t *)&key_status, 2);
    fade_type        = 3;  // B分量
    fade_frames_left = RGB_FADE_FRAMES;
    event_overlay[0] = 0;
    event_overlay[1] = 0;
    event_overlay[2] = RGB_FADE_MAX;
}

static void on_long_press(void)
{
    uint16_t key_status = KEY_EVENT_LONG_PRESS;
    chain_command_complete_return(CHAIN_BUTTON_TRIGGER_EVENT, (uint8_t *)&key_status, 2);
    fade_type        = 1;  // R分量
    fade_frames_left = RGB_FADE_FRAMES;
    event_overlay[0] = RGB_FADE_MAX;
    event_overlay[1] = 0;
    event_overlay[2] = 0;
}

void user_button_init(void)
{
    button_init(&user_button, Button_GPIO_Port, Button_Pin, &button_cfg);
    button_set_callbacks(&user_button, on_single_click, on_double_click, on_long_press);
}

void user_button_scan(void)
{
    button_scan(&user_button);
}

void user_get_button_status(void)
{
    uint8_t status = button_read_pin(&user_button);
    chain_command_complete_return(CHAIN_BUTTON_GET_STATUS, (uint8_t *)&status, 1);
}

void user_set_button_mode(uint8_t mode)
{
    uint8_t result = 0;
    if (mode <= 1) {
        result = 1;
        if (button_mode != mode) {
            button_mode = mode;
            if (mode == 0) {
                button_pause(&user_button);
            } else {
                button_resume(&user_button);
            }
        }
    }
    s_ret_buf[0] = result;
    chain_command_complete_return(CHAIN_BUTTON_SET_MODE, s_ret_buf, 1);
}

void user_get_button_mode(void)
{
    chain_command_complete_return(CHAIN_BUTTON_GET_MODE, &button_mode, 1);
}

void user_set_button_trigger_timeout(uint8_t *buffer, uint8_t len)
{
    uint8_t result = 0;

    if (len == 2 && buffer) {
        button_double_click_time_t dl = buffer[0];
        button_long_press_time_t ll   = buffer[1];
        if (dl > BUTTON_DOUBLE_CLICK_TIME_1000MS || ll > BUTTON_LONG_PRESS_TIME_10S) {
            result = 0;
        } else {
            if (button_cfg.double_click_level != dl || button_cfg.long_press_level != ll) {
                button_cfg.double_click_level = dl;
                button_cfg.long_press_level   = ll;
                button_set_config(&user_button, &button_cfg);
            }
            result = 1;
        }
    }
    s_ret_buf[0] = result;
    chain_command_complete_return(CHAIN_BUTTON_SET_TRIGGER_TIMEOUT, s_ret_buf, 1);
}

void user_get_button_trigger_timeout(void)
{
    s_ret_buf[0] = button_cfg.double_click_level;
    s_ret_buf[1] = button_cfg.long_press_level;
    chain_command_complete_return(CHAIN_BUTTON_GET_TRIGGER_TIMEOUT, s_ret_buf, 2);
}
