#include "button.h"

extern __IO uint8_t g_pedal_work_mode;

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
    if (pconfig) {
        btn->config                 = *pconfig;
        btn->config.long_press_time = btn->config.long_press_time * 1000;
    } else {
        btn->config.double_click_time = 100;
        btn->config.long_press_time   = 1000 * 10;
        btn->config.active_level      = BUTTON_ACTIVE_LOW;
        btn->config.debounce_time_ms  = BUTTON_DEBOUNCE_DEFAULT;
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

    btn->in_config_mode       = 0;
    btn->work_mode            = g_pedal_work_mode;
    btn->mode_change_callback = NULL;
}

void button_set_config(button_t* btn, const button_config_t* cfg)
{
    if (cfg) {
        button_config_t tmp         = *cfg;
        btn->config                 = tmp;
        btn->config.long_press_time = btn->config.long_press_time * 1000;
        button_reset_state(btn);
    }
}

void button_set_callbacks(button_t* btn, void (*single_click_cb)(void), void (*double_click_cb)(void),
                          void (*long_press_cb)(void), void (*mode_change_cb)(void))
{
    btn->single_click_callback = single_click_cb;
    btn->double_click_callback = double_click_cb;
    btn->long_press_callback   = long_press_cb;
    btn->mode_change_callback  = mode_change_cb;
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
            uint32_t pdur = now - btn->press_time;
            if (pdur > btn->config.debounce_time_ms) {
                if (btn->in_config_mode) {
                    btn->work_mode = (btn->work_mode == BUTTON_SINGLE_MODE) ? BUTTON_SWITCH_MODE : BUTTON_SINGLE_MODE;
                    if (btn->mode_change_callback) {
                        btn->mode_change_callback();
                    }
                } else {
                    if (btn->single_click_callback) btn->single_click_callback();
                }

                event = BUTTON_SINGLE_CLICK;
                return event;
            }
        }
    }

    if (btn->config.long_press_time && btn->is_pressed && !btn->long_press_triggered) {
        uint16_t t_long = btn->config.long_press_time;
        if ((now - btn->press_time) >= t_long) {
            btn->long_press_triggered = 1;

            if (!btn->in_config_mode) {
                btn->in_config_mode = 1;
                event               = BUTTON_CONFIG_ENTER;
            } else {
                btn->in_config_mode = 0;
                event               = BUTTON_CONFIG_EXIT;
            }

            if (btn->long_press_callback) {
                btn->long_press_callback();
            }
            return event;
        }
    }

    return BUTTON_NONE;
}

button_work_mode_t button_get_work_mode(button_t* btn)
{
    return btn->work_mode;
}
