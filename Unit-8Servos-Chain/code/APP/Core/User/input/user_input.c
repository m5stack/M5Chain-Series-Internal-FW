/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_input.h"
#include "user_sys.h"
#include "servos2_chain_function.h"

/**
 * @brief Initialize GPIO pin as Input with configured Pull resistor.
 *        将 GPIO 引脚初始化为带有配置电阻的输入模式。
 *
 * @param gpio Index of the GPIO to initialize.
 *             要初始化的 GPIO 索引。
 * @retval None
 */
void user_input_init(uint8_t gpio)
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure Pin settings / 配置引脚设置
    GPIO_InitStruct.Pin  = user_sys_gpio_pin_config[gpio].pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;

    // Set Pull-up/Pull-down/No-Pull based on register value
    // 0: No Pull, 1: Pull Up, 2: Pull Down
    // 根据寄存器值设置 上拉/下拉/无上下拉
    // 0: 无上下拉, 1: 上拉, 2: 下拉
    GPIO_InitStruct.Pull = gpio_gpio_input_pu_pd_reg[gpio];

    // Initialize the GPIO (Port clock must be enabled externally)
    // 初始化 GPIO (端口时钟必须在外部启用)
    HAL_GPIO_Init(user_sys_gpio_pin_config[gpio].port, &GPIO_InitStruct);
}

/**
 * @brief Read the hardware level of the input pin.
 *        读取输入引脚的硬件电平。
 *
 * @param gpio Index of the GPIO.
 *             GPIO 索引。
 * @retval User defined enum for GPIO Level (High/Low).
 *         用户定义的 GPIO 电平枚举 (高/低)。
 */
user_sys_gpio_level_t user_input_get_level(uint8_t gpio)
{
    // Access the Input Data Register (IDR) directly
    // Logic: Mask the IDR with the Pin bit. If result is non-zero, it's HIGH.
    // 直接访问输入数据寄存器 (IDR)
    // 逻辑：将 IDR 与引脚位进行掩码操作。如果结果非零，则为高电平。
    if (user_sys_gpio_pin_config[gpio].port->IDR & user_sys_gpio_pin_config[gpio].pin) {
        return USER_GPIO_LEVEL_HIGH;
    } else {
        return USER_GPIO_LEVEL_LOW;
    }
}