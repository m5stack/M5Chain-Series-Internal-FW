/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "mic_function.h"
#include "adc.h"
#include "RGB.h"
#include <stdbool.h>
#include <stdint.h>

#define MIC_SAMPLE_WINDOW_SIZE (32)

// Microphone ADC Value
static uint16_t s_mic_adc_12value = 0;  // Variable to hold the latest 12-bit ADC value for microphone measurement
static uint8_t s_mic_adc_8value   = 0;  // Variable to hold the latest 8-bit ADC value for microphone measurement
static uint8_t mic_mode           = 1;  // 1: enabled, 0: paused
static uint16_t trigger_interval  = 300;
static uint16_t s_mic_sample_window[MIC_SAMPLE_WINDOW_SIZE] = {0};
static uint8_t s_mic_sample_index                           = 0;
static uint8_t s_mic_sample_count                           = 0;

extern __IO uint16_t g_threshold;
extern RGB_Color_TypeDef event_overlay;
extern __IO uint8_t fade_type;
extern __IO uint8_t fade_frames_left;

/**
 * @brief Map a 12-bit ADC value to an 8-bit value.
 * @note This function scales the input value from a 12-bit range (0 to 4095)
 *       to an 8-bit range (0 to 255).
 *
 * @param input The 12-bit ADC value (0 to 4095).
 * @retval The corresponding 8-bit value (0 to 255).
 */
uint8_t map12_to8(uint16_t input)
{
    return (uint8_t)((input * 255) / 4095);  // Map 12-bit value to 8-bit value
}

static void set_mic_trigger_fade_effect(void)
{
    fade_type        = 2;
    fade_frames_left = RGB_FADE_FRAMES;
    event_overlay.R  = 0;
    event_overlay.G  = RGB_FADE_MAX;
    event_overlay.B  = 0;
}

static void handle_mic_trigger_event(uint16_t trigger_type)
{
    chain_command_complete_return(CHAIN_MIC_TRIGGER, (uint8_t *)&trigger_type, 2);
}

void chain_mic_get_12value(void)
{
    chain_command_complete_return(CHAIN_MIC_GET_12ADC, (uint8_t *)&s_mic_adc_12value, 2);
}

void chain_mic_get_8value(void)
{
    chain_command_complete_return(CHAIN_MIC_GET_8ADC, (uint8_t *)&s_mic_adc_8value, 1);
}

void chain_mic_set_threshold_value(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_status = OPERATION_FAIL;
    if (size == 3 && buffer) {
        uint16_t threshold_value = (buffer[0] | (buffer[1] << 8));  // Combine two bytes into a 16-bit value
        uint8_t flag             = buffer[2];                       // Get the flag indicating whether to save the value
        if (threshold_value <= THRESHOLD_MAX) {
            g_threshold      = threshold_value;                      // Update the global threshold value
            operation_status = OPERATION_SUCCESS;                    // Operation succeeded
            if (flag && get_threshold_value() != threshold_value) {  // If the flag is set, save the new threshold value
                set_threshold_value(g_threshold);                    // Save the new threshold value
            }
        }
    }

    chain_command_complete_return(CHAIN_MIC_SET_THRESHOLD_VALUE, (uint8_t *)&operation_status, 1);
}

void chain_mic_get_threshold_value(void)
{
    chain_command_complete_return(CHAIN_MIC_GET_THRESHOLD_VALUE, (uint8_t *)&g_threshold, 2);
}

void chain_mic_trigger(void)
{
    static bool s_is_above_threshold  = false;
    static uint32_t last_rising_time  = 0;
    static uint32_t last_falling_time = 0;

    if (!mic_mode) {
        s_is_above_threshold = false;
        return;
    }

    if (s_mic_sample_count < 2) {
        return;
    }

    uint32_t current_time  = HAL_GetTick();
    bool saw_rising_cross  = false;
    bool saw_falling_cross = false;

    uint8_t start = (s_mic_sample_count < MIC_SAMPLE_WINDOW_SIZE) ? 0 : s_mic_sample_index;
    uint16_t prev = s_mic_sample_window[start];

    for (uint8_t i = 1; i < s_mic_sample_count; i++) {
        uint8_t current_index = (start + i) % MIC_SAMPLE_WINDOW_SIZE;
        uint16_t current      = s_mic_sample_window[current_index];

        if (prev <= g_threshold && current > g_threshold) {
            saw_rising_cross = true;
            if (!s_is_above_threshold && (current_time - last_rising_time) >= trigger_interval) {
                handle_mic_trigger_event(0x0301);
                set_mic_trigger_fade_effect();
                last_rising_time     = current_time;
                s_is_above_threshold = true;
            }
            break;
        }

        if (prev >= g_threshold && current < g_threshold) {
            saw_falling_cross = true;
            if (s_is_above_threshold && (current_time - last_falling_time) >= trigger_interval) {
                handle_mic_trigger_event(0x0300);
                set_mic_trigger_fade_effect();
                last_falling_time    = current_time;
                s_is_above_threshold = false;
            }
            break;
        }

        prev = current;
    }

    if (!saw_rising_cross && !saw_falling_cross) {
        uint8_t latest_index = (s_mic_sample_index == 0) ? (MIC_SAMPLE_WINDOW_SIZE - 1) : (s_mic_sample_index - 1);
        s_is_above_threshold = (s_mic_sample_window[latest_index] > g_threshold);
    }
}

void chain_mic_set_report_mode(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_status = OPERATION_FAIL;
    if (size == 1 && buffer) {
        uint8_t mode = buffer[0];
        if (mode <= 1) {
            mic_mode         = mode;
            operation_status = OPERATION_SUCCESS;
        }
    }
    chain_command_complete_return(CHAIN_MIC_SET_REPORT_MODE, (uint8_t *)&operation_status, 1);
}

void chain_mic_get_report_mode(void)
{
    chain_command_complete_return(CHAIN_MIC_GET_REPORT_MODE, (uint8_t *)&mic_mode, 1);
}

void chain_mic_set_trigger_interval(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_status = OPERATION_FAIL;
    if (size == 2 && buffer) {
        uint16_t circle = (buffer[0] | (buffer[1] << 8));  // Combine two bytes into a 16-bit value
        if (circle >= 300 && circle <= 1000) {
            trigger_interval = circle;             // Set the trigger interval based on the received value
            operation_status = OPERATION_SUCCESS;  // Operation succeeded
        }
    }
    chain_command_complete_return(CHAIN_MIC_SET_TRIGGER_INTERVAL, (uint8_t *)&operation_status, 1);
}

void chain_mic_get_trigger_interval(void)
{
    chain_command_complete_return(CHAIN_MIC_GET_TRIGGER_INTERVAL, (uint8_t *)&trigger_interval, 2);
}

void mic_adc_sample_ready(uint16_t adc_value)
{
    s_mic_adc_12value                       = adc_value;
    s_mic_adc_8value                        = map12_to8(adc_value);
    s_mic_sample_window[s_mic_sample_index] = adc_value;
    s_mic_sample_index                      = (s_mic_sample_index + 1) % MIC_SAMPLE_WINDOW_SIZE;
    if (s_mic_sample_count < MIC_SAMPLE_WINDOW_SIZE) {
        s_mic_sample_count++;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        mic_adc_sample_ready((uint16_t)HAL_ADC_GetValue(hadc));
    }
}
