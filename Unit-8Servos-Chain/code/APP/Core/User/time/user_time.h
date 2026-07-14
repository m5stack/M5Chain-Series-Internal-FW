/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_TIME_H__
#define __USER_TIME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Initialize the user timer settings.
 *        初始化用户定时器设置。
 * @note  Sets the default frequency for TIM2 and TIM3.
 *        为 TIM2 和 TIM3 设置默认频率。
 */
void user_time_init(void);

/**
 * @brief Update timer frequencies based on register changes.
 *        根据寄存器的变化更新定时器频率。
 * @note  This function checks if the frequency registers have changed.
 *        If changed, it recalculates the Prescaler (PSC) and Auto-Reload Register (ARR)
 *        values. It also updates the PWM duty cycles and handles Servo safety logic.
 *        该函数检查频率寄存器是否发生变化。
 *        如果发生变化，它会重新计算预分频器 (PSC) 和自动重装载寄存器 (ARR) 的值。
 *        同时它会更新 PWM 占空比并处理舵机安全逻辑。
 */
void user_time_freq_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_TIME_H__ */