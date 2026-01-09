/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "angle_function.h"

// Angle ADC Value
static uint16_t s_angle_adc_12value = 0;  // Variable to hold the latest 12-bit ADC value for angle measurement
static uint8_t s_angle_adc_8value   = 0;  // Variable to hold the latest 8-bit mapped ADC value for angle measurement
static uint8_t s_return_status =
    0;  // Variable to indicate the return status of operations (1 for success, 0 for failure)
extern uint8_t
    g_clockwise_status;  // External variable to store the current direction status (clockwise or counter-clockwise)

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

/**
 * @brief Retrieve ADC values and calculate angle readings.
 * @note This function performs multiple ADC conversions to average the result.
 *       It updates the global 12-bit and 8-bit angle values based on the current
 *       clockwise status.
 *
 * @param None
 * @retval None
 */
void adc_value_update(void)
{
    uint8_t adc_convert_success_nums = 0;  ///< Counter for successful ADC conversions
    uint32_t adc_sum                 = 0;  ///< Sum of successful ADC readings

    for (uint8_t i = 0; i < 10; i++) {
        HAL_ADCEx_Calibration_Start(&hadc1);  // Start ADC calibration
        HAL_ADC_Start(&hadc1);                // Start ADC conversion
        if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK) {
            if (HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc1), HAL_ADC_STATE_REG_EOC)) {
                adc_sum += HAL_ADC_GetValue(&hadc1);  // Accumulate ADC value
                adc_convert_success_nums++;           // Increment success counter
            }
        }
        HAL_ADC_Stop(&hadc1);  // Stop ADC conversion
    }

    if (adc_convert_success_nums > 0) {
        // Calculate the average ADC value and map to 8-bit based on clockwise status
        s_angle_adc_12value = adc_sum / adc_convert_success_nums;  // Average value
        s_angle_adc_8value  = (g_clockwise_status == CHAIN_ANGLE_CLOCKWISE_DEC) ? map12_to8(s_angle_adc_12value)
                                                                                : (255 - map12_to8(s_angle_adc_12value));

        if (g_clockwise_status != CHAIN_ANGLE_CLOCKWISE_DEC) {
            s_angle_adc_12value = 4095 - s_angle_adc_12value;  // Invert 12-bit value for counter-clockwise
        }
    }
}

/**
 * @brief Get the current 12-bit angle ADC value and return it.
 * @note This function retrieves the ADC value and sends it back through a chain command.
 *
 * @param None
 * @retval None
 */
void chain_angle_get_12value(void)
{
    chain_command_complete_return(CHAIN_ANGLE_GET_12ADC, (uint8_t *)&s_angle_adc_12value, 2);
}

/**
 * @brief Get the current 8-bit angle ADC value and return it.
 * @note This function retrieves the ADC value and sends it back through a chain command.
 *
 * @param None
 * @retval None
 */
void chain_angle_get_8value(void)
{
    chain_command_complete_return(CHAIN_ANGLE_GET_8ADC, (uint8_t *)&s_angle_adc_8value, 1);
}

/**
 * @brief Set the clockwise direction status.
 * @note This function updates the clockwise status and sends back the result of the operation.
 *
 * @param sta The desired status to set (either CHAIN_ANGLE_CLOCKWISE_DEC or CHAIN_ANGLE_CLOCKWISE_INC).
 * @param flag A flag indicating whether to save the status to memory.
 *             1 means the status will be saved to memory,
 *             0 means the status will not be saved to memory
 * @retval None
 */
void chain_angle_set_clockwise(uint8_t sta, uint8_t flag)
{
    s_return_status = 1;
    if (sta == CHAIN_ANGLE_CLOCKWISE_DEC || sta == CHAIN_ANGLE_CLOCKWISE_INC) {
        if (flag && sta != get_clockwise_status()) {
            set_clockwise_status(sta);  // Persist status to flash
        }
        g_clockwise_status = sta;  // Update the global clockwise status
        s_return_status    = 1;    // Success
    } else {
        s_return_status = 0;  // Failure
    }
    chain_command_complete_return(CHAIN_ANGLE_SET_CLOCKWISE_STATUS, (uint8_t *)&s_return_status, 1);
}

/**
 * @brief Get the current clockwise direction status.
 * @note This function retrieves the current clockwise status and sends it back through a chain command.
 *
 * @param None
 * @retval None
 */
void chain_angle_get_clockwise(void)
{
    chain_command_complete_return(CHAIN_ANGLE_GET_CLOCKWISE_STATUS, (uint8_t *)&g_clockwise_status, 1);
}
