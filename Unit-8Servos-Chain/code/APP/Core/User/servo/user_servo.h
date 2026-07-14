/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_SERVO_H__
#define __USER_SERVO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Initialize the GPIO for Servo PWM output.
 *        初始化用于舵机 PWM 输出的 GPIO。
 * @note  Configures the pin as Alternate Function Push-Pull.
 *        配置引脚为复用推挽输出模式。
 *
 * @param gpio The GPIO index to initialize.
 *             要初始化的 GPIO 索引。
 */
void user_servo_init(uint8_t gpio);

/**
 * @brief Calculate and set the PWM duty cycle for a specific angle.
 *        计算并设置特定角度的 PWM 占空比。
 * @note  Maps 0-180 degrees to 0.5ms-2.5ms pulse width within a 20ms period.
 *        将 0-180 度映射到 20ms 周期内的 0.5ms-2.5ms 脉宽。
 *
 * @param gpio The GPIO index to set.
 *             要设置的 GPIO 索引。
 */
void user_servo_set_angle(uint8_t gpio);

/**
 * @brief Update servo angles based on register changes.
 *        根据寄存器变化更新舵机角度。
 * @note  Checks if the angle registers have changed and updates the PWM pulse width.
 *        Ensures the timer frequency is 50Hz before enabling output.
 *        检查角度寄存器是否发生变化并更新 PWM 脉宽。
 *        在启用输出前确保定时器频率为 50Hz。
 */
void user_sys_servo_angle_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_SERVO_H__ */