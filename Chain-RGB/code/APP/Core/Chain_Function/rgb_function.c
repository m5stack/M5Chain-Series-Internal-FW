/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include <rgb_function.h>
#include "tim.h"
#include "stdio.h"
#include <string.h>
#include <font5x7.h>
#include <myflash.h>

__IO uint8_t g_send_complete_flag = 1;

RGB_Color_TypeDef event_overlay = {0, 0, 0};
__IO uint8_t fade_type          = 0;
__IO uint8_t fade_frames_left   = 0;

uint8_t s_rgb_buf[RGB_NUM + 10][24];  // 2D array to store the final PWM output array; each row contains 24 data
                                      // representing one LED, with the last row as 24 zeros representing the RESET code
uint8_t s_rgb_update_flag = 0;

/** @brief Current display mode */
static rgb_mode_t rgb_mode = RGB_PIXEL_MODE;

/** @brief Return status for command operations */
static uint8_t s_return_status = 0;

/** @brief Return buffer size for command responses */
static uint8_t s_ret_buf_size = 0;

/** @brief Display buffer (8 rows × 8 columns) */
static uint16_t display_buffer[8][8] = {0};

/** @brief Current screen rotation angle */
chain_rgb_rotation_t g_current_rotation = RGB_ROTATION_0;

/** @brief Virtual display buffer for scrolling text (bit-level storage) */
static uint8_t virtual_buf[8][VBUF_W / 8];

/** @brief 5x7 font definition */
const font_t Font5x7 = {.width = FONT5X7_WIDTH, .height = FONT5X7_HEIGHT, .spacing = 1, .data = font5x7};

/** @brief Scroll context structure */
static scroll_ctx_t scroll;

/**
 * @brief Activate DMA for TIM1 to control RGB LED timing.
 * @note This function sets up the DMA for data transmission
 *       to control RGB LEDs using TIM1, configuring the necessary
 *       parameters and enabling interrupts.
 *
 * @param None
 * @retval None
 */
static void activate_tim1_dma(void)
{
    // Set the DMA data length for RGB_NUM LEDs, each requiring 24 bits.
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, (RGB_NUM + 10) * 24);
    // Set the memory address for the DMA buffer.
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)s_rgb_buf);
    // Set the peripheral address to the TIM1 capture/compare register.
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)&(TIM1->CCR1));
    // Clear the global interrupt flag for channel 5.
    LL_DMA_ClearFlag_GI5(DMA1);
    // Clear the transfer complete interrupt flag for channel 5.
    LL_DMA_ClearFlag_TC5(DMA1);
    // Enable the transfer complete interrupt for channel 5.
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_5);
    // Enable DMA requests for TIM1 channel 1.
    LL_TIM_EnableDMAReq_CC1(TIM1);
    // Enable all outputs of TIM1.
    LL_TIM_EnableAllOutputs(TIM1);
    // Set the TIM1 DMA request trigger to CC.
    LL_TIM_CC_SetDMAReqTrigger(TIM1, LL_TIM_CCDMAREQUEST_CC);
    // Enable TIM1 channel 1.
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);
}

/**
 * @brief Reset the RGB buffer for the next data load.
 * @note This function clears the last RGB LED buffer entry.
 *
 * @param None
 * @retval None
 */
static void reset_load(void)
{
    // Clear the RGB buffer for the last LED (RGB_NUM) by setting all bits to 0.
    for (uint8_t i = 0; i < 24; i++) {
        s_rgb_buf[RGB_NUM][i] = 0;
    }
}

/**
 * @brief Send an array to control the RGB LED.
 * @note This function configures the DMA for data transmission
 *       and starts the TIM1 counter for the RGB LED control.
 *
 * @param None
 * @retval None
 */
void rgb_send_array(void)
{
    // Set the data length for the DMA transfer (total bits to send)
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, (RGB_NUM + 10) * 24);

    // Enable the DMA channel for transferring the RGB data
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);

    // Start the timer to enable sending data to the RGB LEDs
    LL_TIM_EnableCounter(TIM1);
}

static void chain_rgb_setcolor(uint8_t led_id, uint16_t rgb565)
{
    uint8_t r5 = (rgb565 >> 11) & 0x1F;
    uint8_t g6 = (rgb565 >> 5) & 0x3F;
    uint8_t b5 = rgb565 & 0x1F;

    uint8_t R = (r5 << 1) | (r5 >> 4);  // 亮度限制在25%
    uint8_t G = g6;
    uint8_t B = (b5 << 1) | (b5 >> 4);

    R = R * (g_light * 0.006f);
    G = G * (g_light * 0.006f);
    B = B * (g_light * 0.006f);

    for (uint8_t i = 0; i < 8; i++) {
        s_rgb_buf[led_id][i]      = ((G & (1 << (7 - i))) ? CODE_1 : CODE_0);
        s_rgb_buf[led_id][i + 8]  = ((R & (1 << (7 - i))) ? CODE_1 : CODE_0);
        s_rgb_buf[led_id][i + 16] = ((B & (1 << (7 - i))) ? CODE_1 : CODE_0);
    }
}

/**
 * @brief Turn off all RGB LEDs.
 * @note This function sets the first LED to black (off), resets
 *       the load, and sends the updated array to turn off the LEDs.
 *
 * @param None
 * @retval None
 */
static void turn_off_all_handle(void)
{
    // Set the first RGB LED color to black (off)
    for (uint8_t i = 0; i < RGB_NUM; i++) {
        chain_rgb_setcolor(i, 0);
    }

    // Reset the load for the RGB LED control
    reset_load();

    // Send the updated RGB array to turn off the LEDs
    rgb_send_array();
}

/**
 * @brief Initialize the RGB LED.
 * @note This function activates DMA for TIM1 and turns off
 *       all RGB LEDs to initialize the system properly.
 *
 * @param None
 * @retval None
 */
void chain_rgb_init(void)
{
    // Activate DMA settings for TIM1 to prepare for RGB control
    activate_tim1_dma();

    // Turn off all RGB LEDs during initialization
    turn_off_all_handle();

    // Delay for a short period to allow for stabilization
    HAL_Delay(1);
}

/**
 * @brief Set the display operating mode
 * @param mode Display mode to set
 * @retval None
 */
void chain_rgb_set_mode(rgb_mode_t mode)
{
    if (rgb_mode == RGB_STRING_SCROLL_MODE && mode == RGB_PIXEL_MODE) {
        chain_rgb_scroll_text_control(SCROLL_CMD_STOP_CLEAR);
    }
    rgb_mode = mode;
}

/**
 * @brief Clear the display buffer
 * @param None
 * @retval None
 */
void chain_rgb_clear(void)
{
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            display_buffer[y][x] = 0;
        }
    }
    s_rgb_update_flag = 1;
}

/**
 * @brief Rotate coordinates based on current rotation setting
 * @note  Default rotation is 90 degrees (RGB_ROTATION_0)
 * @param x Input X coordinate
 * @param y Input Y coordinate
 * @param out_x Output X coordinate pointer
 * @param out_y Output Y coordinate pointer
 * @retval None
 */
static void chain_rgb_rotate_coordinate(uint8_t x, uint8_t y, uint8_t *out_x, uint8_t *out_y)
{
    switch (g_current_rotation) {
        case RGB_ROTATION_0:  // 90 degrees (new default)
            *out_x = 7 - y;
            *out_y = x;
            break;
        case RGB_ROTATION_90:  // 180 degrees
            *out_x = 7 - x;
            *out_y = 7 - y;
            break;
        case RGB_ROTATION_180:  // 270 degrees
            *out_x = y;
            *out_y = 7 - x;
            break;
        case RGB_ROTATION_270:  // 0 degrees (360 degrees)
            *out_x = x;
            *out_y = y;
            break;
        default:
            *out_x = 7 - y;  // 默认使用90度
            *out_y = x;
            break;
    }
}

/**
 * @brief Set LED state at specific position with rotation applied
 * @param x Column coordinate (0-7)
 * @param y Row coordinate (0-7)
 * @param color565 LED color in 565 format
 * @retval None
 */
void chain_rgb_set_pixel(uint8_t x, uint8_t y, uint16_t color565)
{
    uint8_t real_x, real_y;

    if (x < 8 && y < 8) {
        // Coordinate rotation transformation
        chain_rgb_rotate_coordinate(x, y, &real_x, &real_y);

        display_buffer[real_y][real_x] = color565;
        s_rgb_update_flag              = 1;
    }
}

/**
 * @brief Get LED state at specific position with rotation applied
 * @param x Column coordinate (0-7)
 * @param y Row coordinate (0-7)
 * @retval LED color in 565 format
 */
uint16_t chain_rgb_get_pixel(uint8_t x, uint8_t y)
{
    uint8_t real_x, real_y;

    if (x < 8 && y < 8) {
        chain_rgb_rotate_coordinate(x, y, &real_x, &real_y);
        return display_buffer[real_y][real_x];
    }
    return 0;
}

/**
 * @brief Display 8x8 grayscale pattern
 * @param pattern 8x8 grayscale array, each element 0~0xFFFF
 * @retval None
 */
void chain_rgb_display_pattern(const uint16_t pattern[8][8])
{
    // Clear buffer
    chain_rgb_clear();

    // Copy pixel by pixel into display buffer
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            chain_rgb_set_pixel(x, y, pattern[y][x]);
        }
    }
    s_rgb_update_flag = 1;
}

/**
 * @brief Set screen rotation angle
 * @param rotation Rotation angle (ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270)
 * @retval None
 */
void chain_rgb_set_rotation(chain_rgb_rotation_t rotation)
{
    if (rotation <= RGB_ROTATION_270) {
        uint8_t relative_rotation = (rotation + 4 - g_current_rotation) % 4;
        g_current_rotation        = (relative_rotation + 3) % 4;
        uint16_t temp[8][8];
        memcpy(temp, display_buffer, sizeof(temp));
        chain_rgb_display_pattern(temp);
        g_current_rotation = rotation;
        s_rgb_update_flag  = 1;
    }
}

/**
 * @brief Get current rotation angle
 * @param None
 * @retval Current rotation angle
 */
chain_rgb_rotation_t chain_rgb_get_rotation(void)
{
    return g_current_rotation;
}

/**
 * @brief Set display brightness level with gamma correction
 * @param brightness Brightness level (0-100)
 * @retval None
 */
void chain_rgb_set_brightness(uint8_t brightness)
{
    if (brightness > 100) brightness = 100;
    g_light           = brightness;
    s_rgb_update_flag = 1;
}

/**
 * @brief Get actual character width by removing trailing empty columns
 * @param ch Character to measure
 * @param font Font structure pointer
 * @retval Actual character width in columns
 */
static uint8_t chain_rgb_get_char_actual_width(char ch, const font_t *font)
{
    // Character range check
    if (ch < 32 || ch > 127) {
        ch = ' ';  // Treat unsupported characters as space
    }

    // Calculate character index in font data
    uint16_t index       = (ch - 32) * font->width;
    const uint8_t *glyph = &font->data[index];

    // Find first non-empty column from right to left
    for (int8_t col = font->width - 1; col >= 0; col--) {
        if (glyph[col] != 0) {
            return (uint8_t)(col + 1);  // Return actual width (columns + 1)
        }
    }

    return 1;  // Minimum width for space
}

/**
 * @brief Display single character at specified position
 * @param ch Character to display (ASCII 32-127)
 * @param x_offset X-axis offset (0-7)
 * @param y_offset Y-axis offset (0-7)
 * @param font Font structure pointer
 * @param color565 LED color in 565 format
 * @param clear_bg Clear background before displaying (0: no, 1: yes)
 * @retval None
 */
void chain_rgb_show_char(char ch, uint8_t x_offset, uint8_t y_offset, const font_t *font, uint16_t color565,
                         uint8_t clear_bg)
{
    // Character range check
    if (ch < 32 || ch > 127) {
        ch = ' ';  // Display unsupported characters as space
    }

    // Clear screen
    if (clear_bg) {
        chain_rgb_clear();
    }

    // Calculate character index in font data
    uint16_t index       = (ch - 32) * font->width;
    const uint8_t *glyph = &font->data[index];

    // Iterate through each column of the character
    for (uint8_t col = 0; col < font->width; col++) {
        uint8_t screen_x = x_offset + col;

        // X coordinate boundary check
        if (screen_x >= 8) break;

        // Get bitmap data for current column
        uint8_t bits = glyph[col];

        // Iterate through each row of the character
        for (uint8_t row = 0; row < font->height; row++) {
            uint8_t screen_y = y_offset + row;

            // Y coordinate boundary check
            if (screen_y >= 8) continue;

            // If bit is 1, light corresponding pixel
            if (bits & (1 << row)) {
                chain_rgb_set_pixel(screen_x, screen_y, color565);
            } else {
                chain_rgb_set_pixel(screen_x, screen_y, 0);
            }
        }
    }
}

/**
 * @brief Reset scroll coordinates based on direction and mode
 * @retval None
 */
static void chain_rgb_scroll_text_reset_coords(void)
{
    // Reset X-axis
    if (scroll.dir == SCROLL_LEFT) {
        scroll.step_x   = 1;
        scroll.offset_x = 0;
    } else if (scroll.dir == SCROLL_RIGHT) {
        scroll.step_x = -1;
        // In LOOP mode, initial position is 0 for seamless alignment
        // Non-LOOP mode usually sets to max_x
        if (scroll.mode == SCROLL_MODE_LOOP)
            scroll.offset_x = 0;
        else
            scroll.offset_x = scroll.max_x;
    }

    // Reset Y-axis
    if (scroll.dir == SCROLL_UP) {
        scroll.step_y   = 1;
        scroll.offset_y = 0;
    } else if (scroll.dir == SCROLL_DOWN) {
        scroll.step_y = -1;
        // Same logic
        if (scroll.mode == SCROLL_MODE_LOOP)
            scroll.offset_y = 0;
        else
            scroll.offset_y = scroll.max_y;
    }
}

/**
 * @brief Control scroll text state (start, pause, stop)
 * @param cmd Scroll control command
 * @retval None
 */
void chain_rgb_scroll_text_control(scroll_cmd_t cmd)
{
    switch (cmd) {
        case SCROLL_CMD_START:
            scroll.state = SCROLL_STATE_RUNNING;
            break;

        case SCROLL_CMD_PAUSE:
            // Only change state, frame remains static
            scroll.state = SCROLL_STATE_PAUSED;
            break;

        case SCROLL_CMD_STOP_CLEAR:
            // Set to stopped state
            scroll.state = SCROLL_STATE_IDLE;

            // Clear display buffer (black screen)
            memset(display_buffer, 0, sizeof(display_buffer));
            s_rgb_update_flag = 1;
            // Reset coordinates to initial position
            chain_rgb_scroll_text_reset_coords();
            break;
    }
}

/**
 * @brief Update scroll text display based on current state and mode
 * @param tick_ms Current system tick time in milliseconds
 * @retval None
 */
void chain_rgb_scroll_text_update(uint32_t tick_ms)
{
    if (scroll.state != SCROLL_STATE_RUNNING || rgb_mode != RGB_STRING_SCROLL_MODE) return;

    if (tick_ms - scroll.last_tick < scroll.speed_ms) return;

    scroll.last_tick = tick_ms;
    memset(display_buffer, 0, sizeof(display_buffer));

    // ================= LOOP Mode (Circular seamless scrolling) =================
    if (scroll.mode == SCROLL_MODE_LOOP) {
        // 1. Horizontal scrolling (Left / Right)
        if (scroll.dir == SCROLL_LEFT || scroll.dir == SCROLL_RIGHT) {
            for (uint8_t y = 0; y < 8; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    // (scroll.offset_x + x) may be negative, so handle it
                    int32_t target_idx = scroll.offset_x + x;

                    // Normalize index to [0, content_len-1]
                    while (target_idx < 0) target_idx += scroll.content_len;
                    while (target_idx >= scroll.content_len) target_idx -= scroll.content_len;

                    // Only get value if index is within valid buffer range
                    // If target_idx exceeds actual drawn character range (but within content_len), reads as 0 (black)
                    if (target_idx < VBUF_W) {
                        uint8_t byte_pos  = target_idx / 8;
                        uint8_t bit_pos   = target_idx % 8;
                        uint8_t bit_value = (virtual_buf[y][byte_pos] >> bit_pos) & 0x01;
                        if (bit_value) {
                            uint16_t pixel_color;
                            if (scroll.color565 == 0) {
                                // Rainbow gradient effect with continuous scrolling
                                uint16_t hue = (x + y + scroll.total_scroll_steps) *
                                               RAINBOW_HUE_MULTIPLIER;  // Use total steps for continuous gradient
                                hue %= 360;                             // Ensure hue is always in 0-360 range
                                uint8_t r, g, b;

                                // Simple HSV to RGB conversion for rainbow effect
                                if (hue < 60) {
                                    r = 255;
                                    g = (hue * 255) / 60;
                                    b = 0;
                                } else if (hue < 120) {
                                    r = ((120 - hue) * 255) / 60;
                                    g = 255;
                                    b = 0;
                                } else if (hue < 180) {
                                    r = 0;
                                    g = 255;
                                    b = ((hue - 120) * 255) / 60;
                                } else if (hue < 240) {
                                    r = 0;
                                    g = ((240 - hue) * 255) / 60;
                                    b = 255;
                                } else if (hue < 300) {
                                    r = ((hue - 240) * 255) / 60;
                                    g = 0;
                                    b = 255;
                                } else {
                                    r = 255;
                                    g = 0;
                                    b = ((360 - hue) * 255) / 60;
                                }

                                // Convert RGB888 to RGB565
                                pixel_color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                            } else {
                                pixel_color = scroll.color565;
                            }
                            chain_rgb_set_pixel(x, y, pixel_color);
                        } else {
                            chain_rgb_set_pixel(x, y, 0);
                        }
                    }
                }
            }

            // Update coordinates and loop
            scroll.offset_x += scroll.step_x;
            // Adjust total steps based on direction for consistent gradient
            if (scroll.dir == SCROLL_LEFT || scroll.dir == SCROLL_RIGHT) {
                scroll.total_scroll_steps += scroll.step_x;
            }

            // Normalize offset to prevent overflow (keep between 0 ~ content_len)
            if (scroll.offset_x >= scroll.content_len) {
                scroll.offset_x -= scroll.content_len;
            }
            if (scroll.offset_x < 0) {
                scroll.offset_x += scroll.content_len;
            }
        }
        // 2. Vertical scrolling (Up / Down)
        else {
            for (uint8_t y = 0; y < 8; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    int32_t target_idx = scroll.offset_y + y;

                    while (target_idx < 0) target_idx += scroll.content_len;
                    while (target_idx >= scroll.content_len) target_idx -= scroll.content_len;

                    // Vertical mode: virtual_buf[x][target_idx]
                    if (target_idx < VBUF_W) {
                        uint8_t byte_pos  = target_idx / 8;
                        uint8_t bit_pos   = target_idx % 8;
                        uint8_t bit_value = (virtual_buf[x][byte_pos] >> bit_pos) & 0x01;
                        if (bit_value) {
                            uint16_t pixel_color;
                            if (scroll.color565 == 0) {
                                // Rainbow gradient effect with continuous scrolling
                                uint16_t hue = (x + y + scroll.total_scroll_steps) *
                                               RAINBOW_HUE_MULTIPLIER;  // Use total steps for continuous gradient
                                hue %= 360;                             // Ensure hue is always in 0-360 range
                                uint8_t r, g, b;

                                // Simple HSV to RGB conversion for rainbow effect
                                if (hue < 60) {
                                    r = 255;
                                    g = (hue * 255) / 60;
                                    b = 0;
                                } else if (hue < 120) {
                                    r = ((120 - hue) * 255) / 60;
                                    g = 255;
                                    b = 0;
                                } else if (hue < 180) {
                                    r = 0;
                                    g = 255;
                                    b = ((hue - 120) * 255) / 60;
                                } else if (hue < 240) {
                                    r = 0;
                                    g = ((240 - hue) * 255) / 60;
                                    b = 255;
                                } else if (hue < 300) {
                                    r = ((hue - 240) * 255) / 60;
                                    g = 0;
                                    b = 255;
                                } else {
                                    r = 255;
                                    g = 0;
                                    b = ((360 - hue) * 255) / 60;
                                }

                                // Convert RGB888 to RGB565
                                pixel_color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                            } else {
                                pixel_color = scroll.color565;
                            }
                            chain_rgb_set_pixel(x, y, pixel_color);
                        } else {
                            chain_rgb_set_pixel(x, y, 0);
                        }
                    }
                }
            }

            // Update coordinates and loop
            scroll.offset_y += scroll.step_y;
            // Adjust total steps based on direction for consistent gradient
            if (scroll.dir == SCROLL_UP || scroll.dir == SCROLL_DOWN) {
                scroll.total_scroll_steps += scroll.step_y;
            }

            if (scroll.offset_y >= scroll.content_len) {
                scroll.offset_y -= scroll.content_len;
            }
            if (scroll.offset_y < 0) {
                scroll.offset_y += scroll.content_len;
            }
        }
    }
    // ================= BOUNCE / ONCE Mode (Linear scrolling) =================
    else {
        // Horizontal
        if (scroll.dir == SCROLL_LEFT || scroll.dir == SCROLL_RIGHT) {
            for (uint8_t y = 0; y < 8; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    int16_t vx = scroll.offset_x + x;
                    if (vx >= 0 && vx < VBUF_W) {
                        uint8_t byte_pos  = vx / 8;
                        uint8_t bit_pos   = vx % 8;
                        uint8_t bit_value = (virtual_buf[y][byte_pos] >> bit_pos) & 0x01;
                        if (bit_value) {
                            uint16_t pixel_color;
                            if (scroll.color565 == 0) {
                                // Rainbow gradient effect with continuous scrolling
                                uint16_t hue = (x + y + scroll.total_scroll_steps) * RAINBOW_HUE_MULTIPLIER +
                                               scroll.color_offset;  // Use total steps for continuous gradient
                                hue %= 360;                          // Ensure hue is always in 0-360 range
                                uint8_t r, g, b;

                                // Simple HSV to RGB conversion for rainbow effect
                                if (hue < 60) {
                                    r = 255;
                                    g = (hue * 255) / 60;
                                    b = 0;
                                } else if (hue < 120) {
                                    r = ((120 - hue) * 255) / 60;
                                    g = 255;
                                    b = 0;
                                } else if (hue < 180) {
                                    r = 0;
                                    g = 255;
                                    b = ((hue - 120) * 255) / 60;
                                } else if (hue < 240) {
                                    r = 0;
                                    g = ((240 - hue) * 255) / 60;
                                    b = 255;
                                } else if (hue < 300) {
                                    r = ((hue - 240) * 255) / 60;
                                    g = 0;
                                    b = 255;
                                } else {
                                    r = 255;
                                    g = 0;
                                    b = ((360 - hue) * 255) / 60;
                                }

                                // Convert RGB888 to RGB565
                                pixel_color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                            } else {
                                pixel_color = scroll.color565;
                            }
                            chain_rgb_set_pixel(x, y, pixel_color);
                        } else {
                            chain_rgb_set_pixel(x, y, 0);
                        }
                    }
                }
            }
            scroll.offset_x += scroll.step_x;
            // Adjust total steps based on direction for consistent gradient
            scroll.total_scroll_steps += scroll.step_x;

            // Boundary check (Bounce / Once)
            if (scroll.offset_x < 0 || scroll.offset_x > scroll.max_x) {
                if (scroll.mode == SCROLL_MODE_BOUNCE) {
                    scroll.step_x = -scroll.step_x;
                    // Bounce occurred, shift color offset
                    scroll.color_offset += 10;  // Shift hue by 10 degrees per bounce
                    scroll.color_offset %= 360;
                } else {
                    scroll.step_x = 0;  // Stop
                }
            }
        }
        // Vertical
        else {
            for (uint8_t y = 0; y < 8; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    int16_t vy = scroll.offset_y + y;
                    if (vy >= 0 && vy < VBUF_W) {
                        uint8_t byte_pos  = vy / 8;
                        uint8_t bit_pos   = vy % 8;
                        uint8_t bit_value = (virtual_buf[x][byte_pos] >> bit_pos) & 0x01;
                        if (bit_value) {
                            uint16_t pixel_color;
                            if (scroll.color565 == 0) {
                                // Rainbow gradient effect with continuous scrolling
                                uint16_t hue = (x + y + scroll.total_scroll_steps) * RAINBOW_HUE_MULTIPLIER +
                                               scroll.color_offset;  // Use total steps for continuous gradient
                                hue %= 360;                          // Ensure hue is always in 0-360 range
                                uint8_t r, g, b;

                                // Simple HSV to RGB conversion for rainbow effect
                                if (hue < 60) {
                                    r = 255;
                                    g = (hue * 255) / 60;
                                    b = 0;
                                } else if (hue < 120) {
                                    r = ((120 - hue) * 255) / 60;
                                    g = 255;
                                    b = 0;
                                } else if (hue < 180) {
                                    r = 0;
                                    g = 255;
                                    b = ((hue - 120) * 255) / 60;
                                } else if (hue < 240) {
                                    r = 0;
                                    g = ((240 - hue) * 255) / 60;
                                    b = 255;
                                } else if (hue < 300) {
                                    r = ((hue - 240) * 255) / 60;
                                    g = 0;
                                    b = 255;
                                } else {
                                    r = 255;
                                    g = 0;
                                    b = ((360 - hue) * 255) / 60;
                                }

                                // Convert RGB888 to RGB565
                                pixel_color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                            } else {
                                pixel_color = scroll.color565;
                            }
                            chain_rgb_set_pixel(x, y, pixel_color);
                        } else {
                            chain_rgb_set_pixel(x, y, 0);
                        }
                    }
                }
            }
            scroll.offset_y += scroll.step_y;
            // Adjust total steps based on direction for consistent gradient
            scroll.total_scroll_steps += scroll.step_y;

            // Boundary check (Bounce / Once)
            if (scroll.offset_y < 0 || scroll.offset_y > scroll.max_y) {
                if (scroll.mode == SCROLL_MODE_BOUNCE) {
                    scroll.step_y = -scroll.step_y;
                    // Bounce occurred, shift color offset
                    scroll.color_offset += 30;  // Shift hue by 30 degrees per bounce
                    scroll.color_offset %= 360;
                } else {
                    scroll.step_y = 0;  // Stop
                }
            }
        }
    }
}

/**
 * @brief Initialize and start scroll text display
 * @param str String to display
 * @param str_length Length of the string
 * @param font Font pointer
 * @param dir Scroll direction
 * @param mode Scroll mode (ONCE, LOOP, BOUNCE)
 * @param speed_ms Scroll speed in milliseconds
 * @param color565 Color in 565 format
 * @retval None
 */
void chain_rgb_scroll_text_start(char *str, uint8_t str_length, const font_t *font, scroll_dir_t dir,
                                 scroll_mode_t mode, uint16_t speed_ms, uint16_t color565)
{
    memset(&scroll, 0, sizeof(scroll));
    memset(virtual_buf, 0, sizeof(virtual_buf));

    if (str_length <= MAX_TEXT_LEN) {
        memcpy(scroll.current_str, str, str_length);
    } else {
        memcpy(scroll.current_str, str, MAX_TEXT_LEN);
        str_length = MAX_TEXT_LEN;
    }

    scroll.dir      = dir;
    scroll.mode     = mode;
    scroll.speed_ms = speed_ms;
    scroll.color565 = color565;
    scroll.state    = SCROLL_STATE_RUNNING;  // Start scrolling by default

    uint8_t str_offset = 0;

    int16_t cursor = 0;

    if (dir == SCROLL_LEFT || dir == SCROLL_RIGHT) {
        /* Shift the font up by one row, leave row 0 empty */
        uint8_t base_row = 0;  // Original baseline is 0, now start drawing from row 1
        while (*str && str_offset < str_length && cursor < VBUF_W) {
            str_offset++;
            uint8_t c = *str++;
            if (c < 32 || c > 127) c = ' ';
            uint16_t index       = (c - 32) * font->width;
            const uint8_t *glyph = &font->data[index];
            uint8_t actual_width = chain_rgb_get_char_actual_width(c, font);

            for (uint8_t col = 0; col < actual_width; col++) {
                if (cursor + col >= VBUF_W) break;
                uint8_t bits = glyph[col];
                /* Start drawing font from row 1, leave row 0 blank */
                for (uint8_t row = 0; row < font->height; row++) {
                    uint8_t dst_row = base_row + 1 + row;
                    if (dst_row < 8 && (bits & (1 << row))) {
                        uint16_t buf_idx = cursor + col;
                        uint8_t byte_pos = buf_idx / 8;
                        uint8_t bit_pos  = buf_idx % 8;
                        virtual_buf[dst_row][byte_pos] |= (1 << bit_pos);
                    }
                }
            }
            cursor += actual_width + font->spacing;
        }
    } else  // UP or DOWN
    {
        /* For vertical scrolling, also shift the font up by one row */
        uint8_t base_row        = 1;  // Start drawing from row 1, leave row 0 blank
        uint8_t center_offset_x = (font->width < 8) ? (8 - font->width) / 2 : 0;
        while (*str && str_offset < str_length && cursor < VBUF_W) {
            str_offset++;
            uint8_t c = *str++;
            if (c < 32 || c > 127) c = ' ';
            uint16_t index       = (c - 32) * font->width;
            const uint8_t *glyph = &font->data[index];

            for (uint8_t col = 0; col < font->width; col++) {
                uint8_t bits     = glyph[col];
                uint8_t screen_x = center_offset_x + col;
                if (screen_x >= 8) continue;
                for (uint8_t row = 0; row < font->height; row++) {
                    if (bits & (1 << row)) {
                        uint16_t target_y = cursor + base_row + row;
                        if (target_y < VBUF_W) {
                            uint8_t byte_pos = target_y / 8;
                            uint8_t bit_pos  = target_y % 8;
                            virtual_buf[screen_x][byte_pos] |= (1 << bit_pos);
                        }
                    }
                }
            }
            cursor += font->height + font->spacing;
        }
    }

    if (mode == SCROLL_MODE_LOOP) {
        cursor += (dir == SCROLL_LEFT || dir == SCROLL_RIGHT) ? 1 : font->height;
    }

    scroll.content_len = cursor;  // Record total length for loop modulus

    // Calculate boundaries for BOUNCE/ONCE modes (linear movement endpoint)
    scroll.max_x = (cursor > 8) ? (cursor - 8) : 0;
    scroll.max_y = (cursor > 8) ? (cursor - 8) : 0;

    // Initialize offset and step
    chain_rgb_scroll_text_reset_coords();
}

void chain_rgb_update(void)
{
    if (!g_send_complete_flag) {
        return;
    }

    if (s_rgb_update_flag) {
        // Set the first RGB LED color to black (off)
        for (uint8_t i = 0; i < RGB_NUM; i++) {
            uint8_t x      = i % 8;
            uint8_t y      = i / 8;
            uint16_t color = display_buffer[y][x];
            chain_rgb_setcolor(i, color);
        }

        rgb_send_array();
        s_rgb_update_flag    = 0;
        g_send_complete_flag = 0;
    }
}

/**
 * @brief Command handler for setting display mode
 * @param buffer Command buffer containing mode information
 * @param size Size of the command buffer
 * @retval None
 */
void chain_rgb_set_mode_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_FAIL;

    if (buffer != NULL && size == 1) {
        if (buffer[0] <= RGB_STRING_SCROLL_MODE) {
            chain_rgb_set_mode((rgb_mode_t)buffer[0]);
            s_return_status = OPERATION_SUCCESS;
        }
    }
    chain_command_complete_return(CHAIN_RGB_SET_MODE, &s_return_status, 1);
}

/**
 * @brief Command handler for getting current display mode
 * @param None
 * @retval None
 */
void chain_rgb_get_mode_handle(void)
{
    chain_command_complete_return(CHAIN_RGB_GET_MODE, &rgb_mode, 1);
}

/**
 * @brief Command handler for setting pixel states
 * @param buffer Command buffer containing pixel data
 * @param size Size of the command buffer
 * @retval None
 */
void chain_rgb_set_pixel_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_FAIL;

    if (rgb_mode == RGB_PIXEL_MODE) {
        if (buffer != NULL && size >= 4) {
            uint8_t num = buffer[0];
            if (num != 0) {
                for (uint8_t i = 0; i < num; i++) {
                    uint8_t byte      = buffer[1 + i * 3];
                    uint8_t x         = (byte >> 3) & 0b00000111;
                    uint8_t y         = byte & 0b00000111;
                    uint16_t color565 = (buffer[3 + i * 3] << 8) | buffer[2 + i * 3];
                    chain_rgb_set_pixel(x, y, color565);
                }
            }
            s_return_status = OPERATION_SUCCESS;
        }
    } else {
        s_return_status = OPERATION_MODE_MISMATCH;
    }
    // Return the result of the set operation
    chain_command_complete_return(CHAIN_RGB_SET_PIXEL, &s_return_status, 1);
}

/**
 * @brief Command handler for getting pixel states
 * @param buffer Command buffer containing pixel addresses
 * @param size Size of the command buffer
 * @retval None
 */
void chain_rgb_get_pixel_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t pixel_buffer[128];
    s_ret_buf_size = 0;

    if (buffer != NULL && size >= 2) {
        uint8_t num = buffer[0];
        if (num != 0) {
            for (uint8_t i = 0; i < num; i++) {
                uint8_t byte                   = buffer[1 + i];
                uint8_t x                      = (byte >> 3) & 0b00000111;
                uint8_t y                      = byte & 0b00000111;
                uint16_t color565              = chain_rgb_get_pixel(x, y);
                pixel_buffer[s_ret_buf_size++] = color565 & 0xFF;
                pixel_buffer[s_ret_buf_size++] = (color565 >> 8) & 0xFF;
            }
            // Return the result of the get operation
            chain_command_complete_return(CHAIN_RGB_GET_PIXEL, pixel_buffer, s_ret_buf_size);
        }
    }
}

/**
 * @brief Command handler for setting entire display buffer
 * @param buffer Command buffer containing 8 bytes of display data
 * @param size Size of the command buffer
 * @retval None
 */
void chain_rgb_set_display_buffer_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_FAIL;

    if (rgb_mode == RGB_PIXEL_MODE) {
        if (buffer != NULL && size == 128) {
            // Clear buffer first
            chain_rgb_clear();
            // Copy pixel data to display buffer byte by byte
            for (uint8_t y = 0; y < 8; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    uint16_t index = y * 16 + x * 2;
                    chain_rgb_set_pixel(x, y, (buffer[index + 1] << 8 | buffer[index]));
                }
            }
            s_return_status = OPERATION_SUCCESS;
        }
    } else {
        s_return_status = OPERATION_MODE_MISMATCH;
    }
    // Return the result of the set operation
    chain_command_complete_return(CHAIN_RGB_SET_DISPLAY_BUFFER, &s_return_status, 1);
}

/**
 * @brief Command handler for getting entire display buffer
 * @param None
 * @retval None
 */
void chain_rgb_get_display_buffer_handle(void)
{
    uint8_t buffer[128] = {0};

    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            uint16_t color565 = chain_rgb_get_pixel(x, y);
            uint16_t index    = y * 16 + x * 2;
            buffer[index]     = color565 & 0xFF;
            buffer[index + 1] = (color565 >> 8) & 0xFF;
        }
    }
    // Return the display buffer data
    chain_command_complete_return(CHAIN_RGB_GET_DISPLAY_BUFFER, buffer, 128);
}

/**
 * @brief Command handler for setting screen rotation
 * @param buffer Command buffer containing rotation angle and save flag
 * @param size Size of the command buffer
 * @retval None
 */
void chain_rgb_set_rotation_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_SUCCESS;

    if (buffer != NULL && buffer[0] >= 0 && buffer[0] <= 3 && size == 2) {
        chain_rgb_set_rotation(buffer[0]);
        uint8_t flag = buffer[1];                                 // Get the flag indicating whether to save the value
        if (flag && get_screen_rotation() != g_current_rotation)  // If the flag is set, save the new rotation value
        {
            set_screen_rotation(g_current_rotation);  // Save the new rotation value
        }

    } else {
        s_return_status = OPERATION_FAIL;
    }
    // Return the result of the set operation
    chain_command_complete_return(CHAIN_RGB_SET_ROTATION, &s_return_status, 1);
}

/**
 * @brief Command handler for getting current screen rotation
 * @param None
 * @retval None
 */
void chain_rgb_get_rotation_handle(void)
{
    uint8_t rotation = (uint8_t)chain_rgb_get_rotation();

    // Return the current rotation value
    chain_command_complete_return(CHAIN_RGB_GET_ROTATION, &rotation, 1);
}

/**
 * @brief Command handler for setting display brightness
 * @param buffer Command buffer containing brightness level and save flag
 * @param size Size of the command buffer
 * @retval None
 */
void chain_rgb_set_brightness_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_SUCCESS;

    if (buffer != NULL && size == 2 && buffer[0] >= 0 && buffer[0] <= 100) {
        chain_rgb_set_brightness(buffer[0]);
        uint8_t flag = buffer[1];                       // Get the flag indicating whether to save the value
        if (flag && get_brightness_level() != g_light)  // If the flag is set, save the new brightness value
        {
            set_brightness_level(g_light);  // Save the new brightness value
        }
    } else {
        s_return_status = OPERATION_FAIL;
    }
    // Return the result of the set operation
    chain_command_complete_return(CHAIN_RGB_SET_BRIGHTNESS, &s_return_status, 1);
}

/**
 * @brief Command handler for getting current brightness value
 * @param None
 * @retval None
 */
void chain_rgb_get_brightness_handle(void)
{
    // Return the current brightness value
    chain_command_complete_return(CHAIN_RGB_GET_BRIGHTNESS, &g_light, 1);
}

/**
 * @brief Command handler for setting scroll text configuration
 * @param buffer Command buffer containing scroll text parameters
 * @param size Size of the command buffer
 * @retval None
 */
void chain_rgb_set_scroll_text_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_FAIL;

    if (rgb_mode == RGB_STRING_SCROLL_MODE) {
        if (buffer != NULL && size >= 6) {
            scroll_dir_t dir      = (buffer[0] >> 4) & 0b00001111;
            scroll_mode_t mode    = buffer[0] & 0b00001111;
            uint16_t speed_ms     = (buffer[2] << 8) | buffer[1];
            uint16_t color565     = (buffer[4] << 8) | buffer[3];
            uint8_t string_length = buffer[5];
            chain_rgb_scroll_text_start(buffer + 6, string_length, &Font5x7, dir, mode, speed_ms, color565);

            s_return_status = OPERATION_SUCCESS;
        }
    } else {
        s_return_status = OPERATION_MODE_MISMATCH;
    }

    // Return the result of the set operation
    chain_command_complete_return(CHAIN_RGB_SET_SCROLL_TEXT, &s_return_status, 1);
}

/**
 * @brief Command handler for getting scroll text configuration
 * @param None
 * @retval None
 */
void chain_rgb_get_scroll_text_handle(void)
{
    uint8_t scroll_text_buffer[MAX_TEXT_LEN + 6];
    scroll_text_buffer[0] = (uint8_t)(scroll.dir << 4) | scroll.mode;
    scroll_text_buffer[1] = scroll.speed_ms & 0xFF;
    scroll_text_buffer[2] = (scroll.speed_ms >> 8) & 0xFF;
    scroll_text_buffer[3] = scroll.color565 & 0xFF;
    scroll_text_buffer[4] = (scroll.color565 >> 8) & 0xFF;
    scroll_text_buffer[5] = strlen(scroll.current_str) & 0xFF;

    memcpy(scroll_text_buffer + 6, scroll.current_str, scroll_text_buffer[5]);
    // Return the scroll text configuration
    chain_command_complete_return(CHAIN_RGB_GET_SCROLL_TEXT, scroll_text_buffer, scroll_text_buffer[5] + 6);
}

/**
 * @brief Command handler for controlling scroll text state
 * @param scroll_state Scroll state command to set
 * @retval None
 */
void chain_rgb_set_scroll_text_state_handle(uint8_t scroll_state)
{
    s_return_status = OPERATION_FAIL;

    if (rgb_mode == RGB_STRING_SCROLL_MODE) {
        if (scroll_state >= 0 && scroll_state <= 3) {
            chain_rgb_scroll_text_control(scroll_state);
            s_return_status = OPERATION_SUCCESS;
        }
    } else {
        s_return_status = OPERATION_MODE_MISMATCH;
    }
    // Return the result of the set operation
    chain_command_complete_return(CHAIN_RGB_SET_SCROLL_TEXT_STATE, &s_return_status, 1);
}

/**
 * @brief Command handler for getting current scroll text state
 * @param None
 * @retval None
 */
void chain_rgb_get_scroll_text_state_handle(void)
{
    chain_command_complete_return(CHAIN_RGB_GET_SCROLL_TEXT_STATE, &scroll.state, 1);
}

/**
 * @brief Command handler for displaying a single character
 * @param buffer Command buffer containing character and position
 * @param size Size of the command buffer
 * @retval None
 */
void chain_rgb_set_char_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_FAIL;

    if (rgb_mode == RGB_PIXEL_MODE) {
        if (buffer != NULL && size == 4) {
            uint8_t x         = (buffer[1] >> 4) & 0b00001111;
            uint8_t y         = buffer[1] & 0b00001111;
            uint16_t color565 = (buffer[3] << 8) | buffer[2];
            chain_rgb_show_char((char)buffer[0], x, y, &Font5x7, color565, 1);
            s_return_status = OPERATION_SUCCESS;
        }

    } else {
        s_return_status = OPERATION_MODE_MISMATCH;
    }

    // Return the result of the set operation
    chain_command_complete_return(CHAIN_RGB_SET_CHAR, &s_return_status, 1);
}

/**
 * @brief Command handler for clearing the display
 * @param None
 * @retval None
 */
void chain_rgb_clear_handle(void)
{
    s_return_status = OPERATION_SUCCESS;
    if (rgb_mode == RGB_STRING_SCROLL_MODE) {
        chain_rgb_scroll_text_control(SCROLL_CMD_STOP_CLEAR);
    }
    chain_rgb_clear();

    // Return the result of the clear operation
    chain_command_complete_return(CHAIN_RGB_CLEAR, &s_return_status, 1);
}
