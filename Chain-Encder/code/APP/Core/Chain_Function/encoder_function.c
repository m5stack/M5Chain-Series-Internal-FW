/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "encoder_function.h"

static int16_t s_us_now_cnt  = 0;       // Updated value
static uint16_t s_key_status = 0;       // s_key_status is a static variable to hold
                                        // the current status of the key press
extern __IO int16_t g_encoder_count;    // Encoder value
extern __IO int16_t g_increment_count;  // Encoder increment value
extern __IO uint8_t g_encoder_direct;   // Encoder direction, 0->AB, 1->BA

static uint8_t s_return_status = 0;

/**
 * @brief  Encoder initialization
 * @note   Start the timer in encoder mode and set the timer value.
 *
 * @param  None
 * @retval None
 */
void encoder_init(void)
{
    // Start the timer in encoder mode for both channels
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    // Set the initial counter value of the timer
    __HAL_TIM_SET_COUNTER(&htim3, TIM_ENCODE_DEFAULT);
}

/**
 * @brief  Check for addition overflow
 * @note   This function checks whether adding two integers will cause overflow.
 *
 * @param  a First integer
 * @param  b Second integer
 * @retval overflow_t Type of overflow (POSITIVE_OVERFLOW, NEGATIVE_OVERFLOW,
 * NO_OVERFLOW)
 */
static overflow_t check_add_overflow(int16_t a, int16_t b)
{
    // Perform addition and store the result in a 32-bit variable to prevent
    // overflow
    int32_t sum = (int32_t)a + (int32_t)b;

    // Check for positive overflow
    if (sum > INT16_MAX) {
        return POSITIVE_OVERFLOW;
    }
    // Check for negative overflow
    if (sum < INT16_MIN) {
        return NEGATIVE_OVERFLOW;
    }
    // No overflow detected
    return NO_OVERFLOW;
}

/**
 * @brief  Update encoder state
 * @note   Update the encoder count and button press state. If the encoder
 * direction is reversed, the count will be negated.
 *
 * @param  None
 * @retval None
 */
void encoder_update(void)
{
    // Get the current value of the encoder counter
    s_us_now_cnt = __HAL_TIM_GET_COUNTER(&htim3);

    // Check if the counter value is not zero
    if (s_us_now_cnt != 0) {
        // Reset the counter to the default value for the next reading
        __HAL_TIM_SET_COUNTER(&htim3, TIM_ENCODE_DEFAULT);

        // If the encoder direction is reversed, negate the current count
        if (g_encoder_direct == ENCODER_BA) {
            s_us_now_cnt = (-s_us_now_cnt);
        }

        // Check for overflow when updating the encoder count
        overflow_t type = check_add_overflow(g_encoder_count, s_us_now_cnt);
        switch (type) {
            case POSITIVE_OVERFLOW:
                // Set to maximum value if overflow occurs
                g_encoder_count = INT16_MAX;
                break;
            case NEGATIVE_OVERFLOW:
                // Set to minimum value if overflow occurs
                g_encoder_count = INT16_MIN;
                break;
            case NO_OVERFLOW:
                // Update the encoder count safely
                g_encoder_count += s_us_now_cnt;
                break;
            default:
                break;  // No action needed for unexpected cases
        }

        // Update the increment count with the new value
        g_increment_count += s_us_now_cnt;
    }
}

/**
 * @brief  Get the encoder value
 * @note   Retrieve the current value of the encoder.
 *
 * @param  None
 * @retval None
 */
void chain_encoder_get_value_handle(void)
{
    // Return the current value of the encoder via chain command
    chain_command_complete_return(CHAIN_ENCODER_GET_VALUE, (uint8_t *)&g_encoder_count, 2);
}

/**
 * @brief  Get the incremental encoder value
 * @note   Retrieve the current incremental value of the encoder.
 *
 * @param  None
 * @retval None
 */
void chain_encoder_get_inc_value_handle(void)
{
    // Return the current incremental value of the encoder via chain command
    chain_command_complete_return(CHAIN_ENCODER_GET_INC_VALUE, (uint8_t *)&g_increment_count, 2);
}

/**
 * @brief  Reset the encoder value
 * @note   Reset the encoder value to zero.
 *
 * @param  None
 * @retval None
 */
void chain_encoder_reset_value_handle(void)
{
    // Reset the encoder value to 0
    g_encoder_count = 0;

    s_return_status = OPERATION_SUCCESS;

    // Check if the encoder value was successfully reset
    if (g_encoder_count == 0) {
        s_return_status = OPERATION_SUCCESS;  // Reset successful
    } else {
        s_return_status = OPERATION_FAIL;  // Reset failed
    }

    // Return the result of the reset operation
    chain_command_complete_return(CHAIN_ENCODER_RESET_VALUE, &s_return_status, 1);
}

/**
 * @brief  Reset the incremental encoder value
 * @note   Reset the incremental encoder value to zero.
 *
 * @param  None
 * @retval None
 */
void chain_encoder_reset_inc_value_handle(void)
{
    // Reset the incremental encoder value to 0
    g_increment_count = 0;
    s_return_status   = OPERATION_SUCCESS;

    // Check if the incremental value was successfully reset
    if (g_increment_count == 0) {
        s_return_status = OPERATION_SUCCESS;  // Reset successful
    } else {
        s_return_status = OPERATION_FAIL;  // Reset failed
    }

    // Return the result of the reset operation
    chain_command_complete_return(CHAIN_ENCODER_RESET_INC_VALUE, &s_return_status, 1);
}

/**
 * @brief  Set the AB status
 * @note   Set the encoder direction based on the provided status.
 *         Valid statuses are ENCODER_AB and ENCODER_BA.
 *
 * @param  status Status value indicating encoder direction
 * @param  flag A flag indicating whether to save the status to memory.
 *             1 means the status will be saved to memory,
 *             0 means the status will not be saved to memory
 * @retval None
 */
void chain_encoder_set_ab_status_handle(uint8_t status, uint8_t flag)
{
    s_return_status = OPERATION_SUCCESS;

    // Check if the provided status value is valid
    if (status == ENCODER_AB || status == ENCODER_BA) {
        if (flag && status != get_ab_status()) {
            set_ab_status(status);  // Call function to set status
        }
        g_encoder_direct = status;  // Update encoder direction

        s_return_status = OPERATION_SUCCESS;  // Set successful

    } else {
        s_return_status = OPERATION_FAIL;  // Set failed
    }

    // Return the result of the set operation
    chain_command_complete_return(CHAIN_ENCODER_SET_AB_STATUS, &s_return_status, 1);
}

/**
 * @brief  Get the AB status
 * @note   Retrieve the current encoder direction.
 *
 * @param  None
 * @retval None
 */
void chain_encoder_get_ab_status_handle(void)
{
    // Return the current encoder direction
    chain_command_complete_return(CHAIN_ENCODER_GET_AB_STATUS, (uint8_t *)&g_encoder_direct, 1);
}
