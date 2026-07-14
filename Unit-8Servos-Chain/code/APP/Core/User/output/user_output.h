/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_OUTPUT_H__
#define __USER_OUTPUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Initialize the GPIO for digital output mode.
 *        初始化 GPIO 为数字输出模式。
 * @note  Sets the initial level based on the current register value before configuring mode
 *        to avoid glitches.
 *        在配置模式之前根据当前寄存器值设置初始电平，以避免毛刺。
 *
 * @param gpio GPIO index.
 *             GPIO 索引。
 */
void user_output_init(uint8_t gpio);

/**
 * @brief Set the digital output level of a GPIO.
 *        设置 GPIO 的数字输出电平。
 * @note  Uses direct register access (BSRR/BRR) for atomic operation and speed.
 *        使用直接寄存器访问 (BSRR/BRR) 以实现原子操作和高速度。
 *
 * @param gpio GPIO index.
 *             GPIO 索引。
 */
void user_output_set_level(uint8_t gpio);

#ifdef __cplusplus
}
#endif

#endif /* __USER_OUTPUT_H__ */