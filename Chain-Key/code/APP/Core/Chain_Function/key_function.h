/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef KEY_FUNCTION_H_
#define KEY_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include "base_function.h"

typedef enum {
    CHAIN_KEY_PRESS_DOWN = 0xE0,  // CHAIN_KEY_PRESS_DOWN is the command indicating a key press down event
} chain_key_cmd_t;                // chain_key_cmd_t is the command type for key events

void key_press_down_send(void);  // key_press_down_send is a function to send the key press down command

#ifdef __cplusplus
}
#endif

#endif /* KEY_FUNCTION_H_ */
