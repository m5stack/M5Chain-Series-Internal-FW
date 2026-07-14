/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "servos2_chain_function.h"
#include "base_function.h"
#include "user_sys.h"

// Buffer for storing return data to I2C host
// 用于存储返回给 I2C 主机的返回数据的缓冲区
static uint8_t s_ret_buf[128] = {0};
static uint8_t s_ret_buf_size = 0;

// --- Global Variable Definitions ---
// --- 全局变量定义 ---

volatile uint8_t gpio_servo_angle_changed[8]  = {0};  // Flag for Servo angle change / 舵机角度变化标志
volatile uint8_t gpio_servo_angle_changed_any = 0;  // Flag for ANY Servo angle change / 任意舵机角度变化标志
volatile uint8_t gpio_pwm_duty_changed[8]     = {0};  // Flag for PWM duty change / PWM 占空比变化标志
volatile uint8_t gpio_pwm_duty_changed_any    = 0;    // Flag for ANY PWM duty change / 任意 PWM 占空比变化标志
volatile uint8_t gpio_rgb_buf_changed_any     = 0;    // Flag for RGB buffer update / RGB 缓冲区更新标志
volatile uint8_t tim_freq_changed[2]          = {0};  // Flag for Timer freq change / 定时器频率变化标志
volatile uint8_t tim_freq_changed_any = 0;  // Flag for ANY Timer freq change / 任意定时器频率变化标志

// Default Register Values
// 默认寄存器值初始化
volatile uint8_t gpio_mode_reg[8]               = {USER_GPIO_INPUT_MODE, USER_GPIO_INPUT_MODE, USER_GPIO_INPUT_MODE,
                                     USER_GPIO_INPUT_MODE, USER_GPIO_INPUT_MODE, USER_GPIO_INPUT_MODE,
                                     USER_GPIO_INPUT_MODE, USER_GPIO_INPUT_MODE};
volatile uint8_t gpio_gpio_input_pu_pd_reg[8]   = {0};
volatile uint8_t gpio_gpio_output_status_reg[8] = {0};
volatile uint16_t gpio_adc_value_reg[8]         = {0};
volatile uint8_t gpio_servo_angle_reg[8]        = {0};
volatile rgb_config_t gpio_rgb_reg[8]           = {0};
volatile uint8_t gpio_pwm_duty_reg[8]           = {0};
volatile uint32_t gpio_rgb_color_buf_reg[16]    = {0};
volatile user_sys_time_config_t tim_freq_reg[2] = {0};
volatile uint16_t vref_mv_reg                   = SYS_VREF_MV;
volatile uint16_t grove_voltage_mv_reg          = 0;
volatile uint16_t dc_voltage_mv_reg             = 0;
volatile uint16_t sys_current_ma_reg            = 0;
volatile uint8_t fw_version_reg                 = FIRMWARE_VERSION;
volatile uint8_t i2c_addr_reg                   = 0;

/**
 * @brief Set the mode for a single GPIO.
 *        设置单个GPIO的模式。
 * @param gpio GPIO index (0-7) / GPIO 索引
 * @param mode Mode to set / 要设置的模式
 */
void servos2_chain_set_mode_single(uint8_t gpio, uint8_t mode)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX && mode <= USER_GPIO_PWM_MODE) {
        if (gpio_mode_reg[gpio] != mode) {
            gpio_mode_reg[gpio] = mode;
            user_sys_gpio_mode_update(gpio);
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_MODE_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set the mode for all GPIOs at once.
 *        一次性设置所有GPIO的模式。
 * @param mode Pointer to array of modes (size must be GPIO_NUM_MAX) / 模式数组指针
 * @param size Size of the array / 数组大小
 */
void servos2_chain_set_mode_all(uint8_t *mode, uint8_t size)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (size == GPIO_NUM_MAX) {
        uint8_t error_count = 0;
        // Verify all modes are valid first / 首先验证所有模式是否有效
        for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
            if (mode[gpio] > USER_GPIO_PWM_MODE) {
                error_count++;
            }
        }
        if (error_count == 0) {
            for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
                if (gpio_mode_reg[gpio] != mode[gpio]) {
                    gpio_mode_reg[gpio] = mode[gpio];
                    user_sys_gpio_mode_update(gpio);
                }
            }
        } else {
            operation_result = OPERATION_FAIL;
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_MODE_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get the mode of a single GPIO.
 *        获取单个GPIO的模式。
 * @param gpio GPIO index / GPIO 索引
 */
void servos2_chain_get_mode_single(uint8_t gpio)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX) {
        s_ret_buf[s_ret_buf_size++] = operation_result;
        s_ret_buf[s_ret_buf_size++] = gpio_mode_reg[gpio];
    } else {
        operation_result            = OPERATION_FAIL;
        s_ret_buf[s_ret_buf_size++] = operation_result;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_MODE_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get the mode of all GPIOs.
 *        获取所有GPIO的模式。
 */
void servos2_chain_get_mode_all(void)
{
    s_ret_buf_size = 0;
    for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
        s_ret_buf[s_ret_buf_size++] = gpio_mode_reg[gpio];
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_MODE_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set input pull-up/pull-down resistor for a single GPIO.
 *        设置单个GPIO的输入上下拉电阻。
 */
void servos2_chain_set_input_pu_pd_single(uint8_t gpio, uint8_t pu_pd)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX && pu_pd <= USER_GPIO_PULL_DOWN) {
        if (gpio_gpio_input_pu_pd_reg[gpio] != pu_pd) {
            gpio_gpio_input_pu_pd_reg[gpio] = pu_pd;
            user_sys_gpio_mode_update(gpio);
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_INPUT_PU_PD_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set input pull-up/pull-down resistor for all GPIOs.
 *        设置所有GPIO的输入上下拉电阻。
 */
void servos2_chain_set_input_pu_pd_all(uint8_t *pu_pd, uint8_t size)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (size == GPIO_NUM_MAX) {
        uint8_t error_count = 0;
        for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
            if (pu_pd[gpio] > USER_GPIO_PULL_DOWN) {
                error_count++;
            }
        }
        if (error_count == 0) {
            for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
                if (gpio_gpio_input_pu_pd_reg[gpio] != pu_pd[gpio]) {
                    gpio_gpio_input_pu_pd_reg[gpio] = pu_pd[gpio];
                    user_sys_gpio_mode_update(gpio);
                }
            }
        } else {
            operation_result = OPERATION_FAIL;
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_INPUT_PU_PD_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get input pull-up/pull-down config for a single GPIO.
 *        获取单个GPIO的输入上下拉配置。
 */
void servos2_chain_get_input_pu_pd_single(uint8_t gpio)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX) {
        s_ret_buf[s_ret_buf_size++] = operation_result;
        s_ret_buf[s_ret_buf_size++] = gpio_gpio_input_pu_pd_reg[gpio];
    } else {
        operation_result            = OPERATION_FAIL;
        s_ret_buf[s_ret_buf_size++] = operation_result;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_INPUT_PU_PD_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get input pull-up/pull-down config for all GPIOs.
 *        获取所有GPIO的输入上下拉配置。
 */
void servos2_chain_get_input_pu_pd_all(void)
{
    s_ret_buf_size = 0;
    for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
        s_ret_buf[s_ret_buf_size++] = gpio_gpio_input_pu_pd_reg[gpio];
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_INPUT_PU_PD_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get digital input level (high/low) for a single GPIO.
 *        获取单个GPIO的数字输入电平（高/低）。
 */
void servos2_chain_get_input_status_single(uint8_t gpio)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX) {
        s_ret_buf[s_ret_buf_size++] = operation_result;
        s_ret_buf[s_ret_buf_size++] = user_input_get_level(gpio);
    } else {
        operation_result            = OPERATION_FAIL;
        s_ret_buf[s_ret_buf_size++] = operation_result;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_INPUT_STATUS_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get digital input level for all GPIOs.
 *        获取所有GPIO的数字输入电平。
 */
void servos2_chain_get_input_status_all(void)
{
    s_ret_buf_size = 0;
    for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
        s_ret_buf[s_ret_buf_size++] = user_input_get_level(gpio);
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_INPUT_STATUS_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set digital output level for a single GPIO.
 *        设置单个GPIO的数字输出电平。
 */
void servos2_chain_set_output_status_single(uint8_t gpio, uint8_t status)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX) {
        if (gpio_gpio_output_status_reg[gpio] != status) {
            gpio_gpio_output_status_reg[gpio] = status;
            if (gpio_mode_reg[gpio] == USER_GPIO_OUTPUT_MODE) {
                user_output_set_level(gpio);
            }
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_OUTPUT_STATUS_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set digital output level for all GPIOs.
 *        设置所有GPIO的数字输出电平。
 */
void servos2_chain_set_output_status_all(uint8_t *status, uint8_t size)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (size == GPIO_NUM_MAX) {
        uint8_t error_count = 0;
        for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
            if (status[gpio] > USER_GPIO_LEVEL_HIGH) {
                error_count++;
            }
        }
        if (error_count == 0) {
            for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
                if (status[gpio] != gpio_gpio_output_status_reg[gpio]) {
                    gpio_gpio_output_status_reg[gpio] = status[gpio];
                    if (gpio_mode_reg[gpio] == USER_GPIO_OUTPUT_MODE) {
                        user_output_set_level(gpio);
                    }
                }
            }
        } else {
            operation_result = OPERATION_FAIL;
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_OUTPUT_STATUS_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get digital output status (register value) for a single GPIO.
 *        获取单个GPIO的数字输出状态（寄存器值）。
 */
void servos2_chain_get_output_status_single(uint8_t gpio)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX) {
        s_ret_buf[s_ret_buf_size++] = operation_result;
        s_ret_buf[s_ret_buf_size++] = gpio_gpio_output_status_reg[gpio];
    } else {
        operation_result            = OPERATION_FAIL;
        s_ret_buf[s_ret_buf_size++] = operation_result;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_OUTPUT_STATUS_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get digital output status for all GPIOs.
 *        获取所有GPIO的数字输出状态。
 */
void servos2_chain_get_output_status_all(void)
{
    s_ret_buf_size = 0;
    for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
        s_ret_buf[s_ret_buf_size++] = gpio_gpio_output_status_reg[gpio];
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_OUTPUT_STATUS_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get ADC value (16-bit) for a single GPIO.
 *        获取单个GPIO的ADC值（16位）。
 *        Little Endian (Low Byte First)
 */
void servos2_chain_get_adc_value_single(uint8_t gpio)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX) {
        uint16_t adc_value          = 0;
        s_ret_buf[s_ret_buf_size++] = operation_result;  // Byte 0: Status
        if (gpio_mode_reg[gpio] == USER_GPIO_ADC_MODE) {
            adc_value                   = user_adc_get_adc_value(user_sys_gpio_pin_config[gpio].adc_channel);
            s_ret_buf[s_ret_buf_size++] = adc_value & 0xFF;         // Byte 1: Low Byte
            s_ret_buf[s_ret_buf_size++] = (adc_value >> 8) & 0xFF;  // Byte 2: High Byte
        } else {
            s_ret_buf[s_ret_buf_size++] = 0;
            s_ret_buf[s_ret_buf_size++] = 0;
        }
    } else {
        operation_result            = OPERATION_FAIL;
        s_ret_buf[s_ret_buf_size++] = operation_result;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_ADC_VALUE_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get ADC values for all GPIOs.
 *        获取所有GPIO的ADC值。
 *        Little Endian (Low Byte First)
 */
void servos2_chain_get_adc_value_all(void)
{
    s_ret_buf_size = 0;
    for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
        if (gpio_mode_reg[gpio] == USER_GPIO_ADC_MODE) {
            uint16_t adc_value          = user_adc_get_adc_value(user_sys_gpio_pin_config[gpio].adc_channel);
            s_ret_buf[s_ret_buf_size++] = adc_value & 0xFF;         // Low Byte
            s_ret_buf[s_ret_buf_size++] = (adc_value >> 8) & 0xFF;  // High Byte
        } else {
            s_ret_buf[s_ret_buf_size++] = 0;
            s_ret_buf[s_ret_buf_size++] = 0;
        }
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_ADC_VALUE_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set Servo angle (0-180) for a single GPIO.
 *        设置单个GPIO的舵机角度（0-180）。
 */
void servos2_chain_set_servo_angle_single(uint8_t gpio, uint8_t angle)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX && angle <= 180) {
        if (gpio_servo_angle_reg[gpio] != angle) {
            gpio_servo_angle_reg[gpio] = angle;
            // Only flag update if currently in Servo mode
            // 仅在当前处于舵机模式时标记更新
            if (gpio_mode_reg[gpio] == USER_GPIO_SERVO_MODE) {
                gpio_servo_angle_changed[gpio] = 1;
                gpio_servo_angle_changed_any   = 1;
            }
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_SERVO_ANGLE_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set Servo angle for all GPIOs.
 *        设置所有GPIO的舵机角度。
 */
void servos2_chain_set_servo_angle_all(uint8_t *angle, uint8_t size)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (size == GPIO_NUM_MAX) {
        uint8_t error_count = 0;
        for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
            if (angle[gpio] > 180) {
                error_count++;
            }
        }
        if (error_count == 0) {
            for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
                if (gpio_servo_angle_reg[gpio] != angle[gpio]) {
                    gpio_servo_angle_reg[gpio] = angle[gpio];
                    if (gpio_mode_reg[gpio] == USER_GPIO_SERVO_MODE) {
                        gpio_servo_angle_changed[gpio] = 1;
                        gpio_servo_angle_changed_any   = 1;
                    }
                }
            }
        } else {
            operation_result = OPERATION_FAIL;
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_SERVO_ANGLE_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get Servo angle of a single GPIO.
 *        获取单个GPIO的舵机角度。
 */
void servos2_chain_get_servo_angle_single(uint8_t gpio)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX) {
        s_ret_buf[s_ret_buf_size++] = operation_result;
        s_ret_buf[s_ret_buf_size++] = gpio_servo_angle_reg[gpio];
    } else {
        operation_result            = OPERATION_FAIL;
        s_ret_buf[s_ret_buf_size++] = operation_result;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_SERVO_ANGLE_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get Servo angle of all GPIOs.
 *        获取所有GPIO的舵机角度。
 */
void servos2_chain_get_servo_angle_all(void)
{
    s_ret_buf_size = 0;
    for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
        s_ret_buf[s_ret_buf_size++] = gpio_servo_angle_reg[gpio];
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_SERVO_ANGLE_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set RGB configuration for a single GPIO.
 *        设置单个GPIO的RGB配置。
 */
void servos2_chain_set_rgb_config_single(uint8_t gpio, uint8_t rgb)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    rgb_config_t rgb_temp;
    rgb_temp.rgb_config.value = rgb;
    // Check if gpio is valid and LED index (num) is within bounds (assuming 0-16 LEDs supported)
    // 检查GPIO是否有效以及LED索引(num)是否在范围内（假设支持0-16个LED）
    if (gpio < GPIO_NUM_MAX && rgb_temp.rgb_config.flags.num <= 16) {
        gpio_rgb_reg[gpio].rgb_config.value = rgb_temp.rgb_config.value;
        user_sys_rgb_update(gpio);
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_RGB_CONFIG_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set RGB configuration for all GPIOs.
 *        设置所有GPIO的RGB配置。
 */
void servos2_chain_set_rgb_config_all(uint8_t *rgb, uint8_t size)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (size == GPIO_NUM_MAX) {
        uint8_t error_count = 0;
        for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
            rgb_config_t rgb_temp;
            rgb_temp.rgb_config.value = rgb[gpio];
            if (rgb_temp.rgb_config.flags.num > 16) {
                error_count++;
            }
        }
        if (error_count == 0) {
            for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
                gpio_rgb_reg[gpio].rgb_config.value = rgb[gpio];
                user_sys_rgb_update(gpio);
            }
        } else {
            operation_result = OPERATION_FAIL;
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_RGB_CONFIG_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get RGB configuration of a single GPIO.
 *        获取单个GPIO的RGB配置。
 */
void servos2_chain_get_rgb_config_single(uint8_t gpio)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX) {
        s_ret_buf[s_ret_buf_size++] = operation_result;
        s_ret_buf[s_ret_buf_size++] = gpio_rgb_reg[gpio].rgb_config.value;
    } else {
        operation_result            = OPERATION_FAIL;
        s_ret_buf[s_ret_buf_size++] = operation_result;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_RGB_CONFIG_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get RGB configuration of all GPIOs.
 *        获取所有GPIO的RGB配置。
 */
void servos2_chain_get_rgb_config_all(void)
{
    s_ret_buf_size = 0;
    for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
        s_ret_buf[s_ret_buf_size++] = gpio_rgb_reg[gpio].rgb_config.value;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_RGB_CONFIG_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set a specific RGB LED color in the buffer.
 *        在缓冲区中设置特定RGB LED的颜色。
 * @param buffer [Index, R, G, B]
 */
void servos2_chain_set_rgb_buffer_single(uint8_t *buffer, uint8_t size)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (buffer != NULL && size == 4 && buffer[0] < 16) {
        // buffer[0]: index, buffer[1]: R, buffer[2]: G, buffer[3]: B
        user_rgb_buffer_update(buffer[0], buffer[1], buffer[2], buffer[3]);
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_RGB_BUFFER_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set all RGB LED colors in the buffer.
 *        设置缓冲区中所有RGB LED的颜色。
 * @param buffer Array of [R, G, B, R, G, B, ...]
 */
void servos2_chain_set_rgb_buffer_all(uint8_t *buffer, uint8_t size)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    // Expecting 16 LEDs * 3 bytes (RGB) = 48 bytes
    if (buffer != NULL && size == 3 * 16) {
        for (uint8_t i = 0; i < 16; i++) {
            user_rgb_buffer_update(i, buffer[i * 3], buffer[i * 3 + 1], buffer[i * 3 + 2]);
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_RGB_BUFFER_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get color of a specific RGB LED from buffer.
 *        从缓冲区获取特定RGB LED的颜色。
 */
void servos2_chain_get_rgb_buffer_single(uint8_t index)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (index < 16) {
        s_ret_buf[s_ret_buf_size++] = operation_result;
        s_ret_buf[s_ret_buf_size++] = user_rgb_buffer_get(index, 0);  // R
        s_ret_buf[s_ret_buf_size++] = user_rgb_buffer_get(index, 1);  // G
        s_ret_buf[s_ret_buf_size++] = user_rgb_buffer_get(index, 2);  // B
    } else {
        operation_result            = OPERATION_FAIL;
        s_ret_buf[s_ret_buf_size++] = operation_result;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_RGB_BUFFER_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get all RGB LED colors from buffer.
 *        从缓冲区获取所有RGB LED的颜色。
 */
void servos2_chain_get_rgb_buffer_all(void)
{
    s_ret_buf_size = 0;
    for (uint8_t i = 0; i < 16; i++) {
        s_ret_buf[s_ret_buf_size++] = user_rgb_buffer_get(i, 0);
        s_ret_buf[s_ret_buf_size++] = user_rgb_buffer_get(i, 1);
        s_ret_buf[s_ret_buf_size++] = user_rgb_buffer_get(i, 2);
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_RGB_BUFFER_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set PWM duty cycle (0-100%) for a single GPIO.
 *        设置单个GPIO的PWM占空比（0-100%）。
 */
void servos2_chain_set_pwm_duty_single(uint8_t gpio, uint8_t duty)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX && duty <= 100) {
        if (gpio_pwm_duty_reg[gpio] != duty) {
            gpio_pwm_duty_reg[gpio] = duty;
            if (gpio_mode_reg[gpio] == USER_GPIO_PWM_MODE) {
                gpio_pwm_duty_changed[gpio] = 1;
                gpio_pwm_duty_changed_any   = 1;
            }
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_PWM_DUTY_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set PWM duty cycle for all GPIOs.
 *        设置所有GPIO的PWM占空比。
 */
void servos2_chain_set_pwm_duty_all(uint8_t *duty, uint8_t size)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (duty != NULL && size == GPIO_NUM_MAX) {
        uint8_t error_count = 0;
        for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
            if (duty[gpio] > 100) {
                error_count++;
            }
        }
        if (error_count == 0) {
            for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
                if (gpio_pwm_duty_reg[gpio] != duty[gpio]) {
                    gpio_pwm_duty_reg[gpio] = duty[gpio];
                    if (gpio_mode_reg[gpio] == USER_GPIO_PWM_MODE) {
                        gpio_pwm_duty_changed[gpio] = 1;
                        gpio_pwm_duty_changed_any   = 1;
                    }
                }
            }
        } else {
            operation_result = OPERATION_FAIL;
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_PWM_DUTY_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get PWM duty cycle of a single GPIO.
 *        获取单个GPIO的PWM占空比。
 */
void servos2_chain_get_pwm_duty_single(uint8_t gpio)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (gpio < GPIO_NUM_MAX) {
        s_ret_buf[s_ret_buf_size++] = operation_result;
        s_ret_buf[s_ret_buf_size++] = gpio_pwm_duty_reg[gpio];
    } else {
        operation_result            = OPERATION_FAIL;
        s_ret_buf[s_ret_buf_size++] = operation_result;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_PWM_DUTY_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get PWM duty cycle of all GPIOs.
 *        获取所有GPIO的PWM占空比。
 */
void servos2_chain_get_pwm_duty_all(void)
{
    s_ret_buf_size = 0;
    for (uint8_t gpio = 0; gpio < GPIO_NUM_MAX; gpio++) {
        s_ret_buf[s_ret_buf_size++] = gpio_pwm_duty_reg[gpio];
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_PWM_DUTY_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set timer configuration (frequency) for a single timer.
 *        设置单个定时器的配置（频率）。
 *        Protocol: [TimerIndex, Freq_LowByte, Freq_HighByte]
 *        协议: [定时器索引, 频率低字节, 频率高字节]
 */
void servos2_chain_set_time_config_single(uint8_t *buffer, uint8_t size)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;

    // Buffer layout:
    // buffer[0]: Timer Index (0 or 1)
    // buffer[1]: Frequency Low Byte  (LSB) / 频率低字节
    // buffer[2]: Frequency High Byte (MSB) / 频率高字节
    if (buffer != NULL && size == 3 && buffer[0] < 2) {
        // Prevent setting frequency to 0
        if (buffer[1] == 0 && buffer[2] == 0) {
            operation_result = OPERATION_FAIL;
        } else {
            // Combine Low and High bytes: (High << 8) | Low
            uint16_t new_freq = (uint16_t)((buffer[2] << 8) | buffer[1]);

            if (new_freq != tim_freq_reg[buffer[0]].freq) {
                tim_freq_reg[buffer[0]].freq      = new_freq;
                tim_freq_reg[buffer[0]].buffer[0] = buffer[1];  // Store Low
                tim_freq_reg[buffer[0]].buffer[1] = buffer[2];  // Store High
                tim_freq_changed[buffer[0]]       = 1;
                tim_freq_changed_any              = 1;
            }
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_TIME_CONFIG_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Set timer configuration for all timers.
 *        设置所有定时器的配置。
 *        Protocol: [T0_Low, T0_High, T1_Low, T1_High]
 *        协议: [T0低字节, T0高字节, T1低字节, T1高字节]
 */
void servos2_chain_set_time_config_all(uint8_t *buffer, uint8_t size)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;

    // Buffer layout:
    // buffer[0]: Timer 0 Frequency Low Byte
    // buffer[1]: Timer 0 Frequency High Byte
    // buffer[2]: Timer 1 Frequency Low Byte
    // buffer[3]: Timer 1 Frequency High Byte
    if (buffer != NULL && size == 4) {
        // Check for zero frequency in either timer
        if ((buffer[0] == 0 && buffer[1] == 0) || (buffer[2] == 0 && buffer[3] == 0)) {
            operation_result = OPERATION_FAIL;
        } else {
            // --- Process Timer 0 ---
            // Low byte at index 0, High byte at index 1
            uint16_t freq_t0 = (uint16_t)((buffer[1] << 8) | buffer[0]);

            if (freq_t0 != tim_freq_reg[0].freq) {
                tim_freq_reg[0].freq      = freq_t0;
                tim_freq_reg[0].buffer[0] = buffer[0];  // Save Low
                tim_freq_reg[0].buffer[1] = buffer[1];  // Save High
                tim_freq_changed[0]       = 1;
                tim_freq_changed_any      = 1;
            }

            // --- Process Timer 1 ---
            // Low byte at index 2, High byte at index 3
            uint16_t freq_t1 = (uint16_t)((buffer[3] << 8) | buffer[2]);

            if (freq_t1 != tim_freq_reg[1].freq) {
                tim_freq_reg[1].freq      = freq_t1;
                tim_freq_reg[1].buffer[0] = buffer[2];  // Save Low
                tim_freq_reg[1].buffer[1] = buffer[3];  // Save High
                tim_freq_changed[1]       = 1;
                tim_freq_changed_any      = 1;
            }
        }
    } else {
        operation_result = OPERATION_FAIL;
    }
    s_ret_buf[s_ret_buf_size++] = operation_result;
    chain_command_complete_return(SERVOS2_CHAIN_SET_TIME_CONFIG_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get timer configuration (frequency) for a single timer.
 *        获取单个定时器的配置（频率）。
 *        Little Endian (Low Byte First)
 */
void servos2_chain_get_time_config_single(uint8_t time)
{
    operation_t operation_result = OPERATION_SUCCESS;
    s_ret_buf_size               = 0;
    if (time < 2) {
        s_ret_buf[s_ret_buf_size++] = operation_result;
        s_ret_buf[s_ret_buf_size++] = tim_freq_reg[time].freq & 0xFF;  // Low Byte
        s_ret_buf[s_ret_buf_size++] = tim_freq_reg[time].freq >> 8;    // High Byte
    } else {
        operation_result            = OPERATION_FAIL;
        s_ret_buf[s_ret_buf_size++] = operation_result;
    }
    chain_command_complete_return(SERVOS2_CHAIN_GET_TIME_CONFIG_SINGLE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get timer configuration for all timers.
 *        获取所有定时器的配置。
 *        Little Endian (Low Byte First)
 */
void servos2_chain_get_time_config_all(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = tim_freq_reg[0].freq & 0xFF;  // T0 Low
    s_ret_buf[s_ret_buf_size++] = tim_freq_reg[0].freq >> 8;    // T0 High
    s_ret_buf[s_ret_buf_size++] = tim_freq_reg[1].freq & 0xFF;  // T1 Low
    s_ret_buf[s_ret_buf_size++] = tim_freq_reg[1].freq >> 8;    // T1 High
    chain_command_complete_return(SERVOS2_CHAIN_GET_TIME_CONFIG_ALL, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get system voltage reference (VREF).
 *        获取系统参考电压 (VREF)。
 *        Little Endian (Low Byte First)
 */
void servos2_chain_get_vref(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = vref_mv_reg & 0xFF;
    s_ret_buf[s_ret_buf_size++] = vref_mv_reg >> 8;
    chain_command_complete_return(SERVOS2_CHAIN_GET_VREF, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get Grove port voltage.
 *        获取 Grove 接口电压。
 *        Little Endian (Low Byte First)
 */
void servos2_chain_get_grove_voltage(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = grove_voltage_mv_reg & 0xFF;
    s_ret_buf[s_ret_buf_size++] = grove_voltage_mv_reg >> 8;
    chain_command_complete_return(SERVOS2_CHAIN_GET_GROVE_VOLTAGE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get DC input voltage.
 *        获取 DC 输入电压。
 *        Little Endian (Low Byte First)
 */
void servos2_chain_get_dc_voltage(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = dc_voltage_mv_reg & 0xFF;
    s_ret_buf[s_ret_buf_size++] = dc_voltage_mv_reg >> 8;
    chain_command_complete_return(SERVOS2_CHAIN_GET_DC_VOLTAGE, s_ret_buf, s_ret_buf_size);
}

/**
 * @brief Get system current.
 *        获取系统电流。
 *        Little Endian (Low Byte First)
 */
void servos2_chain_get_sys_current(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = sys_current_ma_reg & 0xFF;
    s_ret_buf[s_ret_buf_size++] = sys_current_ma_reg >> 8;
    chain_command_complete_return(SERVOS2_CHAIN_GET_SYS_CURRENT, s_ret_buf, s_ret_buf_size);
}
