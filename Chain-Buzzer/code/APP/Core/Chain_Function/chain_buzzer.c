/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "chain_buzzer.h"

static uint8_t buzzer_cfg_duty       = BUZZER_DUTY_DEFAULT;
static uint16_t buzzer_cfg_freq      = BUZZER_FREQ_DEFAULT;
static buzzer_status_t buzzer_cfg_on = BUZZER_STATUS_OFF;

static uint8_t buzzer_tmp_duty       = 0;
static uint16_t buzzer_tmp_freq      = 0;
static uint8_t buzzer_is_tmp_playing = 0;

static uint32_t buzzer_start_time = 0;
static uint16_t duration_time     = 0;

static uint8_t s_ret_buf[20]  = {0};
static uint8_t s_ret_buf_size = 0;

static buzzer_mode_t buzzer_mode = BUZZER_MODE_AUTO_PLAY;

// ==================== Complete Note Frequency Array ====================
const uint16_t note_frequencies[NOTE_COUNT] = {
    // Special note
    NOTE_REST,  // Index 0: Rest (0 Hz)

    // Octave 3 (12 notes)
    NOTE_C3, NOTE_CS3, NOTE_D3, NOTE_DS3, NOTE_E3, NOTE_F3, NOTE_FS3, NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3,

    // Octave 4 (12 notes)
    NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,

    // Octave 5 (12 notes)
    NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,

    // Octave 6 (12 notes)
    NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,

    // Octave 7 (12 notes)
    NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7,

    // Octave 8 (1 note)
    NOTE_C8};

static inline long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    if (in_max == in_min) return out_min;
    long result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    if (result < out_min) result = out_min;
    if (result > out_max) result = out_max;
    return result;
}

static void buzzer_hw_apply(uint16_t freq, uint8_t duty)
{
    if (freq < BUZZER_FREQ_MIN) freq = BUZZER_FREQ_MIN;
    if (freq > BUZZER_FREQ_MAX) freq = BUZZER_FREQ_MAX;
    if (duty > BUZZER_DUTY_MAX) duty = BUZZER_DUTY_MAX;
    uint32_t psc     = 0;
    uint32_t arr     = (TIM_CLK_FREQ / freq) - 1;
    uint32_t ccr_val = map(duty, 0, 100, 0, arr);
    LL_TIM_DisableCounter(TIM2);
    LL_TIM_SetPrescaler(TIM2, psc);
    LL_TIM_SetAutoReload(TIM2, arr);
    LL_TIM_OC_SetCompareCH1(TIM2, ccr_val);
    LL_TIM_GenerateEvent_UPDATE(TIM2);
    LL_TIM_EnableCounter(TIM2);
}

void buzzer_init(void)
{
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1);
    LL_TIM_EnableCounter(TIM2);
    buzzer_stop();
}

void buzzer_set_mode(buzzer_mode_t mode)
{
    buzzer_mode = mode;
}

void buzzer_set_freq(uint16_t freq)
{
    buzzer_cfg_freq = freq;
}

void buzzer_set_duty(uint8_t duty)
{
    buzzer_cfg_duty = duty;
}

void buzzer_set_turn_on(buzzer_status_t on)
{
    buzzer_cfg_on = on;
}

void buzzer_set_play(uint16_t freq, uint8_t duty, uint16_t duration)
{
    if (buzzer_mode != BUZZER_MODE_AUTO_PLAY && buzzer_mode != BUZZER_MODE_NOTE_PLAY) {
        return;
    }

    buzzer_tmp_freq = freq;
    buzzer_tmp_duty = duty;

    buzzer_hw_apply(buzzer_tmp_freq, buzzer_tmp_duty);

    buzzer_start_time     = HAL_GetTick();
    duration_time         = duration;
    buzzer_is_tmp_playing = 1;
}

void buzzer_stop(void)
{
    LL_TIM_OC_SetCompareCH1(TIM2, 0);
}

void buzzer_update(void)
{
    static uint8_t last_on    = 0xFF;
    static uint16_t last_freq = 0xFFFF;
    static uint8_t last_duty  = 0xFF;
    uint8_t do_update         = 0;

    switch (buzzer_mode) {
        case BUZZER_MODE_AUTO_PLAY:
            if (buzzer_is_tmp_playing && (HAL_GetTick() - buzzer_start_time) >= duration_time) {
                buzzer_is_tmp_playing = 0;
                buzzer_stop();
            }
            break;
        case BUZZER_MODE_MANUAL_PLAY:
            if ((last_on != buzzer_cfg_on) || (last_freq != buzzer_cfg_freq) || (last_duty != buzzer_cfg_duty)) {
                do_update = 1;
            }
            if (do_update) {
                if (buzzer_cfg_on == BUZZER_STATUS_ON) {
                    buzzer_hw_apply(buzzer_cfg_freq, buzzer_cfg_duty);
                } else {
                    buzzer_stop();
                }
                last_on   = buzzer_cfg_on;
                last_freq = buzzer_cfg_freq;
                last_duty = buzzer_cfg_duty;
            }
            break;
        case BUZZER_MODE_NOTE_PLAY:
            if (buzzer_is_tmp_playing && (HAL_GetTick() - buzzer_start_time) >= duration_time) {
                buzzer_is_tmp_playing = 0;
                buzzer_stop();
            }
            break;
        default:
            break;
    }
}

void chain_buzzer_set_mode(uint8_t *buffer, uint8_t size)
{
    operation_t operation_result = OPERATION_FAIL;

    if (buffer != NULL && size == 1) {
        if (buffer[0] <= BUZZER_MODE_NOTE_PLAY) {
            buzzer_set_mode((buzzer_mode_t)buffer[0]);
            operation_result = OPERATION_SUCCESS;
        }
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(CHAIN_BUZZER_SET_MODE, s_ret_buf, s_ret_buf_size);
}

void chain_buzzer_get_mode(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = (uint8_t)buzzer_mode;
    chain_command_complete_return(CHAIN_BUZZER_GET_MODE, s_ret_buf, s_ret_buf_size);
}

void chain_buzzer_set_auto_play(uint8_t *buffer, uint8_t size)
{
    operation_t operation_result = OPERATION_FAIL;

    if (buzzer_mode == BUZZER_MODE_AUTO_PLAY) {
        if (buffer != NULL && size == 5) {
            uint16_t freq     = (uint16_t)(buffer[0] | (buffer[1] << 8));
            uint8_t duty      = buffer[2];
            uint16_t duration = (uint16_t)(buffer[3] | (buffer[4] << 8));
            if (freq >= BUZZER_FREQ_MIN && freq <= BUZZER_FREQ_MAX && duty <= BUZZER_DUTY_MAX) {
                buzzer_set_play(freq, duty, duration);
                operation_result = OPERATION_SUCCESS;
            }
        }

    } else {
        operation_result = OPERATION_MODE_MISMATCH;
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(CHAIN_BUZZER_SET_AUTO_PLAY, s_ret_buf, s_ret_buf_size);
}

void chain_buzzer_set_note_play(uint8_t *buffer, uint8_t size)
{
    operation_t operation_result = OPERATION_FAIL;

    if (buzzer_mode == BUZZER_MODE_NOTE_PLAY) {
        if (buffer != NULL && size == 3) {
            uint8_t note_index = buffer[0];
            uint16_t duration  = (uint16_t)(buffer[1] | (buffer[2] << 8));

            if (note_index < NOTE_COUNT) {
                if (note_index != 0) {
                    buzzer_set_play(note_frequencies[note_index], 50, duration);
                } else {
                    buzzer_stop();
                }
                operation_result = OPERATION_SUCCESS;
            }
        }

    } else {
        operation_result = OPERATION_MODE_MISMATCH;
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(CHAIN_BUZZER_SET_NOTE_PLAY, s_ret_buf, s_ret_buf_size);
}

void chain_buzzer_set_freq(uint8_t *buffer, uint8_t size)
{
    operation_t operation_result = OPERATION_FAIL;

    if (buffer != NULL && size == 2) {
        uint16_t freq = (uint16_t)(buffer[0] | (buffer[1] << 8));
        if (freq >= BUZZER_FREQ_MIN && freq <= BUZZER_FREQ_MAX) {
            buzzer_set_freq(freq);
            operation_result = OPERATION_SUCCESS;
        }
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(CHAIN_BUZZER_SET_FREQ, s_ret_buf, s_ret_buf_size);
}

void chain_buzzer_get_freq(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = (uint8_t)(buzzer_cfg_freq & 0xFF);
    s_ret_buf[s_ret_buf_size++] = (uint8_t)(buzzer_cfg_freq >> 8);
    chain_command_complete_return(CHAIN_BUZZER_GET_FREQ, s_ret_buf, s_ret_buf_size);
}

void chain_buzzer_set_duty(uint8_t *buffer, uint8_t size)
{
    operation_t operation_result = OPERATION_FAIL;

    if (buffer != NULL && size == 1) {
        if (buffer[0] <= BUZZER_DUTY_MAX) {
            buzzer_set_duty(buffer[0]);
            operation_result = OPERATION_SUCCESS;
        }
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(CHAIN_BUZZER_SET_DUTY, s_ret_buf, s_ret_buf_size);
}

void chain_buzzer_get_duty(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = buzzer_cfg_duty;
    chain_command_complete_return(CHAIN_BUZZER_GET_DUTY, s_ret_buf, s_ret_buf_size);
}

void chain_buzzer_set_status(uint8_t *buffer, uint8_t size)
{
    operation_t operation_result = OPERATION_FAIL;

    if (buffer != NULL && size == 1) {
        if (buffer[0] <= BUZZER_STATUS_ON) {
            buzzer_set_turn_on((buzzer_status_t)buffer[0]);
            operation_result = OPERATION_SUCCESS;
        }
    }

    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(CHAIN_BUZZER_SET_STATUS, s_ret_buf, s_ret_buf_size);
}

void chain_buzzer_get_status(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = (uint8_t)buzzer_cfg_on;
    chain_command_complete_return(CHAIN_BUZZER_GET_STATUS, s_ret_buf, s_ret_buf_size);
}
