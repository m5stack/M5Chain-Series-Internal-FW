#include "RGB.h"
#include "stdio.h"
#include "tim.h"

extern button_t user_button;
extern __IO uint8_t switch_status;
extern __IO uint8_t g_pedal_overlay;

__IO uint8_t g_send_complete_flag          = 1;
RGB_Color_TypeDef event_overlay[RGB_NUM]   = {0};
__IO uint8_t fade_type                     = 0;
__IO uint8_t fade_frames_left              = 0;
const static RGB_Color_TypeDef BLACK       = {0};
static RGB_Color_TypeDef color[RGB_NUM]    = {0};
static uint8_t s_rgb_buf[RGB_NUM + 10][24] = {0};
static uint8_t s_ret_buf[20]               = {0};
static uint8_t s_ret_buf_size              = 0;
__IO uint8_t s_rgb_update_flag             = 0;
static uint8_t fade_flag                   = 0;

static uint8_t sat_add(uint8_t a, uint8_t b)
{
    uint16_t tmp = (uint16_t)a + (uint16_t)b;
    return (tmp > 255) ? 255 : (uint8_t)tmp;
}

static uint8_t sat_sub(uint8_t a, uint8_t b)
{
    return (a > b) ? (a - b) : 0;
}

static void clear_all_overlay(void)
{
    for (uint8_t i = 0; i < RGB_NUM; i++) {
        set_overlay_color(i, 0, 0, 0);
    }
}

static void activate_tim1_dma(void)
{
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, (RGB_NUM + 10) * 24);
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)s_rgb_buf);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t) (&(TIM1->CCR1)));
    LL_DMA_ClearFlag_GI5(DMA1);
    LL_DMA_ClearFlag_TC5(DMA1);
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_5);
    LL_TIM_EnableDMAReq_CC1(TIM1);
    LL_TIM_EnableAllOutputs(TIM1);
    LL_TIM_CC_SetDMAReqTrigger(TIM1, LL_TIM_CCDMAREQUEST_CC);
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);
}

static void rgb_setcolor(uint8_t led_id, RGB_Color_TypeDef color)
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    if (!user_button.in_config_mode) {
        r = color.R * (g_light / 100.0);
        g = color.G * (g_light / 100.0);
        b = color.B * (g_light / 100.0);
    }

    r = sat_add(r, event_overlay[led_id].R);
    g = sat_add(g, event_overlay[led_id].G);
    b = sat_add(b, event_overlay[led_id].B);

    for (uint8_t i = 0; i < 8; i++) {
        s_rgb_buf[led_id][i] = ((g & (1 << (7 - i))) ? (CODE_1) : CODE_0);
    }
    for (uint8_t i = 8; i < 16; i++) {
        s_rgb_buf[led_id][i] = ((r & (1 << (15 - i))) ? (CODE_1) : CODE_0);
    }
    for (uint8_t i = 16; i < 24; i++) {
        s_rgb_buf[led_id][i] = ((b & (1 << (23 - i))) ? (CODE_1) : CODE_0);
    }
}

static void rgb_send_array(void)
{
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, (RGB_NUM + 10) * 24);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
    LL_TIM_EnableCounter(TIM1);
}

static void turn_off_all_handle(void)
{
    for (uint8_t i = 0; i < RGB_NUM; i++) {
        rgb_setcolor(i, BLACK);
    }
    rgb_send_array();
}

void set_overlay_color(uint8_t led_id, uint8_t r, uint8_t g, uint8_t b)
{
    event_overlay[led_id].R = r;
    event_overlay[led_id].G = g;
    event_overlay[led_id].B = b;
}

void rgb_init(void)
{
    activate_tim1_dma();
    turn_off_all_handle();
    HAL_Delay(1);
}

void chain_set_rgb_value(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer != NULL && size >= 2) {
        uint8_t start = buffer[0];
        uint8_t count = buffer[1];
        if (size == (uint8_t)(count * 3 + 2) && IS_VALID_RANGE(start, count)) {
            uint8_t *pdata = &buffer[2];
            for (uint8_t i = 0; i < count; i++) {
                color[start + i].R = *pdata++;
                color[start + i].G = *pdata++;
                color[start + i].B = *pdata++;
            }
            s_rgb_update_flag = 1;
            operation_result  = OPERATION_SUCCESS;
        }
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(CHAIN_SET_RGB_VALUE, s_ret_buf, s_ret_buf_size);
}

void chain_get_rgb_value(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer != NULL && size == 2) {
        uint8_t start = buffer[0];
        uint8_t count = buffer[1];
        if (IS_VALID_RANGE(start, count)) {
            operation_result = OPERATION_SUCCESS;
        }
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;

    if (operation_result == OPERATION_SUCCESS) {
        uint8_t *pout = &s_ret_buf[s_ret_buf_size];
        for (uint8_t i = 0; i < buffer[1]; i++) {
            *pout++ = color[buffer[0] + i].R;
            *pout++ = color[buffer[0] + i].G;
            *pout++ = color[buffer[0] + i].B;
        }
        s_ret_buf_size = (uint8_t)(pout - s_ret_buf);
    }

    chain_command_complete_return(CHAIN_GET_RGB_VALUE, s_ret_buf, s_ret_buf_size);
}

void chain_set_light_value(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    if (buffer && size == 2) {
        uint8_t light_value = buffer[0];
        uint8_t save_flag   = buffer[1];

        if (light_value <= 100) {
            if (save_flag && light_value != get_rgb_light()) {
                set_rgb_light(light_value);
            }
            if (g_light != light_value) {
                g_light           = light_value;
                s_rgb_update_flag = 1;
            }
            operation_result = OPERATION_SUCCESS;
        }
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(CHAIN_SET_RGB_LIGHT, s_ret_buf, s_ret_buf_size);
}

void chain_get_light_value(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = g_light;
    chain_command_complete_return(CHAIN_GET_RGB_LIGHT, s_ret_buf, s_ret_buf_size);
}

static void apply_overlay_pattern(uint8_t pattern)
{
    switch (pattern) {
        case RGB_TYPE_SWITCH_INDICATOR:
            clear_all_overlay();
            if (switch_status) {
                for (uint8_t i = 0; i < RGB_NUM; i++) {
                    if (g_pedal_overlay & (1 << i)) {
                        set_overlay_color(i, 227, 38, 54);
                    }
                }
            }
            break;

        case RGB_TYPE_CONFIG_MODE:
            if (user_button.in_config_mode) {
                set_overlay_color(0, 255, 0, 255);

                if (user_button.work_mode == BUTTON_SINGLE_MODE) {
                    set_overlay_color(1, 0, 0, 0);
                    set_overlay_color(2, 255, 182, 193);
                    set_overlay_color(3, 0, 0, 0);
                } else {
                    set_overlay_color(1, 255, 182, 193);
                    set_overlay_color(2, 0, 0, 0);
                    set_overlay_color(3, 255, 182, 193);
                }
            } else {
                clear_all_overlay();
            }
            break;

        case RGB_TYPE_SINGLE_MODE_INDICATOR:
            set_overlay_color(0, 255, 0, 255);
            set_overlay_color(1, 0, 0, 0);
            set_overlay_color(2, 152, 255, 152);
            set_overlay_color(3, 0, 0, 0);
            break;

        case RGB_TYPE_SWITCH_MODE_INDICATOR:
            set_overlay_color(0, 255, 0, 255);
            set_overlay_color(1, 152, 255, 152);
            set_overlay_color(2, 0, 0, 0);
            set_overlay_color(3, 152, 255, 152);
            break;

        default:
            break;
    }
}

void rgb_update(void)
{
    static uint32_t last_step_tick = 0;

    if (!g_send_complete_flag) {
        return;
    }

    uint32_t now = HAL_GetTick();

    if (fade_type) {
        if (fade_type == 3) {
            if (now - last_step_tick >= RGB_FADE_MS_STEP) {
                last_step_tick = now;

                uint8_t blue_intensity =
                    (fade_frames_left > 0) ? (uint8_t)(((fade_frames_left - 1) * RGB_FADE_MAX) / (RGB_FADE_FRAMES - 1))
                                           : 0;

                for (uint8_t i = 0; i < RGB_NUM; i++) {
                    if (g_pedal_overlay & (1 << i)) {
                        set_overlay_color(i, 0, 0, blue_intensity);
                    }
                }

                if (fade_frames_left > 0) {
                    fade_frames_left--;
                }

                if (fade_frames_left == 0) {
                    clear_all_overlay();
                    fade_type = 0;
                }
                fade_flag = 1;
            }
        } else {
            apply_overlay_pattern(fade_type);
            fade_flag = 1;
            fade_type = 0;
        }
    }

    if (fade_flag || s_rgb_update_flag) {
        for (uint8_t i = 0; i < RGB_NUM; i++) {
            rgb_setcolor(i, color[i]);
        }
        rgb_send_array();
        s_rgb_update_flag    = 0;
        g_send_complete_flag = 0;
        fade_flag            = 0;
    }
}
