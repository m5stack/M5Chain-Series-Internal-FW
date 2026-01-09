/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "joystick_function.h"
#include <stdio.h>

__IO static uint32_t s_last_interrupt_time               = 0;    // Last interrupt time (in milliseconds)
__IO static uint16_t s_adc_16value_buf[ADC_CHANNEL_NUMS] = {0};  // Buffer for 16-bit ADC values
__IO static uint8_t s_adc_8value_buf[ADC_CHANNEL_NUMS]   = {0};  // Buffer for 8-bit ADC values
__IO static uint32_t s_adc_collect_buf[ADC_CHANNEL_NUMS] = {0};  // Buffer for collecting raw ADC values
__IO uint16_t g_adc_map_range_value[8]                   = {0};  // Global mapped range values for ADC
__IO static int16_t s_adc_map_16value[ADC_CHANNEL_NUMS]  = {0};  // Buffer for mapped 16-bit ADC values
__IO static int8_t s_adc_map_8value[ADC_CHANNEL_NUMS]    = {0};  // Buffer for mapped 8-bit ADC values

__IO static uint32_t s_last_press_time = 0;  // Store the last press time in milliseconds

#if DEBUG
static uint8_t s_ret_buf[256] = {0};  // Return buffer size for debug mode
#else
static uint8_t s_ret_buf[12] = {0};  // Return buffer size for normal mode
#endif

static uint8_t s_ret_buf_size = 0;    // Size of the return buffer
adc_cal_t g_adc_cal_value     = {0};  // Global ADC calibration values

static uint16_t s_key_status = 0;  // Status of the joystick key/button

/**
 * @brief Maps a number from one range to another.
 * @note This function takes a value 'x' and maps it from the input range
 *       defined by 'in_min' and 'in_max' to the output range defined by
 *       'out_min' and 'out_max'.
 *
 * @param x The value to map.
 * @param in_min The lower bound of the input range.
 * @param in_max The upper bound of the input range.
 * @param out_min The lower bound of the output range.
 * @param out_max The upper bound of the output range.
 * @retval The mapped value within the output range.
 */
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    long result;

    // Perform the mapping calculation
    result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

    // Clamp the result to ensure it's within the output range
    if (result < out_min) {
        result = out_min;  // Set to out_min if below range
    } else if (result > out_max) {
        result = out_max;  // Set to out_max if above range
    }

    return result;  // Return the mapped value
}

/**
 * @brief Updates the ADC calibration values based on the current mapped range.
 * @note This function retrieves the current mapped range values from
 *       'g_adc_map_range_value' and updates the corresponding fields
 *       in the global ADC calibration structure 'g_adc_cal_value'.
 *
 * @param None
 * @retval None
 */
void update_adc_cal_value(void)
{
    // Update the ADC calibration values from the mapped range
    g_adc_cal_value.x_negative_min_adc_16bits = g_adc_map_range_value[0];
    g_adc_cal_value.x_negative_max_adc_16bits = g_adc_map_range_value[1];
    g_adc_cal_value.x_positive_min_adc_16bits = g_adc_map_range_value[2];
    g_adc_cal_value.x_positive_max_adc_16bits = g_adc_map_range_value[3];
    g_adc_cal_value.y_negative_min_adc_16bits = g_adc_map_range_value[4];
    g_adc_cal_value.y_negative_max_adc_16bits = g_adc_map_range_value[5];
    g_adc_cal_value.y_positive_min_adc_16bits = g_adc_map_range_value[6];
    g_adc_cal_value.y_positive_max_adc_16bits = g_adc_map_range_value[7];
}

/**
 * @brief ADC conversion complete callback.
 * @note This function is called when the ADC conversion is complete.
 *       It updates the ADC values and maps them based on the calibration
 * values.
 *
 * @param hadc Pointer to the ADC handle.
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    HAL_ADC_Stop_DMA(hadc);  // Stop DMA transfer

    // Update ADC values from the collected buffer
    for (uint8_t i = 0; i < ADC_CHANNEL_NUMS; i++) {
        s_adc_16value_buf[i] = (65535 - s_adc_collect_buf[i]);               // Copy raw ADC values
        s_adc_8value_buf[i]  = map(s_adc_16value_buf[i], 0, 65535, 0, 255);  // Map to 8-bit value
    }

    // Map X-axis values
    if (s_adc_16value_buf[AXIS_X] <= g_adc_cal_value.x_negative_max_adc_16bits) {
        s_adc_map_16value[AXIS_X] = map(s_adc_16value_buf[AXIS_X], g_adc_cal_value.x_negative_min_adc_16bits,
                                        g_adc_cal_value.x_negative_max_adc_16bits, -4095, 0);
        s_adc_map_8value[AXIS_X]  = map(s_adc_16value_buf[AXIS_X], g_adc_cal_value.x_negative_min_adc_16bits,
                                        g_adc_cal_value.x_negative_max_adc_16bits, -127, 0);
    } else if (s_adc_16value_buf[AXIS_X] >= g_adc_cal_value.x_positive_min_adc_16bits) {
        s_adc_map_16value[AXIS_X] = map(s_adc_16value_buf[AXIS_X], g_adc_cal_value.x_positive_min_adc_16bits,
                                        g_adc_cal_value.x_positive_max_adc_16bits, 0, 4095);
        s_adc_map_8value[AXIS_X]  = map(s_adc_16value_buf[AXIS_X], g_adc_cal_value.x_positive_min_adc_16bits,
                                        g_adc_cal_value.x_positive_max_adc_16bits, 0, 127);
    } else {
        s_adc_map_16value[AXIS_X] = 0;  // Default value if out of range
        s_adc_map_8value[AXIS_X]  = 0;  // Default value if out of range
    }

    // Map Y-axis values
    if (s_adc_16value_buf[AXIS_Y] <= g_adc_cal_value.y_negative_max_adc_16bits) {
        s_adc_map_16value[AXIS_Y] = map(s_adc_16value_buf[AXIS_Y], g_adc_cal_value.y_negative_min_adc_16bits,
                                        g_adc_cal_value.y_negative_max_adc_16bits, -4095, 0);
        s_adc_map_8value[AXIS_Y]  = map(s_adc_16value_buf[AXIS_Y], g_adc_cal_value.y_negative_min_adc_16bits,
                                        g_adc_cal_value.y_negative_max_adc_16bits, -127, 0);
    } else if (s_adc_16value_buf[AXIS_Y] >= g_adc_cal_value.y_positive_min_adc_16bits) {
        s_adc_map_16value[AXIS_Y] = map(s_adc_16value_buf[AXIS_Y], g_adc_cal_value.y_positive_min_adc_16bits,
                                        g_adc_cal_value.y_positive_max_adc_16bits, 0, 4095);
        s_adc_map_8value[AXIS_Y]  = map(s_adc_16value_buf[AXIS_Y], g_adc_cal_value.y_positive_min_adc_16bits,
                                        g_adc_cal_value.y_positive_max_adc_16bits, 0, 127);
    } else {
        s_adc_map_16value[AXIS_Y] = 0;  // Default value if out of range
        s_adc_map_8value[AXIS_Y]  = 0;  // Default value if out of range
    }

    // Restart ADC DMA for the next conversion
    HAL_ADC_Start_DMA(hadc, (uint32_t *)s_adc_collect_buf, ADC_CHANNEL_NUMS);
}

/**
 * @brief Initializes the joystick and ADC.
 * @note This function starts the ADC calibration and initiates DMA for ADC
 * conversion.
 *
 * @param None
 * @retval None
 */
void joystick_init(void)
{
    HAL_ADCEx_Calibration_Start(&hadc1);  // Start ADC calibration
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)s_adc_collect_buf,
                      ADC_CHANNEL_NUMS);  // Start ADC with DMA
}

/**
 * @brief Sends the 16-bit ADC values for the joystick.
 * @note This function either transmits the ADC values over USART in debug mode
 *       or returns them through a command completion callback in normal mode.
 *
 * @param None
 * @retval None
 */
void chain_joystick_get_16adc(void)
{
#if DEBUG
    // Clear the return buffer
    s_ret_buf[0] = '\0';

    // Format the 16-bit ADC values into the return buffer
    sprintf(s_ret_buf, "x_16adc: %d   y_16adc: %d\n", s_adc_16value_buf[0], s_adc_16value_buf[1]);

    // Transmit the formatted string via USART
    usart1_transmit_dma((uint8_t *)s_ret_buf, strlen(s_ret_buf));
#else
    // Return the ADC values through a command completion callback
    chain_command_complete_return(CHAIN_JOYSTICK_GET_16ADC, (uint8_t *)s_adc_16value_buf, 4);
#endif
}

/**
 * @brief Sends the 8-bit ADC values for the joystick.
 * @note This function either transmits the ADC values over USART in debug mode
 *       or returns them through a command completion callback in normal mode.
 *
 * @param None
 * @retval None
 */
void chain_joystick_get_8adc(void)
{
#if DEBUG
    // Clear the return buffer
    s_ret_buf[0] = '\0';

    // Format the 8-bit ADC values into the return buffer
    sprintf(s_ret_buf, "x_8adc: %d   y_8adc: %d\n", s_adc_8value_buf[0], s_adc_8value_buf[1]);

    // Transmit the formatted string via USART
    usart1_transmit_dma((uint8_t *)s_ret_buf, strlen(s_ret_buf));
#else
    // Return the ADC values through a command completion callback
    chain_command_complete_return(CHAIN_JOYSTICK_GET_8ADC, (uint8_t *)s_adc_8value_buf, 2);
#endif
}

/**
 * @brief Sends the mapped range values for the ADC.
 * @note This function either transmits the mapped range values over USART in
 * debug mode
 *       or returns them through a command completion callback in normal mode.
 *
 * @param None
 * @retval None
 */
void chain_joystick_get_adc_xy_mapped_range(void)
{
#if DEBUG
    // Clear the return buffer
    s_ret_buf[0] = '\0';

    // Start building the mapped range string
    sprintf(s_ret_buf, "mapped_range: ");

    // Append each mapped range value to the return buffer
    for (uint8_t i = 0; i < 8; i++) {
        char temp[10];  // Temporary buffer for each value
        sprintf(temp, "%d ", g_adc_map_range_value[i]);
        strcat(s_ret_buf, temp);  // Append the value to the main buffer
    }

    // Transmit the formatted string via USART
    usart1_transmit_dma((uint8_t *)s_ret_buf, strlen(s_ret_buf));
#else
    // Return the mapped range values through a command completion callback
    chain_command_complete_return(CHAIN_JOYSTICK_GET_ADC_XY_MAPPED_RANGE, (uint8_t *)g_adc_map_range_value, 16);
#endif
}

/**
 * @brief Sets the ADC mapped range for the joystick.
 * @note This function updates the global mapped range values based on the
 * provided buffer.
 *       If the size is 16 bytes, the mapping is successful; otherwise, it
 * indicates an error.
 *
 * @param buffer Pointer to the buffer containing the new mapped range values.
 * @param size Size of the buffer.
 * @retval None
 */
void chain_joystick_set_adc_xy_mapped_range(uint8_t *buffer, uint16_t size)
{
#if DEBUG
    // Clear the return buffer
    s_ret_buf[0] = '\0';

    // Check if the buffer size is correct
    if (size == 17) {
        for (uint8_t i = 0; i < 16; i += 2) {
            g_adc_map_range_value[i / 2] = ((uint16_t)buffer[i + 1] << 8) | (uint16_t)buffer[i];
        }
        update_adc_cal_value();  // Update calibration values
        set_map_range(g_adc_map_range_value,
                      sizeof(g_adc_map_range_value));  // Store in flash
        sprintf(s_ret_buf, "set success\n");
    } else {
        sprintf(s_ret_buf, "set error\n");
    }

    // Transmit the status message via USART
    usart1_transmit_dma((uint8_t *)s_ret_buf, strlen(s_ret_buf));
#else
    // If the size is correct, update the mapped range values
    if (size == 17) {
        for (uint8_t i = 0; i < 16; i += 2) {
            g_adc_map_range_value[i / 2] = ((uint16_t)buffer[i + 1] << 8) | (uint16_t)buffer[i];
        }
        update_adc_cal_value();  // Update calibration values
        if (buffer[16]) {
            set_map_range(g_adc_map_range_value,
                          sizeof(g_adc_map_range_value));  // Store in flash
        }

        // Prepare success response
        s_ret_buf_size              = 0;
        s_ret_buf[s_ret_buf_size++] = OPERATION_SUCCESS;
        chain_command_complete_return(CHAIN_JOYSTICK_SET_ADC_XY_MAPPED_RANGE, s_ret_buf, s_ret_buf_size);
    } else {
        // Prepare failure response
        s_ret_buf_size              = 0;
        s_ret_buf[s_ret_buf_size++] = OPERATION_FAIL;
        chain_command_complete_return(CHAIN_JOYSTICK_SET_ADC_XY_MAPPED_RANGE, s_ret_buf, s_ret_buf_size);
    }
#endif
}

/**
 * @brief Gets the mapped 16-bit ADC values for the joystick.
 * @note This function either transmits the mapped values over USART in debug
 * mode
 *       or returns them through a command completion callback in normal mode.
 *
 * @param None
 * @retval None
 */
void chain_joystick_get_adc_xy_mapped_int16_value(void)
{
#if DEBUG
    // Clear the return buffer
    s_ret_buf[0] = '\0';

    // Format the mapped values into the return buffer
    sprintf(s_ret_buf, "x_map_16value: %d   y_map_16value: %d\n", s_adc_map_16value[0], s_adc_map_16value[1]);

    // Transmit the formatted string via USART
    usart1_transmit_dma((uint8_t *)s_ret_buf, strlen(s_ret_buf));
#else
    // Return the mapped 16-bit values through a command completion callback
    chain_command_complete_return(CHAIN_JOYSTICK_GET_ADC_XY_MAPPED_INT16_VALUE, (uint8_t *)s_adc_map_16value, 4);
#endif
}

/**
 * @brief Gets the mapped 8-bit ADC values for the joystick.
 * @note This function either transmits the mapped values over USART in debug
 * mode
 *       or returns them through a command completion callback in normal mode.
 *
 * @param None
 * @retval None
 */
void chain_joystick_get_adc_xy_mapped_int8_value(void)
{
#if DEBUG
    // Clear the return buffer
    s_ret_buf[0] = '\0';

    // Format the mapped values into the return buffer
    sprintf(s_ret_buf, "x_map_8value: %d   y_map_8value: %d\n", s_adc_map_8value[0], s_adc_map_8value[1]);

    // Transmit the formatted string via USART
    usart1_transmit_dma((uint8_t *)s_ret_buf, strlen(s_ret_buf));
#else
    // Return the mapped 8-bit values through a command completion callback
    chain_command_complete_return(CHAIN_JOYSTICK_GET_ADC_XY_MAPPED_INT8_VALUE, (uint8_t *)s_adc_map_8value, 2);
#endif
}
