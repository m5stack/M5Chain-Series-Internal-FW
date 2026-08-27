#include "user_pedal.h"

extern __IO uint8_t fade_type;
extern __IO uint8_t fade_frames_left;
extern __IO uint8_t rgb_mix_flag;
extern __IO uint8_t g_pedal_overlay;
extern __IO uint8_t g_pedal_enter_config_time;
extern __IO uint8_t s_rgb_update_flag;

button_t user_button;
__IO uint8_t switch_status      = 0;
__IO uint8_t button_report_mode = 1;

static button_config_t button_cfg = {
    .double_click_time = 200,
    .long_press_time   = 10,
    .active_level      = BUTTON_ACTIVE_LOW,
    .debounce_time_ms  = BUTTON_DEBOUNCE_DEFAULT,
};

static uint16_t toggle_switch_status(void)
{
    switch_status = (switch_status == 0) ? 1 : 0;
    return (switch_status == 1) ? PEDAL_STATUS_SWITCH_ON : PEDAL_STATUS_SWITCH_OFF;
}

static void set_rgb_effect(rgb_fade_type_t type)
{
    fade_type = type;
    if (type == RGB_TYPE_BLUE_FADE) {
        fade_frames_left = RGB_FADE_FRAMES;
    }
}

static void send_key_event(uint16_t key_status)
{
    if (button_report_mode) {
        chain_command_complete_return(CHAIN_PEDAL_TRIGGER_EVENT, (uint8_t *)&key_status, sizeof(key_status));
    }
}

static void handle_single_mode_click(void)
{
    set_rgb_effect(RGB_TYPE_BLUE_FADE);
    send_key_event(PEDAL_EVENT_SINGLE_CLICK);
}

static void handle_switch_mode_click(void)
{
    uint16_t key_status = toggle_switch_status();
    set_rgb_effect(RGB_TYPE_SWITCH_INDICATOR);
    send_key_event(key_status);
}

static void on_single_click(void)
{
    if (user_button.work_mode == BUTTON_SINGLE_MODE) {
        handle_single_mode_click();
    } else if (user_button.work_mode == BUTTON_SWITCH_MODE) {
        handle_switch_mode_click();
    }
}

static void on_long_press(void)
{
    if (user_button.in_config_mode) {
        set_rgb_effect(RGB_TYPE_CONFIG_MODE);
    } else {
        if (user_button.work_mode == BUTTON_SWITCH_MODE) {
            set_rgb_effect(RGB_TYPE_SWITCH_INDICATOR);
        } else {
            set_rgb_effect(RGB_TYPE_CONFIG_MODE);
        }
    }
}

static void on_mode_change(void)
{
    if (user_button.work_mode == BUTTON_SINGLE_MODE) {
        set_rgb_effect(RGB_TYPE_SINGLE_MODE_INDICATOR);
    } else if (user_button.work_mode == BUTTON_SWITCH_MODE) {
        set_rgb_effect(RGB_TYPE_SWITCH_MODE_INDICATOR);
    }
}

void user_button_init(void)
{
    button_cfg.long_press_time = g_pedal_enter_config_time;

    button_init(&user_button, Button_GPIO_Port, Button_Pin, &button_cfg);

    button_set_callbacks(&user_button, on_single_click, NULL, on_long_press, on_mode_change);

    switch_status = 0;
}

void user_button_scan(void)
{
    button_scan(&user_button);
}

void user_get_pedal_button_status(void)
{
    uint8_t status = button_read_pin(&user_button);
    chain_command_complete_return(CHAIN_PEDAL_GET_BUTTON_STATUS, (uint8_t *)&status, 1);
}

void user_get_pedal_switch_status(void)
{
    uint8_t status = switch_status;
    chain_command_complete_return(CHAIN_PEDAL_GET_SWITCH_STATUS, (uint8_t *)&status, 1);
}

void user_set_pedal_work_mode(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer && size == 2) {
        uint8_t work_mode = buffer[0];
        uint8_t save_flag = buffer[1];
        if (work_mode <= BUTTON_SWITCH_MODE) {
            if (save_flag && work_mode != get_pedal_work_mode()) {
                set_pedal_work_mode(work_mode);
            }
            if (work_mode != user_button.work_mode) {
                user_button.work_mode = work_mode;
                if (user_button.in_config_mode) {
                    if (user_button.mode_change_callback) {
                        user_button.mode_change_callback();
                    }
                }
            }
            operation_result = OPERATION_SUCCESS;
        }
    }

    chain_command_complete_return(CHAIN_PEDAL_SET_WORK_MODE, (uint8_t *)&operation_result, 1);
}

void user_get_pedal_work_mode(void)
{
    uint8_t work_mode = user_button.work_mode;
    chain_command_complete_return(CHAIN_PEDAL_GET_WORK_MODE, (uint8_t *)&work_mode, 1);
}

void user_set_pedal_button_mode(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer && size == 1) {
        uint8_t button_mode = buffer[0];
        if (button_mode <= 1) {
            if (button_mode != button_report_mode) {
                button_report_mode = button_mode;
            }
            operation_result = OPERATION_SUCCESS;
        }
    }

    chain_command_complete_return(CHAIN_PEDAL_SET_BUTTON_MODE, (uint8_t *)&operation_result, 1);
}

void user_get_pedal_button_mode(void)
{
    uint8_t button_mode = button_report_mode;
    chain_command_complete_return(CHAIN_PEDAL_GET_BUTTON_MODE, (uint8_t *)&button_mode, 1);
}

void user_set_pedal_enter_config_time(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer && size == 2) {
        uint8_t enter_config_time = buffer[0];
        uint8_t save_flag         = buffer[1];

        if (enter_config_time <= 30) {
            if (save_flag && enter_config_time != get_pedal_enter_config_time()) {
                set_pedal_enter_config_time(enter_config_time);
            }

            if (enter_config_time != button_cfg.long_press_time) {
                button_cfg.long_press_time = enter_config_time;
                button_set_config(&user_button, &button_cfg);
            }

            if (enter_config_time == 0) {
                user_button.in_config_mode = 0;
                if (user_button.long_press_callback) {
                    user_button.long_press_callback();
                }
            }

            operation_result = OPERATION_SUCCESS;
        }
    }

    chain_command_complete_return(CHAIN_PEDAL_SET_ENTER_CONFIG_TIME, (uint8_t *)&operation_result, 1);
}

void user_get_pedal_enter_config_time(void)
{
    uint8_t enter_config_time = button_cfg.long_press_time;
    chain_command_complete_return(CHAIN_PEDAL_GET_ENTER_CONFIG_TIME, (uint8_t *)&enter_config_time, 1);
}

void user_set_pedal_rgb_overlay_value(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer && size == 2) {
        uint8_t overlay_mode = buffer[0];
        uint8_t save_flag    = buffer[1];

        if (overlay_mode <= PEDAL_OVERLAY_BASE) {
            if (save_flag && overlay_mode != get_pedal_overlay()) {
                set_pedal_overlay(overlay_mode);
            }

            if (overlay_mode != g_pedal_overlay) {
                g_pedal_overlay = overlay_mode;
                if (!user_button.in_config_mode && user_button.work_mode == BUTTON_SWITCH_MODE && switch_status) {
                    set_rgb_effect(RGB_TYPE_SWITCH_INDICATOR);
                }
            }

            operation_result = OPERATION_SUCCESS;
        }
    }

    chain_command_complete_return(CHAIN_PEDAL_SET_RGB_OVERLAY_VALUE, (uint8_t *)&operation_result, 1);
}

void user_get_pedal_rgb_overlay_value(void)
{
    uint8_t overlay_mode = g_pedal_overlay;
    chain_command_complete_return(CHAIN_PEDAL_GET_RGB_OVERLAY_VALUE, (uint8_t *)&overlay_mode, 1);
}
