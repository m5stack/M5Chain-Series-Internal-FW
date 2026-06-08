/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MONO_FUNCTION_H_
#define MONO_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include "base_function.h"

/** @defgroup Mono_Display_Constants
 * @brief Mono display configuration constants
 * @{*/
#define PWM_LEVELS   (16)               /*!< PWM levels for brightness control */
#define MAX_TEXT_LEN (32)               /*!< Maximum length of scroll text */
#define VBUF_W       (MAX_TEXT_LEN * 8) /*!< Virtual buffer width for scrolling text */
/** @}*/

/** @defgroup Mono_LED_Control
 * @brief LED matrix control macros
 * @{*/
#define PIN2MODER(pin) (0x3u << (__builtin_ctz(pin) * 2)) /*!< Convert pin to MODE register mask */
/**
 * @brief Mask for all LED pins MODE register
 * @note Covers all LED pins: PA0, PA1, PA4-PA8, PA11-PA12
 */
#define LED_ALL_MODER_MASK                                                                                \
    (PIN2MODER(LED_PA0_Pin) | PIN2MODER(LED_PA1_Pin) | PIN2MODER(LED_PA4_Pin) | PIN2MODER(LED_PA5_Pin) |  \
     PIN2MODER(LED_PA6_Pin) | PIN2MODER(LED_PA7_Pin) | PIN2MODER(LED_PA8_Pin) | PIN2MODER(LED_PA11_Pin) | \
     PIN2MODER(LED_PA12_Pin))
/** @}*/

/** @defgroup Mono_Commands
 * @brief Mono display command definitions
 * @{*/
typedef enum {
    CHAIN_MONO_SET_MODE              = 0x10, /*!< Set display mode */
    CHAIN_MONO_GET_MODE              = 0x11, /*!< Get current display mode */
    CHAIN_MONO_SET_PIXEL             = 0x30, /*!< Set individual pixel */
    CHAIN_MONO_SET_DISPLAY_BUFFER    = 0x31, /*!< Set entire display buffer */
    CHAIN_MONO_GET_PIXEL             = 0x32, /*!< Get individual pixel state */
    CHAIN_MONO_GET_DISPLAY_BUFFER    = 0x33, /*!< Get entire display buffer */
    CHAIN_MONO_SET_CHAR              = 0x34, /*!< Display single character */
    CHAIN_MONO_SET_SCROLL_TEXT       = 0x40, /*!< Set scroll text configuration */
    CHAIN_MONO_GET_SCROLL_TEXT       = 0x41, /*!< Get scroll text configuration */
    CHAIN_MONO_SET_SCROLL_TEXT_STATE = 0x42, /*!< Control scroll text state */
    CHAIN_MONO_GET_SCROLL_TEXT_STATE = 0x43, /*!< Get scroll text state */
    CHAIN_MONO_SET_ROTATION          = 0xE0, /*!< Set display rotation */
    CHAIN_MONO_GET_ROTATION          = 0xE1, /*!< Get current rotation */
    CHAIN_MONO_SET_BRIGHTNESS        = 0xE2, /*!< Set display brightness */
    CHAIN_MONO_GET_BRIGHTNESS        = 0xE3, /*!< Get current brightness */
    CHAIN_MONO_CLEAR                 = 0xE4, /*!< Clear display */
} chain_mono_cmd_t;
/** @}*/

/** @defgroup Mono_Modes
 * @brief Mono display operating modes
 * @{*/
typedef enum {
    MONO_PIXEL_MODE         = 0x00, /*!< Normal display mode */
    MONO_STRING_SCROLL_MODE = 0x01, /*!< String scroll display mode */
} mono_mode_t;
/** @}*/

/** @defgroup Mono_Rotation
 * @brief Screen rotation angles
 * @{*/
typedef enum {
    ROTATION_0   = 0, /*!< 0° rotation (default) */
    ROTATION_90  = 1, /*!< 90° clockwise rotation */
    ROTATION_180 = 2, /*!< 180° rotation */
    ROTATION_270 = 3  /*!< 270° clockwise rotation */
} chain_mono_rotation_t;
/** @}*/

/** @defgroup Mono_Data_Structures
 * @brief Mono display data structures
 * @{*/
/**
 * @brief LED pin mapping structure
 */
typedef struct {
    uint16_t anode_pin;   /*!< Anode pin (X-axis) */
    uint16_t cathode_pin; /*!< Cathode pin (Y-axis) */
} led_pin_map_t;

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
} scroll_ctx_t;
/** @}*/

/** @defgroup Mono_Global_Variables
 * @brief Mono display global variables
 * @{*/
extern const font_t Font5x7;                     /*!< 5x7 font definition */
extern chain_mono_rotation_t g_current_rotation; /*!< Current display rotation */
extern uint8_t g_brightness_level;               /*!< Brightness level (0-7, default: 7) */

extern uint8_t buf_front[9][8];
extern const uint16_t comp_table[9];
extern const uint8_t gamma_table[8];
extern const uint16_t led_refresh_map[9];
extern uint8_t s_col;

/** @}*/

/** @defgroup Mono_Common_Functions
 * @brief Common mono display functions
 * @{*/

/**
 * @brief Set the display operating mode
 * @param mode Display mode to set
 * @retval None
 */
void chain_mono_set_mode(mono_mode_t mode);

/**
 * @brief Turn off all LEDs
 * @param None
 * @retval None
 */
void chain_mono_all_off(void);

/**
 * @brief Clear the display buffer
 * @param None
 * @retval None
 */
void chain_mono_clear(void);

/**
 * @brief Set screen rotation angle
 * @param rotation Rotation angle (ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270)
 * @retval None
 */
void chain_mono_set_rotation(chain_mono_rotation_t rotation);

/**
 * @brief Get current rotation angle
 * @param None
 * @retval Current rotation angle
 */
chain_mono_rotation_t chain_mono_get_rotation(void);

/**
 * @brief Set LED state at specific position (with rotation applied)
 * @param x Column coordinate (0-7)
 * @param y Row coordinate (0-7)
 * @param brightness LED brightness (0-7)
 * @retval None
 */
void chain_mono_set_pixel(uint8_t x, uint8_t y, uint8_t brightness);

/**
 * @brief Get LED state at specific position (with rotation applied)
 * @param x Column coordinate (0-7)
 * @param y Row coordinate (0-7)
 * @retval LED brightness (0-7)
 */
uint8_t chain_mono_get_pixel(uint8_t x, uint8_t y);

/**
 * @brief LED matrix scanning function
 * @note Must be called in main loop or timer, recommended 1-5ms interval
 * @param None
 * @retval None
 */
void chain_mono_scan(void);

/**
 * @brief Display 8x8 grayscale pattern
 * @param pattern 8x8 grayscale array, each element 0~7
 * @retval None
 */
void chain_mono_display_pattern(const uint8_t pattern[8][8]);

/**
 * @brief Display single character at specified position
 * @param ch Character to display (ASCII 32-127)
 * @param x_offset X-axis offset (0-7)
 * @param y_offset Y-axis offset (0-7)
 * @param font Font pointer
 * @param brightness Brightness (0-7)
 * @param clear_bg Clear background before displaying (0: no, 1: yes)
 * @retval None
 */
void chain_mono_show_char(char ch, uint8_t x_offset, uint8_t y_offset, const font_t *font, uint8_t brightness,
                          uint8_t clear_bg);

/**
 * @brief Set display brightness
 * @param brightness Brightness level (0-7)
 * @retval None
 */
void chain_mono_set_brightness(uint8_t brightness);

/**
 * @brief Start scroll text display
 * @param str String to scroll
 * @param str_length String length
 * @param font Font pointer
 * @param dir Scroll direction
 * @param mode Scroll mode
 * @param speed_ms Scroll speed in milliseconds
 * @param brightness Brightness (0-7)
 * @retval None
 */
void chain_mono_scroll_text_start(char *str, uint8_t str_length, const font_t *font, scroll_dir_t dir,
                                  scroll_mode_t mode, uint16_t speed_ms, uint8_t brightness);

/**
 * @brief Update scroll text display
 * @param tick_ms Current tick time in milliseconds
 * @retval None
 */
void chain_mono_scroll_text_update(uint32_t tick_ms);

/**
 * @brief Control scroll text state
 * @param cmd Scroll control command
 * @retval None
 */
void chain_mono_scroll_text_control(scroll_cmd_t cmd);

/** @}*/

/** @defgroup Mono_Command_Handlers
 * @brief Command handler functions
 * @{*/

/**
 * @brief Handle set mode command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_mono_set_mode_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get mode command
 * @param None
 * @retval None
 */
void chain_mono_get_mode_handle(void);

/**
 * @brief Handle set pixel command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_mono_set_pixel_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get pixel command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_mono_get_pixel_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle set display buffer command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_mono_set_display_buffer_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get display buffer command
 * @param None
 * @retval None
 */
void chain_mono_get_display_buffer_handle(void);

/**
 * @brief Handle set rotation command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_mono_set_rotation_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get rotation command
 * @param None
 * @retval None
 */
void chain_mono_get_rotation_handle(void);

/**
 * @brief Handle set brightness command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_mono_set_brightness_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get brightness command
 * @param None
 * @retval None
 */
void chain_mono_get_brightness_handle(void);

/**
 * @brief Handle set scroll text command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_mono_set_scroll_text_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle get scroll text command
 * @param None
 * @retval None
 */
void chain_mono_get_scroll_text_handle(void);

/**
 * @brief Handle set scroll text state command
 * @param scroll_state Scroll state to set
 * @retval None
 */
void chain_mono_set_scroll_text_state_handle(uint8_t scroll_state);

/**
 * @brief Handle get scroll text state command
 * @param None
 * @retval None
 */
void chain_mono_get_scroll_text_state_handle(void);

/**
 * @brief Handle set char command
 * @param buffer Command buffer
 * @param size Buffer size
 * @retval None
 */
void chain_mono_set_char_handle(uint8_t *buffer, uint8_t size);

/**
 * @brief Handle clear command
 * @param None
 * @retval None
 */
void chain_mono_clear_handle(void);

void led_power_test(uint8_t col, uint8_t count);

/** @}*/

#ifdef __cplusplus
}
#endif

#endif /* MONO_FUNCTION_H_ */
