/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_sys.h"
#include "user_input.h"
#include "user_output.h"
#include "user_adc.h"
#include "user_pwm.h"
#include "user_servo.h"
#include "user_rgb.h"
#include "tim.h"
#include "servos2_chain_function.h"

// Global configuration array for GPIOs, initialized to 0
// GPIO 全局配置数组，初始化为 0
__IO user_sys_gpio_pin_config_t user_sys_gpio_pin_config[8] = {0};

/**
 * @brief Initialize the system hardware configuration map.
 *        初始化系统硬件配置映射。
 * @note  This function populates the `user_sys_gpio_pin_config` array.
 *        It maps logical GPIO indices (0-7) to physical Ports, Pins, Timers,
 *        PWM Channels, and ADC Channels.
 *        该函数填充 `user_sys_gpio_pin_config` 数组。
 *        它将逻辑 GPIO 索引 (0-7) 映射到物理端口、引脚、定时器、PWM 通道和 ADC 通道。
 *
 * @param None
 * @retval None
 */
void user_sys_config_init(void)
{
    // --- Group 1: Managed by TIM2 (GPIO 0-3) / 第一组：由 TIM2 管理 ---

    // GPIO0 Configuration
    user_sys_gpio_pin_config[0].port        = GPIO0_GPIO_Port;
    user_sys_gpio_pin_config[0].htim        = &htim2;
    user_sys_gpio_pin_config[0].pin         = GPIO0_Pin;
    user_sys_gpio_pin_config[0].alternate   = GPIO_AF2_TIM2;
    user_sys_gpio_pin_config[0].adc_channel = ADC_CHANNEL_0;
    user_sys_gpio_pin_config[0].pwm_channel = TIM_CHANNEL_1;

    // GPIO1 Configuration
    user_sys_gpio_pin_config[1].port        = GPIO1_GPIO_Port;
    user_sys_gpio_pin_config[1].htim        = &htim2;
    user_sys_gpio_pin_config[1].pin         = GPIO1_Pin;
    user_sys_gpio_pin_config[1].alternate   = GPIO_AF2_TIM2;
    user_sys_gpio_pin_config[1].adc_channel = ADC_CHANNEL_1;
    user_sys_gpio_pin_config[1].pwm_channel = TIM_CHANNEL_2;

    // GPIO2 Configuration
    user_sys_gpio_pin_config[2].port        = GPIO2_GPIO_Port;
    user_sys_gpio_pin_config[2].htim        = &htim2;
    user_sys_gpio_pin_config[2].pin         = GPIO2_Pin;
    user_sys_gpio_pin_config[2].alternate   = GPIO_AF2_TIM2;
    user_sys_gpio_pin_config[2].adc_channel = ADC_CHANNEL_2;
    user_sys_gpio_pin_config[2].pwm_channel = TIM_CHANNEL_3;

    // GPIO3 Configuration
    user_sys_gpio_pin_config[3].port        = GPIO3_GPIO_Port;
    user_sys_gpio_pin_config[3].htim        = &htim2;
    user_sys_gpio_pin_config[3].pin         = GPIO3_Pin;
    user_sys_gpio_pin_config[3].alternate   = GPIO_AF2_TIM2;
    user_sys_gpio_pin_config[3].adc_channel = ADC_CHANNEL_3;
    user_sys_gpio_pin_config[3].pwm_channel = TIM_CHANNEL_4;

    // --- Group 2: Managed by TIM3 (GPIO 4-7) / 第二组：由 TIM3 管理 ---

    // GPIO4 Configuration
    user_sys_gpio_pin_config[4].port        = GPIO4_GPIO_Port;
    user_sys_gpio_pin_config[4].htim        = &htim3;
    user_sys_gpio_pin_config[4].pin         = GPIO4_Pin;
    user_sys_gpio_pin_config[4].alternate   = GPIO_AF1_TIM3;
    user_sys_gpio_pin_config[4].adc_channel = ADC_CHANNEL_6;
    user_sys_gpio_pin_config[4].pwm_channel = TIM_CHANNEL_1;

    // GPIO5 Configuration
    user_sys_gpio_pin_config[5].port        = GPIO5_GPIO_Port;
    user_sys_gpio_pin_config[5].htim        = &htim3;
    user_sys_gpio_pin_config[5].pin         = GPIO5_Pin;
    user_sys_gpio_pin_config[5].alternate   = GPIO_AF1_TIM3;
    user_sys_gpio_pin_config[5].adc_channel = ADC_CHANNEL_7;
    user_sys_gpio_pin_config[5].pwm_channel = TIM_CHANNEL_2;

    // GPIO6 Configuration
    user_sys_gpio_pin_config[6].port        = GPIO6_GPIO_Port;
    user_sys_gpio_pin_config[6].htim        = &htim3;
    user_sys_gpio_pin_config[6].pin         = GPIO6_Pin;
    user_sys_gpio_pin_config[6].alternate   = GPIO_AF1_TIM3;
    user_sys_gpio_pin_config[6].adc_channel = ADC_CHANNEL_8;
    user_sys_gpio_pin_config[6].pwm_channel = TIM_CHANNEL_3;

    // GPIO7 Configuration
    user_sys_gpio_pin_config[7].port        = GPIO7_GPIO_Port;
    user_sys_gpio_pin_config[7].htim        = &htim3;
    user_sys_gpio_pin_config[7].pin         = GPIO7_Pin;
    user_sys_gpio_pin_config[7].alternate   = GPIO_AF1_TIM3;
    user_sys_gpio_pin_config[7].adc_channel = ADC_CHANNEL_9;
    user_sys_gpio_pin_config[7].pwm_channel = TIM_CHANNEL_4;

    // Initialize Timer Frequency Registers / 初始化定时器频率寄存器
    // reg[0] corresponds to TIM2, reg[1] corresponds to TIM3 / reg[0] 对应 TIM2, reg[1] 对应 TIM3
    tim_freq_reg[0].freq      = SERVO_FREQ_DEFAULT;
    tim_freq_reg[1].freq      = SERVO_FREQ_DEFAULT;
    tim_freq_reg[0].buffer[0] = SERVO_FREQ_DEFAULT;
    tim_freq_reg[1].buffer[0] = SERVO_FREQ_DEFAULT;
}

/**
 * @brief Update GPIO hardware modes based on change flags.
 *        根据变化标志更新 GPIO 硬件模式。
 * @note  This function checks if any GPIO mode has changed. If so, it disables
 *        conflicting features (like PWM) and initializes the new mode (Input, Output, ADC, etc.).
 *        For Servo mode, it ensures the timer frequency is strictly 50Hz to protect the servo.
 *        该函数检查是否有 GPIO 模式发生变化。如果有，它会禁用冲突的功能（如 PWM）
 *        并初始化新模式（输入、输出、ADC 等）。
 *        对于舵机模式，它确保定时器频率严格为 50Hz 以保护舵机。
 *
 * @param None
 * @retval None
 */
void user_sys_gpio_mode_update(uint8_t gpio)
{
    // Check if the GPIO index is valid / 检查 GPIO 索引是否有效
    if (gpio >= GPIO_NUM_MAX) {
        return;
    }

    switch (gpio_mode_reg[gpio]) {
        case USER_GPIO_INPUT_MODE:
            // Stop PWM before switching to Input / 切换到输入前停止 PWM
            HAL_TIM_PWM_Stop(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel);
            user_input_init(gpio);
            break;

        case USER_GPIO_OUTPUT_MODE:
            // Stop PWM before switching to Output / 切换到输出前停止 PWM
            HAL_TIM_PWM_Stop(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel);
            user_output_init(gpio);
            break;

        case USER_GPIO_ADC_MODE:
            // Stop PWM before switching to ADC / 切换到 ADC 前停止 PWM
            HAL_TIM_PWM_Stop(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel);
            user_adc_init(gpio);
            break;

        case USER_GPIO_SERVO_MODE:
            user_servo_init(gpio);
            // GPIO 0-3 use TIM2 (tim_freq_reg[0]) / GPIO 0-3 使用 TIM2
            if (gpio < 4) {
                // Ensure frequency is 50Hz for Servos / 确保舵机频率为 50Hz
                if (tim_freq_reg[0].freq == SERVO_FREQ_DEFAULT) {
                    user_servo_set_angle(gpio);
                    HAL_TIM_PWM_Start(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel);
                } else {
                    // Safety: Disable output if freq is not 50Hz / 安全：如果频率不是 50Hz，禁用输出
                    HAL_TIM_PWM_Stop(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel);
                }
            }
            // GPIO 4-7 use TIM3 (tim_freq_reg[1]) / GPIO 4-7 使用 TIM3
            else {
                if (tim_freq_reg[1].freq == SERVO_FREQ_DEFAULT) {
                    user_servo_set_angle(gpio);
                    HAL_TIM_PWM_Start(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel);
                } else {
                    HAL_TIM_PWM_Stop(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel);
                }
            }
            break;

        case USER_GPIO_RGB_MODE:
            // Stop PWM (Timer PWM) before RGB init (likely Bit-banging or SPI)
            // 在 RGB 初始化（可能是 Bit-bang 或 SPI）之前停止 PWM
            HAL_TIM_PWM_Stop(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel);
            user_rgb_init(gpio);
            break;

        case USER_GPIO_PWM_MODE:
            user_pwm_init(gpio);
            user_set_pwm_duty(gpio);
            HAL_TIM_PWM_Start(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel);
            break;

        default:
            break;
    }
}

void user_sys_grove_led_update(void)
{
    if (grove_voltage_mv_reg < GROVE_LOW_VOLTAGE_MV) {
        HAL_GPIO_WritePin(GPIO_LED1_GPIO_Port, GPIO_LED1_Pin, GPIO_PIN_SET);  // 高电压时 LED1 亮
    } else {
        HAL_GPIO_WritePin(GPIO_LED1_GPIO_Port, GPIO_LED1_Pin, GPIO_PIN_RESET);  // 低电压时 LED1 灭
    }
}

void user_sys_dc_led_update(void)
{
    if (dc_voltage_mv_reg > DC_HIGH_VOLTAGE_MV) {
        HAL_GPIO_WritePin(GPIO_LED2_GPIO_Port, GPIO_LED2_Pin, GPIO_PIN_SET);  // 高电压时 LED2 亮
    } else {
        HAL_GPIO_WritePin(GPIO_LED2_GPIO_Port, GPIO_LED2_Pin, GPIO_PIN_RESET);  // 低电压时 LED2 灭
    }
}
