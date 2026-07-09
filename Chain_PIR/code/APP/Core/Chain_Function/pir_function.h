/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PIR_FUNCTION_H_
#define PIR_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "base_function.h"
#include "myflash.h"

#define CHAIN_PIR_REPORT_TYPE (0x05)

extern __IO uint8_t g_trigger_keep_seconds;
typedef enum {
    CHAIN_PIR_GET_IR       = 0x37,  // Get the latest IR Induction value
    CHAIN_PIR_AUTO_SEND_IR = 0xE0,  // Auto send the latest IR Induction value when the PIR sensor is triggered
    CHAIN_PIR_SET_AUTO_SEND_IR_STATUS = 0xE1,  // Control function: auto send ir status to uart when ir status change
    CHAIN_PIR_GET_AUTO_SEND_IR_STATUS = 0xE2,  // Acquire Control function Status
    CHAIN_SET_TRIGGER_DELAY           = 0xE3,  // Set Delay times when trigger Non-Person Status
    CHAIN_GET_TRIGGER_DELAY           = 0xE4,  // Get Delay times when trigger Non-Person Status
} chain_pir_cmd_t;                             // chain_pir_cmd_t is the command type for PIR measurement operations

void chain_pir_get_ir_induction(
    void);  // chain_pir_get_ir_induction is a function to retrieve the latest IR Induction value

void chain_pir_set_auto_send_status(uint8_t status);

void chain_pir_get_auto_send_status(void);

void chain_pir_set_trigger_keep_seconds(uint8_t seconds, uint8_t save_to_flash);

void chain_pir_get_trigger_delay_times(void);

void pir_status_update(void);

#ifdef __cplusplus
}
#endif

#endif /* PIR_FUNCTION_H_ */
