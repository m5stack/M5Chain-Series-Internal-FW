/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "tim.h"
#include "RGB.h"
#include <stdlib.h>

__IO uint8_t event_overlay[3] = {0};
__IO uint8_t fade_type        = 0;
__IO uint8_t fade_frames_left = 0;

static uint8_t s_led_num         = 0;
static uint32_t *s_color_buf     = NULL;
static uint8_t s_ret_buf[20]     = {0};
static uint8_t s_ret_buf_size    = 0;
static uint8_t s_rgb_color[3]    = {0};
static uint8_t s_rgb_update_flag = 0;
static uint8_t fade_flag         = 0;

#define gpio_low()  GPIOA->BRR = GPIO_PIN_8
#define gpio_high() GPIOA->BSRR = GPIO_PIN_8
#define delay_150ns() \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP")
#define delay_300ns() \
    delay_150ns();    \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP");     \
    __asm("NOP")
#define delay_600ns() \
    delay_300ns();    \
    delay_300ns();    \
    __asm("NOP");     \
    __asm("NOP")
#define delay_900ns() \
    delay_600ns();    \
    delay_300ns()

#define out_bit_low() \
    gpio_high();      \
    delay_300ns();    \
    gpio_low();       \
    delay_900ns()

#define out_bit_high() \
    gpio_high();       \
    delay_600ns();     \
    gpio_low();        \
    delay_600ns()

#define restart()                           \
    do {                                    \
        for (uint8_t i = 0; i < 255; i++) { \
            delay_900ns();                  \
        }                                   \
    } while (0)

static uint8_t sat_add(uint8_t a, uint8_t b)
{
    uint16_t tmp = a + b;
    return (tmp > 255) ? 255 : tmp;
}

static uint8_t sat_sub(uint8_t a, uint8_t b)
{
    return (a > b) ? (a - b) : 0;
}

/**
 * @brief Initialize GPIO settings for the LED control.
 * @note This function configures GPIO pin PA8 as a push-pull output
 *       without internal pull-up or pull-down resistors and sets the
 *       output speed to medium frequency.
 *
 * @param None
 * @retval None
 */
void GPIO_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};                     // Structure to hold GPIO initialization parameters
    GPIO_InitStruct.Pin              = GPIO_PIN_8;              // Specify the GPIO pin to configure
    GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT_PP;     // Set pin mode to output push-pull
    GPIO_InitStruct.Pull             = GPIO_NOPULL;             // No internal pull-up or pull-down
    GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_MEDIUM;  // Set speed to medium frequency
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);                     // Initialize GPIO with the specified settings
}

/**
 * @brief Send RGB color data to the LED.
 * @note This function sends 24 bits of color data to the LED, one bit at
 *       a time. It uses bit manipulation to determine whether to set the
 *       output high or low based on the color value.
 *
 * @param color 32-bit integer representing the RGB color value
 * @retval None
 */
void rgb_send_data(uint32_t color)
{
    for (uint8_t i = 0; i < 24; i++)  // Loop through each bit of the 24-bit color value
    {
        if (color & (1 << (23 - i)))  // Check if the current bit is set
        {
            out_bit_high();  // Set the output high if the bit is 1
        } else {
            out_bit_low();  // Set the output low if the bit is 0
        }
    }
}

/**
 * @brief Set the color value in the color buffer.
 * @note This function updates the specified index in the color buffer
 *       with the new color value. It allows for setting multiple colors
 *       in the buffer, which can later be used for LED display.
 *
 * @param num Index in the color buffer to set the color
 * @param color 32-bit integer representing the RGB color value to set
 * @retval None
 */
void set_color(uint8_t num, uint32_t color)
{
    s_color_buf[num] = color;  // Update the color buffer at the specified index
}

/**
 * @brief Display the current RGB values on the LED.
 * @note This function disables interrupts, sends the current RGB color data
 *       to the LEDs, and then re-enables interrupts. It also calls the
 *       `restart` function to refresh the LED display.
 *
 * @param None
 * @retval None
 */
void rgb_show(void)
{
    __disable_irq();  // Disable interrupts for safe operation
    for (uint8_t i = 0; i < s_led_num; i++) {
        rgb_send_data(s_color_buf[i]);  // Send RGB data to each LED
    }
    __enable_irq();  // Re-enable interrupts after sending data
    restart();       // Refresh the LED display
}

/**
 * @brief Convert RGB color values to a format suitable for transmission.
 * @note This function calculates the RGB values based on the current
 *       brightness level (`g_light`) and returns a combined 32-bit integer
 *       representing the RGB color.
 *
 * @param None
 * @retval uint32_t Combined RGB value as a single 32-bit integer
 */
uint32_t rgb_value_convert(void)
{
    uint8_t buf[3] = {0};  // Buffer to hold adjusted RGB values
    for (uint8_t i = 0; i < 3; i++) {
        buf[i] = s_rgb_color[i] * (g_light / 100.0);  // Adjust RGB values based on brightness
        if (fade_flag) {
            buf[i] = sat_add(buf[i], event_overlay[i]);
        }
    }

    return (uint32_t)buf[2] | ((uint32_t)buf[0] << 8) | ((uint32_t)buf[1] << 16);  // Combine RGB values
}

/**
 * @brief Initialize RGB settings and allocate resources.
 * @note This function initializes the GPIO settings, restarts the LED display,
 *       allocates memory for the color buffer, and sets the number of LEDs.
 *
 * @param None
 * @retval None
 */
void rgb_init(void)
{
    GPIO_init();                                                  // Initialize GPIO settings
    restart();                                                    // Restart the LED display
    s_color_buf = (uint32_t *)calloc(RGB_NUM, sizeof(uint32_t));  // Allocate memory for color buffer
    s_led_num   = RGB_NUM;                                        // Set the number of LEDs
    rgb_send_data(0x000000);
}

/**
 * @brief Set RGB values based on the provided buffer.
 * @param buffer Pointer to an array containing RGB values [index, num, R, G, B].
 * @param size The size of the RGB data (should be 5 for complete RGB command).
 * @retval None
 */
void chain_set_rgb_value(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    // Validate input and check buffer format
    if (buffer != NULL && size == 5 && buffer[0] == 0 && buffer[1] == RGB_NUM) {
        // Update RGB color values
        s_rgb_color[0] = buffer[2];
        s_rgb_color[1] = buffer[3];
        s_rgb_color[2] = buffer[4];

        s_rgb_update_flag = 1;

        operation_result = OPERATION_SUCCESS;
    }

    // Send response
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(CHAIN_SET_RGB_VALUE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get the current RGB values
 * @param buffer Pointer to command buffer [index, num]
 * @param size The size of the command buffer (should be 2)
 * @retval None
 */
void chain_get_rgb_value(uint8_t *buffer, uint8_t size)
{
    uint8_t operation_result = OPERATION_FAIL;

    // Validate input and check buffer format
    if (buffer != NULL && size == 2 && buffer[0] == 0 && buffer[1] == RGB_NUM) {
        operation_result = OPERATION_SUCCESS;
    }

    // Prepare response buffer
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = operation_result;

    // Include RGB values only if operation succeeded
    if (operation_result == OPERATION_SUCCESS) {
        s_ret_buf[s_ret_buf_size++] = s_rgb_color[0];
        s_ret_buf[s_ret_buf_size++] = s_rgb_color[1];
        s_ret_buf[s_ret_buf_size++] = s_rgb_color[2];
    }

    // Send command complete response
    chain_command_complete_return(CHAIN_GET_RGB_VALUE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief  Set the brightness of the RGB light
 * @param  g_light_value New brightness value, range from 0 to 100
 * @param  flag A flag indicating whether to save the status to memory.
 *             1 means the status will be saved to memory,
 *             0 means the status will not be saved to memory
 * @retval None
 */
void chain_set_light_value(uint8_t g_light_value, uint8_t flag)
{
    // Check if the new brightness value is within the valid range (0-100)
    if (g_light_value <= 100) {
        // Set the brightness of the RGB light
        if (flag && g_light_value != get_rgb_light()) {
            set_rgb_light(g_light_value);
        }
        if (g_light != g_light_value) {
            // Update the global brightness value
            g_light           = g_light_value;
            s_rgb_update_flag = 1;
        }

        // Clear the return buffer and mark the operation as successful
        s_ret_buf_size              = 0;
        s_ret_buf[s_ret_buf_size++] = OPERATION_SUCCESS;

        // Send command complete response indicating success
        chain_command_complete_return(CHAIN_SET_RGB_LIGHT, s_ret_buf, s_ret_buf_size);
    } else {
        // Clear the return buffer and mark the operation as failed
        s_ret_buf_size              = 0;
        s_ret_buf[s_ret_buf_size++] = OPERATION_FAIL;

        // Send command complete response indicating failure
        chain_command_complete_return(CHAIN_SET_RGB_LIGHT, s_ret_buf, s_ret_buf_size);
    }
}

/**
 * @brief Retrieve the current RGB light brightness value.
 * @note This function sends the current brightness level of the RGB light
 *       to the command completion handler using the `chain_command_complete_return` function.
 *
 * @param None
 * @retval None
 */
void chain_get_light_value(void)
{
    s_ret_buf_size              = 0;        // Reset response buffer size
    s_ret_buf[s_ret_buf_size++] = g_light;  // Add the current brightness value
    // Return the command completion response
    chain_command_complete_return(CHAIN_GET_RGB_LIGHT, s_ret_buf, s_ret_buf_size);
}

void rgb_update(void)
{
    static uint32_t last_step_tick = 0;

    uint32_t now = HAL_GetTick();

    if (fade_frames_left > 0 && (now - last_step_tick) >= RGB_FADE_MS_STEP) {
        last_step_tick = now;
        if (fade_type == 1) {  // R
            event_overlay[0] = (uint8_t)(((fade_frames_left - 1) * RGB_FADE_MAX) / (RGB_FADE_FRAMES - 1));
            event_overlay[1] = 0;
            event_overlay[2] = 0;
        } else if (fade_type == 2) {  // G
            event_overlay[1] = (uint8_t)(((fade_frames_left - 1) * RGB_FADE_MAX) / (RGB_FADE_FRAMES - 1));
            event_overlay[0] = 0;
            event_overlay[2] = 0;
        } else if (fade_type == 3) {  // B
            event_overlay[2] = (uint8_t)(((fade_frames_left - 1) * RGB_FADE_MAX) / (RGB_FADE_FRAMES - 1));
            event_overlay[0] = 0;
            event_overlay[1] = 0;
        }
        fade_frames_left--;
        if (fade_frames_left == 0) {
            event_overlay[0] = 0;
            event_overlay[1] = 0;
            event_overlay[2] = 0;
            fade_type        = 0;
        }
        fade_flag = 1;
    }

    if (fade_flag || s_rgb_update_flag) {
        set_color(0, rgb_value_convert());  // Set the RGB color based on the current value
        rgb_show();                         // Display the RGB light with updated settings
        s_rgb_update_flag = 0;
        fade_flag         = 0;
    }
}
