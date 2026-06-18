/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef RGB_FUNCTION_H_
#define RGB_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include "base_function.h"

#define RGB_NUM (64)  // Macro definition for the number of LEDs, indicating there is 1 LED

#define CODE_1 (52)  // Timer count for 1 code, used for controlling timing behavior for '1'

#define CODE_0 (25)  // Timer count for 0 code, used for controlling timing behavior for '0'

/** @defgroup Mono_Display_Constants
 * @brief Mono display configuration constants
 * @{*/
#define PWM_LEVELS             (32)               /*!< PWM levels for brightness control */
#define MAX_TEXT_LEN           (32)               /*!< Maximum length of scroll text */
#define VBUF_W                 (MAX_TEXT_LEN * 8) /*!< Virtual buffer width for scrolling text */
#define RAINBOW_HUE_MULTIPLIER (10)               /*!< Multiplier for rainbow hue speed and density */
/** @}*/

/* Structure to define the size of RGB color components for a single LED */
typedef struct {
    uint8_t R;        // Red component, range 0-255
    uint8_t G;        // Green component, range 0-255
    uint8_t B;        // Blue component, range 0-255
} RGB_Color_TypeDef;  // RGB_Color_TypeDef is used to store the RGB color values of the LED

typedef enum {
    CHAIN_RGB_SET_MODE              = 0x10, /*!< Set display mode */
    CHAIN_RGB_GET_MODE              = 0x11, /*!< Get current display mode */
    CHAIN_RGB_SET_PIXEL             = 0x30, /*!< Set individual pixel */
    CHAIN_RGB_SET_DISPLAY_BUFFER    = 0x31, /*!< Set entire display buffer */
    CHAIN_RGB_GET_PIXEL             = 0x32, /*!< Get individual pixel state */
    CHAIN_RGB_GET_DISPLAY_BUFFER    = 0x33, /*!< Get entire display buffer */
    CHAIN_RGB_SET_CHAR              = 0x34, /*!< Display single character */
    CHAIN_RGB_SET_SCROLL_TEXT       = 0x40, /*!< Set scroll text configuration */
    CHAIN_RGB_GET_SCROLL_TEXT       = 0x41, /*!< Get scroll text configuration */
    CHAIN_RGB_SET_SCROLL_TEXT_STATE = 0x42, /*!< Control scroll text state */
    CHAIN_RGB_GET_SCROLL_TEXT_STATE = 0x43, /*!< Get scroll text state */
    CHAIN_RGB_SET_ROTATION          = 0xE0, /*!< Set display rotation */
    CHAIN_RGB_GET_ROTATION          = 0xE1, /*!< Get current rotation */
    CHAIN_RGB_SET_BRIGHTNESS        = 0xE2, /*!< Set display brightness */
    CHAIN_RGB_GET_BRIGHTNESS        = 0xE3, /*!< Get current brightness */
    CHAIN_RGB_CLEAR                 = 0xE4, /*!< Clear display */
} chain_rgb_cmd_t;

typedef enum {
    RGB_PIXEL_MODE         = 0x00, /*!< Normal display mode */
    RGB_STRING_SCROLL_MODE = 0x01, /*!< String scroll display mode */
} rgb_mode_t;

/** @defgroup RGB_Rotation
 * @brief Screen rotation angles
 * @{*/
typedef enum {
    RGB_ROTATION_0   = 0, /*!< 0° rotation (default) */
    RGB_ROTATION_90  = 1, /*!< 90° clockwise rotation */
    RGB_ROTATION_180 = 2, /*!< 180° rotation */
    RGB_ROTATION_270 = 3  /*!< 270° clockwise rotation */
} chain_rgb_rotation_t;

/**
 * @brief Font structure definition
 */
typedef struct {
    uint8_t width;       /*!< Font width in columns */
    uint8_t height;      /*!< Font height in rows */
    uint8_t spacing;     /*!< Character spacing */
    const uint8_t *data; /*!< Pointer to font data */
} font_t;
/** @}*/

/** @defgroup Scroll_Config
 * @brief Scroll text configuration
 * @{*/
/**
 * @brief Scroll mode options
 */
typedef enum {
    SCROLL_MODE_ONCE   = 0, /*!< Play once and stop */
    SCROLL_MODE_LOOP   = 1, /*!< Loop scrolling continuously */
    SCROLL_MODE_BOUNCE = 2  /*!< Bounce back and forth */
} scroll_mode_t;

/**
 * @brief Scroll direction options
 */
typedef enum {
    SCROLL_LEFT  = 0, /*!< Scroll to the left */
    SCROLL_RIGHT = 1, /*!< Scroll to the right */
    SCROLL_UP    = 2, /*!< Scroll upwards */
    SCROLL_DOWN  = 3  /*!< Scroll downwards */
} scroll_dir_t;

/**
 * @brief Scroll control commands
 */
typedef enum {
    SCROLL_CMD_START      = 0, /*!< Start or resume scrolling */
    SCROLL_CMD_PAUSE      = 1, /*!< Pause scrolling (frame remains static) */
    SCROLL_CMD_STOP_CLEAR = 2  /*!< Stop and clear display */
} scroll_cmd_t;

/**
 * @brief Scroll internal state
 */
typedef enum {
    SCROLL_STATE_RUNNING = 0, /*!< Currently scrolling */
    SCROLL_STATE_PAUSED  = 1, /*!< Paused state */
    SCROLL_STATE_IDLE    = 2  /*!< Idle/stopped state */
} scroll_state_t;

/**
 * @brief Scroll context structure
 */
typedef struct {
    char current_str[MAX_TEXT_LEN + 1]; /*!< Current scroll text */
    scroll_dir_t dir;                   /*!< Scroll direction */
    scroll_mode_t mode;                 /*!< Scroll mode */
    uint16_t speed_ms;                  /*!< Scroll speed in milliseconds */
    scroll_state_t state;               /*!< Current running state */
    uint32_t last_tick;                 /*!< Last update tick time */
    int16_t offset_x;                   /*!< X-axis offset */
    int16_t step_x;                     /*!< X-axis step size */
    int16_t max_x;                      /*!< Maximum X-axis offset */
    int16_t offset_y;                   /*!< Y-axis offset */
    int16_t step_y;                     /*!< Y-axis step size */
    int16_t max_y;                      /*!< Maximum Y-axis offset */
    int16_t content_len;                /*!< Total content length */
    uint16_t color565;                  /*!< LED color in 565 format */
    uint32_t last_rainbow_tick;         /*!< Last rainbow update tick time */
    uint8_t rainbow_hue;                /*!< Current rainbow hue value (0-255) */
    uint16_t color_offset;              /*!< Color offset for cycle-based color shifting */
    uint16_t last_offset;               /*!< Last offset value to detect cycle completion */
    uint32_t total_scroll_steps;        /*!< Total scroll steps for continuous gradient */
} scroll_ctx_t;
/** @}*/

/** @defgroup RGB_Global_Variables
 * @brief RGB display global variables
 * @{*/
extern const font_t Font5x7; /*!< 5x7 font definition */

extern __IO uint8_t
    g_light;  // RGB brightness, an externally accessible variable representing the current brightness level
extern chain_rgb_rotation_t g_current_rotation; /*!< Current display rotation */
extern uint8_t s_rgb_update_flag;
extern __IO uint8_t g_send_complete_flag;
extern uint8_t s_rgb_buf[RGB_NUM + 10][24];

void chain_rgb_init(void);

/**
 * @brief Set the display operating mode
 * @param mode Display mode to set
 * @retval None
 */
void chain_rgb_set_mode(rgb_mode_t mode);

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
                         uint8_t clear_bg);

/**
 * @brief Control scroll text state (start, pause, stop)
 * @param cmd Scroll control command
 * @retval None
 */
void chain_rgb_scroll_text_control(scroll_cmd_t cmd);

/**
 * @brief Update scroll text display based on current state and mode
 * @param tick_ms Current system tick time in milliseconds
 * @retval None
 */
void chain_rgb_scroll_text_update(uint32_t tick_ms);

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
                                 scroll_mode_t mode, uint16_t speed_ms, uint16_t color565);

/**
 * @brief Set screen rotation angle
 * @param rotation Rotation angle (ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270)
 * @retval None
 */
void chain_rgb_set_rotation(chain_rgb_rotation_t rotation);

/**
 * @brief Display 8x8 grayscale pattern
 * @param pattern 8x8 grayscale array, each element 0~0xFFFF
 * @retval None
 */
void chain_rgb_display_pattern(const uint16_t pattern[8][8]);

/**
 * @brief Set LED state at specific position with rotation applied
 * @param x Column coordinate (0-7)
 * @param y Row coordinate (0-7)
 * @param color565 LED color in 565 format
 * @retval None
 */
void chain_rgb_set_pixel(uint8_t x, uint8_t y, uint16_t color565);

/**
 * @brief Set display brightness level with gamma correction
 * @param brightness Brightness level (0-100)
 * @retval None
 */
void chain_rgb_set_brightness(uint8_t brightness);

/**
 * @brief Update RGB display with current buffer content
 * @retval None
 */
void chain_rgb_update(void);

/**
 * @brief Clear the display buffer
 * @retval None
 */
void chain_rgb_clear(void);

/**
 * @brief Handle set mode command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_rgb_set_mode_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get mode command
 * @param None
 * @retval None
 */
void chain_rgb_get_mode_handle(void);

/**
 * @brief Handle set pixel command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_rgb_set_pixel_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get pixel command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_rgb_get_pixel_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle set display buffer command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_rgb_set_display_buffer_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get display buffer command
 * @param None
 * @retval None
 */
void chain_rgb_get_display_buffer_handle(void);

/**
 * @brief Handle set rotation command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_rgb_set_rotation_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get rotation command
 * @param None
 * @retval None
 */
void chain_rgb_get_rotation_handle(void);

/**
 * @brief Handle set brightness command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_rgb_set_brightness_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get brightness command
 * @param None
 * @retval None
 */
void chain_rgb_get_brightness_handle(void);

/**
 * @brief Handle set scroll text command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_rgb_set_scroll_text_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get scroll text command
 * @param None
 * @retval None
 */
void chain_rgb_get_scroll_text_handle(void);

/**
 * @brief Handle set scroll text state command
 * @param scroll_state Scroll state to set
 * @retval None
 */
void chain_rgb_set_scroll_text_state_handle(uint8_t scroll_state);

/**
 * @brief Handle get scroll text state command
 * @param None
 * @retval None
 */
void chain_rgb_get_scroll_text_state_handle(void);

/**
 * @brief Handle set char command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_rgb_set_char_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle clear command
 * @param None
 * @retval None
 */
void chain_rgb_clear_handle(void);
/** @}*/

#ifdef __cplusplus
}
#endif

#endif /* RGB_FUNCTION_H_ */
