/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ENCODER_FUNCTION_H_
#define ENCODER_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include "base_function.h"
#include "tim.h"
#include "myflash.h"

#define TIM_ENCODE_DEFAULT (0)  // Default timer encoder value

typedef enum {
    ENCODER_AB = 0x00,  // AB, increases clockwise
    ENCODER_BA = 0x01,  // BA, decreases clockwise
} ab_t;                 // AB pin status

typedef enum {
    NO_OVERFLOW       = 0x00,  // No overflow
    POSITIVE_OVERFLOW = 0x01,  // Positive overflow
    NEGATIVE_OVERFLOW = 0x02,  // Negative overflow
} overflow_t;                  // Overflow status

typedef enum {
    CHAIN_ENCODER_GET_VALUE       = 0x10,  // Get encoder value
    CHAIN_ENCODER_GET_INC_VALUE   = 0x11,  // Get encoder increment value
    CHAIN_ENCODER_RESET_VALUE     = 0x13,  // Reset encoder value
    CHAIN_ENCODER_RESET_INC_VALUE = 0x14,  // Reset encoder increment value
    CHAIN_ENCODER_SET_AB_STATUS   = 0x15,  // Set AB status, 0->AB, 1->BA
    CHAIN_ENCODER_GET_AB_STATUS   = 0x16,  // Get AB status, 0->AB, 1->BA
} chain_encoder_cmd_t;                     // Command types

void encoder_init(void);
void encoder_update(void);
void chain_encoder_get_value_handle(void);
void chain_encoder_get_inc_value_handle(void);
void chain_encoder_reset_value_handle(void);
void chain_encoder_reset_inc_value_handle(void);
void chain_encoder_set_ab_status_handle(uint8_t status, uint8_t flag);
void chain_encoder_get_ab_status_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_FUNCTION_H_ */
