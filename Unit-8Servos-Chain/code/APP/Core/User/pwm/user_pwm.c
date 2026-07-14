/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_pwm.h"
#include "user_sys.h"
#include "servos2_chain_function.h"

/**
 * @brief Initialize GPIO hardware for PWM function.
 *        初始化 PWM 功能的 GPIO 硬件。
 *
 * @param gpio Index of the GPIO.
 *             GPIO 索引。
 * @retval None
 */
void user_pwm_init(uint8_t gpio)
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

    // Initialize the GPIO (Port clock must be enabled previously)
    // 初始化 GPIO (端口时钟必须预先启用)
    HAL_GPIO_Init(user_sys_gpio_pin_config[gpio].port, &GPIO_InitStruct);
}

/**
 * @brief Set the duty cycle for a specific PWM channel.
 *        设置特定 PWM 通道的占空比。
 *
 * @param gpio Index of the GPIO.
 *             GPIO 索引。
 * @retval None
 */
void user_set_pwm_duty(uint8_t gpio)
{
    // 1. Get current Auto-Reload value (ARR)
    // 1. 获取当前的自动重装载值 (ARR)
    // Critical: Since frequency is dynamic, ARR varies. We must read the register in real-time.
    // 关键点：因为频率是动态的，ARR 会变化。我们必须实时读取寄存器。
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(user_sys_gpio_pin_config[gpio].htim);

    // Get duty cycle and clamp to 0-100%
    // 获取占空比并限制在 0-100%
    float duty = (float)gpio_pwm_duty_reg[gpio];
    if (duty > 100.0f) duty = 100.0f;
    if (duty < 0.0f) duty = 0.0f;

    // 2. Calculate Capture Compare Value (CCR)
    // 2. 计算比较值 (CCR)
    // Logic: CCR = Total Ticks * (Percentage / 100)
    // (arr + 1) handles the 0-based counting (e.g., ARR=99 means 100 steps).
    // 逻辑：CCR = 总周期 ticks * (百分比 / 100)
    // (arr + 1) 处理从 0 开始的计数（例如 ARR=99 意味着 100 个刻度）。
    uint32_t ccr = (uint32_t)((double)(arr + 1) * (duty / 100.0f));

    // 3. Write to Capture/Compare Register
    // 3. 写入捕获/比较寄存器
    // Use macro for direct register access (highest efficiency)
    // 使用宏直接操作寄存器（效率最高）
    __HAL_TIM_SET_COMPARE(user_sys_gpio_pin_config[gpio].htim, user_sys_gpio_pin_config[gpio].pwm_channel, ccr);
}

/**
 * @brief System loop to update PWM outputs.
 *        用于更新 PWM 输出的系统循环。
 *
 * @param None
 * @retval None
 */
void user_sys_pwm_update(void)
{
    // Check global dirty flag to avoid unnecessary loops
    // 检查全局脏标志以避免不必要的循环
    if (!gpio_pwm_duty_changed_any) {
        return;
    }

    gpio_pwm_duty_changed_any = 0;

    for (uint8_t i = 0; i < 8; i++) {
        // Check individual channel flag
        // 检查单独通道的标志
        if (gpio_pwm_duty_changed[i]) {
            gpio_pwm_duty_changed[i] = 0;

            // Only update if the pin is actually in PWM mode
            // 仅当引脚实际处于 PWM 模式时更新
            if (gpio_mode_reg[i] == USER_GPIO_PWM_MODE) {
                user_set_pwm_duty(i);
                // Ensure PWM is running (Start is idempotent, safe to call repeatedly)
                // 确保 PWM 正在运行（Start 是幂等的，重复调用是安全的）
                HAL_TIM_PWM_Start(user_sys_gpio_pin_config[i].htim, user_sys_gpio_pin_config[i].pwm_channel);
            }
        }
    }
}
