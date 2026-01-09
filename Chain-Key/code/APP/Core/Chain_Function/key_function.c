/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <key_function.h>

static uint16_t s_key_status = 0;  // s_key_status is a static variable to hold the current status of the key press

/**
 * @brief Sends a command indicating that a key has been pressed down.
 * @note This function sets the key status to a predefined value representing
 *       a key press down event and sends this status using the
 *       chain_command_complete_return function.
 *
 * @param None
 * @retval None
 */
void key_press_down_send(void)
{                           // key_press_down_send is a function that handles the key press down event
    s_key_status = 0x0001;  // Sets s_key_status to 0x11, indicating the key press down state
    chain_command_complete_return(CHAIN_KEY_PRESS_DOWN, (uint8_t *)&s_key_status,
                                  2);  // Sends the command with the current key status
}
