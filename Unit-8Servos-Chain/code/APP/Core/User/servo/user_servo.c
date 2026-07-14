/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_servo.h"
#include "user_sys.h"
#include "servos2_chain_function.h"

/**
 * @brief Initialize GPIO pin for servo operation.
 *        初始化舵机操作的 GPIO 引脚。
 *
 * @param gpio Index of the GPIO.
 *             GPIO 索引。
 * @retval None
 */
void user_servo_init(uint8_t gpio)
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin              = user_sys_gpio_pin_config[gpio].pin;
    GPIO_InitStruct.Mode             = GPIO_MODE_AF_PP;  // Alternate Function Push-Pull / 复用推挽
    GPIO_InitStruct.Pull             = GPIO_NOPULL;
    GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate        = user_sys_gpio_pin_config[gpio].alternate;

    // Note: GPIO Port Clock must be enabled externally before this call
    // 注意：在此调用之前必须在外部启用 GPIO 端口时钟
    HAL_GPIO_Init(user_sys_gpio_pin_config[gpio].port, &GPIO_InitStruct);
}

/**
 * @brief Convert angle to PWM pulse width and update hardware.
 *        将角度转换为 PWM 脉宽并更新硬件。
 * @note  Standard Servo Timing:
 *        Frequency: 50Hz (Period: 20ms)
 *        0 degrees   -> 0.5ms High
 *        180 degrees -> 2.5ms High
 *        标准舵机时序：
 *        频率：50Hz (周期：20ms)
 *        0 度   -> 0.5ms 高电平
 *        180 度 -> 2.5ms 高电平
 *
 * @param gpio Index of the GPIO.
 *             GPIO 索引。
 * @retval None
 */
void user_servo_set_angle(uint8_t gpio)
{
    // 1. Get current Auto-Reload Register (ARR) value
    // 获取当前的自动重装载寄存器 (ARR) 值
    // At 50Hz & 64MHz clock, ARR is typically around 63999 theoretically.
    // 在 50Hz 和 64MHz 时钟下，ARR 理论值约为 63999。
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(user_sys_gpio_pin_config[gpio].htim);

    // Get angle and clamp it to 0-180 to prevent mechanical damage
    // 获取角度并将其限制在 0-180 范围内以防止机械损坏
    uint8_t angle = gpio_servo_angle_reg[gpio];
    if (angle > 180) angle = 180;

    // 2. Calculate Pulse Width in milliseconds
    // 计算脉宽（毫秒）
    // Formula: Pulse_ms = 0.5 + (angle / 180.0) * (Max_Width - Min_Width)
    double pulse_ms = 0.5 + ((double)angle / 180.0) * 2.0;

    // 3. Convert Pulse Width to Capture Compare Register (CCR) value
    // 将脉宽转换为捕获比较寄存器 (CCR) 值
    // Duty Cycle = Pulse_ms / Period_ms (20ms)
    // CCR = (ARR + 1) * Duty Cycle
    uint32_t ccr = (uint32_t)((double)(arr + 1) * (pulse_ms / 20.0));

    // 4. Write to the register
    // 写入寄存器
    __HAL_TIM_SET_COMPARE(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel, ccr);
}

/**
 * @brief Batch update routine for servo angles.
 *        舵机角度的批量更新例程。
 *
 * @param None
 * @retval None
 */
void user_sys_servo_angle_update(void)
{
    // Check global dirty flag
    // 检查全局脏标志
    if (!gpio_servo_angle_changed_any) {
        return;
    }

    gpio_servo_angle_changed_any = 0;

    // Iterate through all channels
    // 遍历所有通道
    for (uint8_t i = 0; i < 8; i++) {
        if (gpio_servo_angle_changed[i]) {
            gpio_servo_angle_changed[i] = 0;  // Clear individual flag / 清除单独标志

            // Only process if the pin is actually configured in SERVO mode
            // 仅当引脚实际配置为 SERVO 模式时才处理
            if (gpio_mode_reg[i] == USER_GPIO_SERVO_MODE) {
                // Determine which timer frequency register to check (0-3 -> TIM2/Reg0, 4-7 -> TIM3/Reg1)
                // 确定要检查哪个定时器频率寄存器
                uint8_t freq_reg_idx = i / 4;

                // Safety Check: Only enable output if frequency is strictly 50Hz
                // 安全检查：仅当频率严格为 50Hz 时才启用输出
                if (tim_freq_reg[freq_reg_idx].freq == SYS_TIM_FREQ_DEFAULT) {
                    user_servo_set_angle(i);
                    HAL_TIM_PWM_Start(user_sys_gpio_pin_config[i].htim, user_sys_gpio_pin_config[i].pwm_channel);
                } else {
                    // Wrong frequency for servo, stop PWM to prevent damage
                    // 舵机频率错误，停止 PWM 以防止损坏
                    HAL_TIM_PWM_Stop(user_sys_gpio_pin_config[i].htim, user_sys_gpio_pin_config[i].pwm_channel);
                }
            }
        }
    }
}
