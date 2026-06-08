/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include <mono_function.h>
#include <font5x7.h>

const uint16_t led_refresh_map[9] = {LED_PA0_Pin, LED_PA1_Pin, LED_PA4_Pin,  LED_PA5_Pin, LED_PA6_Pin,
                                     LED_PA7_Pin, LED_PA8_Pin, LED_PA11_Pin, LED_PA12_Pin};

// 硬件显示双缓冲区
static uint8_t led_refresh_date[9][8] = {0};  // 主循环写这个
uint8_t buf_front[9][8]               = {0};  // 中断服务程序读这个

typedef struct {
    uint8_t hw_row;  // 目标硬件行
    uint8_t hw_col;  // 目标硬件列
} reverse_mapping;

// display_buffer[8][8] → led_refresh_date[9][8]
const reverse_mapping reverse_table[8][8] = {
    // display第0行  col: 0       1       2       3       4       5       6       7
    /* row0 */ {{1, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 4}, {6, 5}, {7, 6}, {8, 7}},

    // display第1行
    /* row1 */ {{0, 7}, {2, 0}, {3, 1}, {4, 2}, {5, 3}, {6, 4}, {7, 5}, {8, 6}},

    // display第2行
    /* row2 */ {{0, 6}, {1, 7}, {3, 0}, {4, 1}, {5, 2}, {6, 3}, {7, 4}, {8, 5}},

    // display第3行
    /* row3 */ {{0, 5}, {1, 6}, {2, 6}, {4, 0}, {5, 1}, {6, 2}, {7, 3}, {8, 4}},

    // display第4行
    /* row4 */ {{0, 4}, {1, 5}, {2, 7}, {3, 7}, {5, 0}, {6, 1}, {7, 2}, {8, 3}},

    // display第5行
    /* row5 */ {{0, 3}, {1, 4}, {2, 5}, {3, 6}, {4, 7}, {6, 0}, {7, 1}, {8, 2}},

    // display第6行
    /* row6 */ {{0, 2}, {1, 3}, {2, 4}, {3, 5}, {4, 6}, {5, 7}, {7, 0}, {8, 1}},

    // display第7行
    /* row7 */ {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {8, 0}},
};

/** @brief Bitmask of all LED pins */
const uint32_t all_led_pins = LED_PA0_Pin | LED_PA1_Pin | LED_PA4_Pin | LED_PA5_Pin | LED_PA6_Pin | LED_PA7_Pin |
                              LED_PA8_Pin | LED_PA11_Pin | LED_PA12_Pin;

/**
 * @brief Gamma correction table mapping 8 brightness levels to 16 PWM levels (gamma 2.2)
 */
const uint8_t gamma_table[8] = {
    0,   // Level 0: 0%      (Off)
    2,   // Level 1: 12.5%
    4,   // Level 2: 25%
    6,   // Level 3: 37.5%
    8,   // Level 4: 50%
    10,  // Level 5: 62.5%
    13,  // Level 6: 81.25%
    16   // Level 7: 100%
};

// 补偿查表（×256定点），索引 = 点亮数量 0~8
// 值 = I(8)/I(n) × 256，基于实测数据
const uint16_t comp_table[9] = {
    //   0     1     2     3     4     5     6     7     8
    0, 100, 118, 135, 151, 171, 201, 229, 256};

uint8_t s_col = 0;

/** @brief Return status for command operations */
static uint8_t s_return_status = 0;

/** @brief Display buffer (8 rows × 8 columns) */
static uint8_t display_buffer[8][8] = {0};

/** @brief Current screen rotation angle */
chain_mono_rotation_t g_current_rotation = ROTATION_0;

/** @brief Global brightness level (0-7), default 7 */
uint8_t g_brightness_level = 7;
/** @brief Cached gamma-corrected brightness value */
static uint8_t g_corrected_brightness = 15;

/** @brief Virtual display buffer for scrolling text */
static uint8_t virtual_buf[8][VBUF_W];

/** @brief 5x7 font definition */
const font_t Font5x7 = {.width = FONT5X7_WIDTH, .height = FONT5X7_HEIGHT, .spacing = 1, .data = font5x7};

/** @brief Scroll context structure */
static scroll_ctx_t scroll;

/** @brief Return buffer size for command responses */
static uint8_t s_ret_buf_size = 0;

/** @brief Current mono display mode */
static mono_mode_t mono_mode = MONO_PIXEL_MODE;

void chain_mono_swap_buffer(void)
{
    __disable_irq();
    memcpy(buf_front, led_refresh_date, sizeof(buf_front));
    __enable_irq();
}

/**
 * @brief Set the display operating mode
 * @param mode Display mode to set
 * @retval None
 */
void chain_mono_set_mode(mono_mode_t mode)
{
    if (mono_mode == MONO_STRING_SCROLL_MODE && mode == MONO_PIXEL_MODE) {
        chain_mono_scroll_text_control(SCROLL_CMD_STOP_CLEAR);
    }
    mono_mode = mode;
}

/**
 * @brief Clear the display buffer
 * @param None
 * @retval None
 */
void chain_mono_clear(void)
{
    memset(display_buffer, 0, sizeof(display_buffer));
    memset(led_refresh_date, 0, sizeof(led_refresh_date));
}

/**
 * @brief Set screen rotation angle
 * @param rotation Rotation angle (ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270)
 * @retval None
 */
void chain_mono_set_rotation(chain_mono_rotation_t rotation)
{
    if (rotation <= ROTATION_270) {
        g_current_rotation = (rotation + 4 - g_current_rotation) % 4;
        uint8_t temp[8][8];
        memcpy(temp, display_buffer, sizeof(temp));
        chain_mono_display_pattern(temp);
        g_current_rotation = rotation;
    }
}

/**
 * @brief Get current rotation angle
 * @param None
 * @retval Current rotation angle
 */
chain_mono_rotation_t chain_mono_get_rotation(void)
{
    return g_current_rotation;
}

/**
 * @brief Rotate coordinates based on current rotation setting
 * @param x Input X coordinate
 * @param y Input Y coordinate
 * @param out_x Output X coordinate pointer
 * @param out_y Output Y coordinate pointer
 * @retval None
 */
static void chain_mono_rotate_coordinate(uint8_t x, uint8_t y, uint8_t *out_x, uint8_t *out_y)
{
    switch (g_current_rotation) {
        case ROTATION_0:  // 0 degrees (default)
            *out_x = x;
            *out_y = y;
            break;

        case ROTATION_90:  // 90 degrees clockwise
            *out_x = 7 - y;
            *out_y = x;
            break;

        case ROTATION_180:  // 180 degrees
            *out_x = 7 - x;
            *out_y = 7 - y;
            break;

        case ROTATION_270:  // 270 degrees clockwise
            *out_x = y;
            *out_y = 7 - x;
            break;

        default:
            *out_x = x;
            *out_y = y;
            break;
    }
}

/**
 * @brief Set LED state at specific position with rotation applied
 * @param x Column coordinate (0-7)
 * @param y Row coordinate (0-7)
 * @param brightness LED brightness (0-7)
 * @retval None
 */
void chain_mono_set_pixel(uint8_t x, uint8_t y, uint8_t brightness)
{
    uint8_t real_x, real_y;

    if (x < 8 && y < 8) {
        // 坐标旋转转换
        chain_mono_rotate_coordinate(x, y, &real_x, &real_y);
        uint8_t hw_row                   = reverse_table[real_y][real_x].hw_row;
        uint8_t hw_col                   = reverse_table[real_y][real_x].hw_col;
        led_refresh_date[hw_row][hw_col] = brightness;
        display_buffer[real_y][real_x]   = brightness;
    }
}

/**
 * @brief Get LED state at specific position with rotation applied
 * @param x Column coordinate (0-7)
 * @param y Row coordinate (0-7)
 * @retval LED brightness (0-7)
 */
uint8_t chain_mono_get_pixel(uint8_t x, uint8_t y)
{
    uint8_t real_x, real_y;

    if (x < 8 && y < 8) {
        chain_mono_rotate_coordinate(x, y, &real_x, &real_y);
        return display_buffer[real_y][real_x];
    }
    return 0;
}

/**
 * @brief Turn off all LEDs
 * @param None
 * @retval None
 */
inline void chain_mono_all_off(void)
{
    GPIOA->MODER &= ~LED_ALL_MODER_MASK;
    GPIOA->BSRR = all_led_pins << 16;  // 全部拉低
}

/**
 * @brief Display 8x8 grayscale pattern
 * @param pattern 8x8 grayscale array, each element 0~7
 * @retval None
 */
void chain_mono_display_pattern(const uint8_t pattern[8][8])
{
    // Clear buffer
    chain_mono_clear();

    // Copy pixel by pixel into display buffer
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            chain_mono_set_pixel(x, y, pattern[y][x]);
        }
    }
    chain_mono_swap_buffer();
}

/**
 * @brief Get actual character width by removing trailing empty columns
 * @param ch Character to measure
 * @param font Font structure pointer
 * @retval Actual character width in columns
 */
static uint8_t chain_mono_get_char_actual_width(char ch, const font_t *font)
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
 * @param brightness Brightness (0-7)
 * @param clear_bg Clear background before displaying (0: no, 1: yes)
 * @retval None
 */
void chain_mono_show_char(char ch, uint8_t x_offset, uint8_t y_offset, const font_t *font, uint8_t brightness,
                          uint8_t clear_bg)
{
    // Character range check
    if (ch < 32 || ch > 127) {
        ch = ' ';  // Display unsupported characters as space
    }

    // Clear screen
    if (clear_bg) {
        chain_mono_clear();
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
                chain_mono_set_pixel(screen_x, screen_y, brightness);
            } else {
                chain_mono_set_pixel(screen_x, screen_y, 0);
            }
        }
    }
    chain_mono_swap_buffer();
}

/**
 * @brief Set display brightness level with gamma correction
 * @param brightness Brightness level (0-7)
 * @retval None
 */
void chain_mono_set_brightness(uint8_t brightness)
{
    if (brightness > 7) brightness = 7;
    g_brightness_level     = brightness;
    g_corrected_brightness = gamma_table[g_brightness_level];  // Pre-calculate gamma correction
}

/**
 * @brief Reset scroll coordinates based on direction and mode
 * @retval None
 */
static void chain_mono_scroll_text_reset_coords(void)
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
void chain_mono_scroll_text_control(scroll_cmd_t cmd)
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
            chain_mono_clear();
            chain_mono_swap_buffer();
            // Reset coordinates to initial position
            chain_mono_scroll_text_reset_coords();
            break;
    }
}

/**
 * @brief Update scroll text display based on current state and mode
 * @param tick_ms Current system tick time in milliseconds
 * @retval None
 */
void chain_mono_scroll_text_update(uint32_t tick_ms)
{
    if (scroll.state != SCROLL_STATE_RUNNING || mono_mode != MONO_STRING_SCROLL_MODE) return;

    if (tick_ms - scroll.last_tick < scroll.speed_ms) return;

    scroll.last_tick = tick_ms;
    chain_mono_clear();

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
                    if (target_idx < VBUF_W) chain_mono_set_pixel(x, y, virtual_buf[y][target_idx]);
                }
            }

            // Update coordinates and loop
            scroll.offset_x += scroll.step_x;

            // Normalize offset to prevent overflow (keep between 0 ~ content_len)
            if (scroll.offset_x >= scroll.content_len) scroll.offset_x -= scroll.content_len;
            if (scroll.offset_x < 0) scroll.offset_x += scroll.content_len;
        }
        // 2. Vertical scrolling (Up / Down)
        else {
            for (uint8_t y = 0; y < 8; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    int32_t target_idx = scroll.offset_y + y;

                    while (target_idx < 0) target_idx += scroll.content_len;
                    while (target_idx >= scroll.content_len) target_idx -= scroll.content_len;

                    // Vertical mode: virtual_buf[x][target_idx]
                    if (target_idx < VBUF_W) chain_mono_set_pixel(x, y, virtual_buf[x][target_idx]);
                }
            }

            // Update coordinates and loop
            scroll.offset_y += scroll.step_y;

            if (scroll.offset_y >= scroll.content_len) scroll.offset_y -= scroll.content_len;
            if (scroll.offset_y < 0) scroll.offset_y += scroll.content_len;
        }
    }
    // ================= BOUNCE / ONCE Mode (Linear scrolling) =================
    else {
        // Horizontal
        if (scroll.dir == SCROLL_LEFT || scroll.dir == SCROLL_RIGHT) {
            for (uint8_t y = 0; y < 8; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    int16_t vx = scroll.offset_x + x;
                    if (vx >= 0 && vx < VBUF_W) chain_mono_set_pixel(x, y, virtual_buf[y][vx]);
                }
            }
            scroll.offset_x += scroll.step_x;

            // Boundary check (Bounce / Once)
            if (scroll.offset_x < 0 || scroll.offset_x > scroll.max_x) {
                if (scroll.mode == SCROLL_MODE_BOUNCE)
                    scroll.step_x = -scroll.step_x;
                else
                    scroll.step_x = 0;  // Stop
            }
        }
        // Vertical
        else {
            for (uint8_t y = 0; y < 8; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    int16_t vy = scroll.offset_y + y;
                    if (vy >= 0 && vy < VBUF_W) chain_mono_set_pixel(x, y, virtual_buf[x][vy]);
                }
            }

            // Update coordinates and loop
            scroll.offset_y += scroll.step_y;

            if (scroll.offset_y < 0 || scroll.offset_y > scroll.max_y) {
                if (scroll.mode == SCROLL_MODE_BOUNCE)
                    scroll.step_y = -scroll.step_y;
                else
                    scroll.step_y = 0;
            }
        }
    }
    chain_mono_swap_buffer();
}

/**
 * @brief Initialize and start scroll text display
 * @param str String to display
 * @param str_length Length of the string
 * @param font Font pointer
 * @param dir Scroll direction
 * @param mode Scroll mode (ONCE, LOOP, BOUNCE)
 * @param speed_ms Scroll speed in milliseconds
 * @param brightness Brightness level (0-7)
 * @retval None
 */
void chain_mono_scroll_text_start(char *str, uint8_t str_length, const font_t *font, scroll_dir_t dir,
                                  scroll_mode_t mode, uint16_t speed_ms, uint8_t brightness)
{
    memset(&scroll, 0, sizeof(scroll));
    memset(virtual_buf, 0, sizeof(virtual_buf));

    if (str_length <= MAX_TEXT_LEN) {
        memcpy(scroll.current_str, str, str_length);
    } else {
        memcpy(scroll.current_str, str, MAX_TEXT_LEN);
    }

    scroll.dir      = dir;
    scroll.mode     = mode;
    scroll.speed_ms = speed_ms;
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
            uint8_t actual_width = chain_mono_get_char_actual_width(c, font);

            for (uint8_t col = 0; col < actual_width; col++) {
                if (cursor + col >= VBUF_W) break;
                uint8_t bits = glyph[col];
                /* Start drawing font from row 1, leave row 0 blank */
                for (uint8_t row = 0; row < font->height; row++) {
                    uint8_t dst_row = base_row + 1 + row;
                    if (dst_row < 8 && (bits & (1 << row))) {
                        virtual_buf[dst_row][cursor + col] = brightness;
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
                        if (target_y < VBUF_W) virtual_buf[screen_x][target_y] = brightness;
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
    chain_mono_scroll_text_reset_coords();
}

/**
 * @brief Command handler for setting display mode
 * @param buffer Command buffer containing mode information
 * @param size Size of the command buffer
 * @retval None
 */
void chain_mono_set_mode_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_FAIL;

    if (buffer != NULL && size == 1) {
        if (buffer[0] <= MONO_STRING_SCROLL_MODE) {
            chain_mono_set_mode((mono_mode_t)buffer[0]);
            s_return_status = OPERATION_SUCCESS;
        }
    }
    chain_command_complete_return(CHAIN_MONO_SET_MODE, &s_return_status, 1);
}

/**
 * @brief Command handler for getting current display mode
 * @param None
 * @retval None
 */
void chain_mono_get_mode_handle(void)
{
    chain_command_complete_return(CHAIN_MONO_GET_MODE, &mono_mode, 1);
}

/**
 * @brief Command handler for setting pixel states
 * @param buffer Command buffer containing pixel data
 * @param size Size of the command buffer
 * @retval None
 */
void chain_mono_set_pixel_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_FAIL;

    if (mono_mode == MONO_PIXEL_MODE) {
        if (buffer != NULL && size >= 2) {
            uint8_t num = buffer[0];
            if (num != 0) {
                for (uint8_t i = 0; i < num; i++) {
                    uint8_t byte  = buffer[1 + i];
                    uint8_t x     = (byte >> 3) & 0b00000111;
                    uint8_t y     = byte & 0b00000111;
                    uint8_t value = (byte >> 6) & 0b000001;
                    chain_mono_set_pixel(x, y, value ? 7 : 0);
                }
                chain_mono_swap_buffer();
            }
            s_return_status = OPERATION_SUCCESS;
        }
    } else {
        s_return_status = OPERATION_MODE_MISMATCH;
    }
    // Return the result of the set operation
    chain_command_complete_return(CHAIN_MONO_SET_PIXEL, &s_return_status, 1);
}

/**
 * @brief Command handler for getting pixel states
 * @param buffer Command buffer containing pixel addresses
 * @param size Size of the command buffer
 * @retval None
 */
void chain_mono_get_pixel_handle(uint8_t *buffer, uint8_t size)
{
    uint8_t pixel_buffer[64];
    s_ret_buf_size = 0;

    if (buffer != NULL && size >= 2) {
        uint8_t num = buffer[0];
        if (num != 0) {
            for (uint8_t i = 0; i < num; i++) {
                uint8_t byte                   = buffer[1 + i];
                uint8_t x                      = (byte >> 3) & 0b00000111;
                uint8_t y                      = byte & 0b00000111;
                uint8_t value                  = (chain_mono_get_pixel(x, y) > 0) ? 1 : 0;
                pixel_buffer[s_ret_buf_size++] = value;
            }
            // Return the result of the get operation
            chain_command_complete_return(CHAIN_MONO_GET_PIXEL, pixel_buffer, s_ret_buf_size);
        }
    }
}

/**
 * @brief Command handler for setting entire display buffer
 * @param buffer Command buffer containing 8 bytes of display data
 * @param size Size of the command buffer
 * @retval None
 */
void chain_mono_set_display_buffer_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_FAIL;

    if (mono_mode == MONO_PIXEL_MODE) {
        if (buffer != NULL && size == 8) {
            // Clear buffer first
            chain_mono_clear();
            // Copy pixel data to display buffer byte by byte
            for (uint8_t y = 0; y < 8; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    chain_mono_set_pixel(x, y, buffer[y] & (0b10000000 >> x) ? 7 : 0);
                }
            }
            chain_mono_swap_buffer();
            s_return_status = OPERATION_SUCCESS;
        }
    } else {
        s_return_status = OPERATION_MODE_MISMATCH;
    }
    // Return the result of the set operation
    chain_command_complete_return(CHAIN_MONO_SET_DISPLAY_BUFFER, &s_return_status, 1);
}

/**
 * @brief Command handler for getting entire display buffer
 * @param None
 * @retval None
 */
void chain_mono_get_display_buffer_handle(void)
{
    uint8_t buffer[8] = {0};

    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            buffer[y] |= (chain_mono_get_pixel(x, y) > 0) << (7 - x);
        }
    }
    // Return the display buffer data
    chain_command_complete_return(CHAIN_MONO_GET_DISPLAY_BUFFER, buffer, 8);
}

/**
 * @brief Command handler for setting screen rotation
 * @param buffer Command buffer containing rotation angle and save flag
 * @param size Size of the command buffer
 * @retval None
 */
void chain_mono_set_rotation_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_SUCCESS;

    if (buffer != NULL && buffer[0] >= 0 && buffer[0] <= 3 && size == 2) {
        chain_mono_set_rotation(buffer[0]);
        uint8_t flag = buffer[1];                                 // Get the flag indicating whether to save the value
        if (flag && get_screen_rotation() != g_current_rotation)  // If the flag is set, save the new rotation value
        {
            set_screen_rotation(g_current_rotation);  // Save the new rotation value
        }

    } else {
        s_return_status = OPERATION_FAIL;
    }
    // Return the result of the set operation
    chain_command_complete_return(CHAIN_MONO_SET_ROTATION, &s_return_status, 1);
}

/**
 * @brief Command handler for getting current screen rotation
 * @param None
 * @retval None
 */
void chain_mono_get_rotation_handle(void)
{
    uint8_t rotation = (uint8_t)chain_mono_get_rotation();

    // Return the current rotation value
    chain_command_complete_return(CHAIN_MONO_GET_ROTATION, &rotation, 1);
}

/**
 * @brief Command handler for setting display brightness
 * @param buffer Command buffer containing brightness level and save flag
 * @param size Size of the command buffer
 * @retval None
 */
void chain_mono_set_brightness_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_SUCCESS;

    if (buffer != NULL && size == 2 && buffer[0] >= 0 && buffer[0] <= 7) {
        chain_mono_set_brightness(buffer[0]);
        uint8_t flag = buffer[1];                                  // Get the flag indicating whether to save the value
        if (flag && get_brightness_level() != g_brightness_level)  // If the flag is set, save the new brightness value
        {
            set_brightness_level(g_brightness_level);  // Save the new brightness value
        }
    } else {
        s_return_status = OPERATION_FAIL;
    }
    // Return the result of the set operation
    chain_command_complete_return(CHAIN_MONO_SET_BRIGHTNESS, &s_return_status, 1);
}

/**
 * @brief Command handler for getting current brightness level
 * @param None
 * @retval None
 */
void chain_mono_get_brightness_handle(void)
{
    // Return the current brightness level
    chain_command_complete_return(CHAIN_MONO_GET_BRIGHTNESS, &g_brightness_level, 1);
}

/**
 * @brief Command handler for setting scroll text configuration
 * @param buffer Command buffer containing scroll text parameters
 * @param size Size of the command buffer
 * @retval None
 */
void chain_mono_set_scroll_text_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_FAIL;

    if (mono_mode == MONO_STRING_SCROLL_MODE) {
        if (buffer != NULL && size >= 4) {
            scroll_dir_t dir      = (buffer[0] >> 4) & 0b00001111;
            scroll_mode_t mode    = buffer[0] & 0b00001111;
            uint16_t speed_ms     = (buffer[2] << 8) | buffer[1];
            uint8_t string_length = buffer[3];
            chain_mono_scroll_text_start(buffer + 4, string_length, &Font5x7, dir, mode, speed_ms, 7);

            s_return_status = OPERATION_SUCCESS;
        }
    } else {
        s_return_status = OPERATION_MODE_MISMATCH;
    }

    // Return the result of the set operation
    chain_command_complete_return(CHAIN_MONO_SET_SCROLL_TEXT, &s_return_status, 1);
}

/**
 * @brief Command handler for getting scroll text configuration
 * @param None
 * @retval None
 */
void chain_mono_get_scroll_text_handle(void)
{
    uint8_t scroll_text_buffer[MAX_TEXT_LEN + 4];
    scroll_text_buffer[0] = (uint8_t)(scroll.dir << 4) | scroll.mode;
    scroll_text_buffer[1] = scroll.speed_ms & 0xFF;
    scroll_text_buffer[2] = (scroll.speed_ms >> 8) & 0xFF;
    scroll_text_buffer[3] = strlen(scroll.current_str) & 0xFF;

    memcpy(scroll_text_buffer + 4, scroll.current_str, scroll_text_buffer[3]);
    // Return the scroll text configuration
    chain_command_complete_return(CHAIN_MONO_GET_SCROLL_TEXT, scroll_text_buffer, scroll_text_buffer[3] + 4);
}

/**
 * @brief Command handler for controlling scroll text state
 * @param scroll_state Scroll state command to set
 * @retval None
 */
void chain_mono_set_scroll_text_state_handle(uint8_t scroll_state)
{
    s_return_status = OPERATION_FAIL;

    if (mono_mode == MONO_STRING_SCROLL_MODE) {
        if (scroll_state >= 0 && scroll_state <= 3) {
            chain_mono_scroll_text_control(scroll_state);
            s_return_status = OPERATION_SUCCESS;
        }
    } else {
        s_return_status = OPERATION_MODE_MISMATCH;
    }
    // Return the result of the set operation
    chain_command_complete_return(CHAIN_MONO_SET_SCROLL_TEXT_STATE, &s_return_status, 1);
}

/**
 * @brief Command handler for getting current scroll text state
 * @param None
 * @retval None
 */
void chain_mono_get_scroll_text_state_handle(void)
{
    chain_command_complete_return(CHAIN_MONO_GET_SCROLL_TEXT_STATE, &scroll.state, 1);
}

/**
 * @brief Command handler for displaying a single character
 * @param buffer Command buffer containing character and position
 * @param size Size of the command buffer
 * @retval None
 */
void chain_mono_set_char_handle(uint8_t *buffer, uint8_t size)
{
    s_return_status = OPERATION_FAIL;

    if (mono_mode == MONO_PIXEL_MODE) {
        if (buffer != NULL && size == 2) {
            uint8_t x = (buffer[1] >> 4) & 0b00001111;
            uint8_t y = buffer[1] & 0b00001111;
            chain_mono_show_char((char)buffer[0], x, y, &Font5x7, 7, 1);
            s_return_status = OPERATION_SUCCESS;
        }

    } else {
        s_return_status = OPERATION_MODE_MISMATCH;
    }

    // Return the result of the set operation
    chain_command_complete_return(CHAIN_MONO_SET_CHAR, &s_return_status, 1);
}

/**
 * @brief Command handler for clearing the display
 * @param None
 * @retval None
 */
void chain_mono_clear_handle(void)
{
    s_return_status = OPERATION_SUCCESS;
    if (mono_mode == MONO_STRING_SCROLL_MODE) {
        chain_mono_scroll_text_control(SCROLL_CMD_STOP_CLEAR);
    }
    chain_mono_clear();
    chain_mono_swap_buffer();

    // Return the result of the clear operation
    chain_command_complete_return(CHAIN_MONO_CLEAR, &s_return_status, 1);
}
