/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_INPUT_H__
#define __USER_INPUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "user_sys.h"

/**
 * @brief Initialize the GPIO for Input mode.
 *        初始化 GPIO 为输入模式。
 * @note  Configures the pin as input and sets the Pull-up/Pull-down resistor
 *        based on the register setting.
 *        配置引脚为输入，并根据寄存器设置配置上拉/下拉电阻。
 *
 * @param gpio GPIO index (0-7).
 *             GPIO 索引 (0-7)。
 */
void user_input_init(uint8_t gpio);

/**
 * @brief Get the current digital level of the input pin.
 *        获取输入引脚的当前数字电平。
 * @note  Reads the IDR (Input Data Register) directly for efficiency.
 *        直接读取 IDR (输入数据寄存器) 以提高效率。
 *
 * @param gpio GPIO index (0-7).
 *             GPIO 索引 (0-7)。
 * @retval USER_GPIO_LEVEL_HIGH or USER_GPIO_LEVEL_LOW.
 *         USER_GPIO_LEVEL_HIGH 或 USER_GPIO_LEVEL_LOW。
 */
user_sys_gpio_level_t user_input_get_level(uint8_t gpio);

#ifdef __cplusplus
}
#endif

#endif /* __USER_INPUT_H__ */
