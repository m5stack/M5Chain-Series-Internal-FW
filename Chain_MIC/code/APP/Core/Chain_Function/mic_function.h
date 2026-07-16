/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _MIC_FUNCTION_H_
#define _MIC_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "base_function.h"
#include "RGB.h"

typedef enum {
    CHAIN_MIC_GET_12ADC            = 0x30,
    CHAIN_MIC_GET_8ADC             = 0x31,
    CHAIN_MIC_SET_THRESHOLD_VALUE  = 0x32,
    CHAIN_MIC_GET_THRESHOLD_VALUE  = 0x33,
    CHAIN_MIC_TRIGGER              = 0xE0,
    CHAIN_MIC_SET_REPORT_MODE      = 0xE1,
    CHAIN_MIC_GET_REPORT_MODE      = 0xE2,
    CHAIN_MIC_SET_TRIGGER_INTERVAL = 0xE3,
    CHAIN_MIC_GET_TRIGGER_INTERVAL = 0xE4,
} chain_mic_cmd_t;

void mic_adc_sample_ready(uint16_t adc_value);
void chain_mic_get_12value(void);
void chain_mic_get_8value(void);
void chain_mic_set_threshold_value(uint8_t *buffer, uint8_t size);
void chain_mic_get_threshold_value(void);
void chain_mic_trigger(void);
void chain_mic_set_report_mode(uint8_t *buffer, uint8_t size);
void chain_mic_get_report_mode(void);
void chain_mic_set_trigger_interval(uint8_t *buffer, uint8_t size);
void chain_mic_get_trigger_interval(void);

#ifdef __cplusplus
}
#endif

#endif /* _MIC_FUNCTION_H_ */
