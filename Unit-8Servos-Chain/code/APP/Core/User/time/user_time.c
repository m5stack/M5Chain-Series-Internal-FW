/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_time.h"
#include "user_sys.h"
#include "user_servo.h"
#include "user_pwm.h"
#include "tim.h"
#include "servos2_chain_function.h"

/**
 * @brief Set the frequency of a hardware timer.
 *        设置硬件定时器的频率。
 * @note  Calculates the optimal Prescaler (PSC) and Auto-Reload (ARR) values
 *        to achieve the target frequency with the highest possible resolution
 *        given a 64MHz system clock.
 *        计算最佳的预分频器 (PSC) 和自动重装载 (ARR) 值，
 *        以便在 64MHz 系统时钟下以尽可能高的分辨率实现目标频率。
 *
 * @param htim Pointer to the timer handle (e.g., &htim2).
 *             定时器句柄指针 (例如 &htim2)。
 * @param freq_hz Target frequency in Hz (1.0 - 65535.0).
 *                目标频率 Hz (1.0 - 65535.0)。
 * @retval None
 */
static void tim_set_frequency(TIM_HandleTypeDef *htim, double freq_hz)
{
    // Range protection / 范围保护
    if (freq_hz < 1.0) freq_hz = 1.0;
    if (freq_hz > 65535.0) freq_hz = 65535.0;

    uint32_t psc = 0;
    uint32_t arr = 0;

    // Calculate total ticks required under 64MHz clock
    // 计算 64MHz 时钟下的总计数值
    double total_ticks = SYSTEM_CLOCK_FREQ / freq_hz;

    // Core Algorithm: Strategy for 1Hz - 65535Hz
    // 核心算法：1Hz - 65535Hz 策略
    if (total_ticks < 65536.0) {
        // Mode A: High Frequency (> ~976Hz)
        // total_ticks fits in 16-bit ARR. No prescaler needed.
        // Direct 64MHz drive for nanosecond precision.
        // 模式 A: 频率较高 (> 976Hz)
        // 此时 total_ticks < 65535，完全不需要分频。
        // 我们直接用 64MHz 驱动计数器，获得纳秒级精度。
        psc = 0;
        arr = (uint32_t)(total_ticks - 1.0 + 0.5);  // +0.5 for rounding / +0.5 用于四舍五入
    } else {
        // Mode B: Low Frequency (1Hz - ~976Hz)
        // total_ticks exceeds 16-bit. Must use prescaler.
        // Calculate minimal PSC to make ARR fit into 65535.
        // 模式 B: 频率较低 (1Hz - 976Hz)
        // 此时 total_ticks > 65535，必须分频。
        // 计算最小的 PSC，使得 ARR 尽可能填满 65535。
        uint32_t psc_temp = (uint32_t)(total_ticks / 65536.0);
        psc               = psc_temp;
        arr               = (uint32_t)((total_ticks / (double)(psc + 1)) - 1.0 + 0.5);
    }

    // Apply settings to registers / 应用设置到寄存器
    __HAL_TIM_SET_PRESCALER(htim, psc);
    __HAL_TIM_SET_AUTORELOAD(htim, arr);

    // Reset counter to 0 to prevent glitches.
    // If the current CNT is greater than the new ARR, the timer would otherwise
    // count up to 0xFFFF (overflow) before wrapping around, causing a delay.
    // 将计数器复位为 0 以防止毛刺。
    // 如果当前 CNT 大于新的 ARR，定时器否则会向上计数直到 0xFFFF (溢出) 才会回绕，导致延迟。
    __HAL_TIM_SET_COUNTER(htim, 0);
}

/**
 * @brief Initialize timer frequencies to default values.
 *        将定时器频率初始化为默认值。
 */
void user_time_init(void)
{
    tim_set_frequency(&htim2, SYS_TIM_FREQ_DEFAULT);
    tim_set_frequency(&htim3, SYS_TIM_FREQ_DEFAULT);
}

/**
 * @brief Check and update timer frequencies and dependent peripherals.
 *        检查并更新定时器频率及相关外设。
 */
void user_time_freq_update(void)
{
    // Check if any frequency change flag is set
    // 检查是否有频率更改标志被置位
    if (!tim_freq_changed_any) {
        return;
    }

    tim_freq_changed_any = 0;

    // --- TIM2 Update (Controls GPIO 0-3) ---
    if (tim_freq_changed[0]) {
        tim_freq_changed[0] = 0;
        tim_set_frequency(&htim2, (double)tim_freq_reg[0].freq);

        // Update peripherals on Pins 0-3 / 更新引脚 0-3 上的外设
        for (int i = 0; i < 4; i++) {
            // Servo Logic: Only allow output if freq is default (50Hz)
            // 舵机逻辑：仅当频率为默认值 (50Hz) 时允许输出
            if (gpio_mode_reg[i] == USER_GPIO_SERVO_MODE) {
                if (tim_freq_reg[0].freq == SYS_TIM_FREQ_DEFAULT) {
                    user_servo_set_angle(i);
                    HAL_TIM_PWM_Start(user_sys_gpio_pin_config[i].htim, user_sys_gpio_pin_config[i].pwm_channel);
                } else {
                    HAL_TIM_PWM_Stop(user_sys_gpio_pin_config[i].htim, user_sys_gpio_pin_config[i].pwm_channel);
                }
            }
            // PWM Logic: Re-calculate duty cycle (CCR) based on new ARR
            // PWM 逻辑：基于新的 ARR 重新计算占空比 (CCR)
            if (gpio_mode_reg[i] == USER_GPIO_PWM_MODE) {
                user_set_pwm_duty(i);
                HAL_TIM_PWM_Start(user_sys_gpio_pin_config[i].htim, user_sys_gpio_pin_config[i].pwm_channel);
            }
        }
    }

    // --- TIM3 Update (Controls GPIO 4-7) ---
    if (tim_freq_changed[1]) {
        tim_freq_changed[1] = 0;
        tim_set_frequency(&htim3, (double)tim_freq_reg[1].freq);

        // Update peripherals on Pins 4-7 / 更新引脚 4-7 上的外设
        for (int i = 4; i < 8; i++) {
            // Servo Logic / 舵机逻辑
            if (gpio_mode_reg[i] == USER_GPIO_SERVO_MODE) {
                if (tim_freq_reg[1].freq == SYS_TIM_FREQ_DEFAULT) {
                    user_servo_set_angle(i);
                    HAL_TIM_PWM_Start(user_sys_gpio_pin_config[i].htim, user_sys_gpio_pin_config[i].pwm_channel);
                } else {
                    HAL_TIM_PWM_Stop(user_sys_gpio_pin_config[i].htim, user_sys_gpio_pin_config[i].pwm_channel);
                }
            }
            // PWM Logic / PWM 逻辑
            if (gpio_mode_reg[i] == USER_GPIO_PWM_MODE) {
                user_set_pwm_duty(i);
                HAL_TIM_PWM_Start(user_sys_gpio_pin_config[i].htim, user_sys_gpio_pin_config[i].pwm_channel);
            }
        }
    }
}
