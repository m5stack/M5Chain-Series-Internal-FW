/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_PWM_H__
#define __USER_PWM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Initialize the GPIO for PWM output.
 *        初始化用于 PWM 输出的 GPIO。
 * @note  Configures the pin as Alternate Function Push-Pull.
 *        Note: The GPIO port clock must be enabled externally.
 *        配置引脚为复用推挽输出。
 *        注意：GPIO 端口时钟必须在外部启用。
 *
 * @param gpio GPIO index (0-7).
 *             GPIO 索引 (0-7)。
 */
void user_pwm_init(uint8_t gpio);

/**
 * @brief Calculate and set the PWM duty cycle.
 *        计算并设置 PWM 占空比。
 * @note  Reads the current timer Auto-Reload Register (ARR) to calculate
 *        the Capture Compare Register (CCR) value based on the requested percentage.
 *        读取当前定时器自动重装载寄存器 (ARR)，
 *        基于请求的百分比计算捕获比较寄存器 (CCR) 的值。
 *
 * @param gpio GPIO index (0-7).
 *             GPIO 索引 (0-7)。
 */
void user_set_pwm_duty(uint8_t gpio);

/**
 * @brief Update PWM settings based on register changes.
 *        根据寄存器更改更新 PWM 设置。
 * @note  Checks dirty flags and updates hardware if changes occurred.
 *        检查脏标志，如果发生更改则更新硬件。
 */
void user_sys_pwm_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_PWM_H__ */