/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_output.h"
#include "user_sys.h"
#include "servos2_chain_function.h"

/**
 * @brief Initialize GPIO pin as Push-Pull Output.
 *        将 GPIO 引脚初始化为推挽输出。
 *
 * @param gpio Index of the GPIO to initialize.
 *             要初始化的 GPIO 索引。
 * @retval None
 */
void user_output_init(uint8_t gpio)
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. Set the initial level BEFORE configuring as output to prevent glitches
    // 1. 在配置为输出之前设置初始电平，以防止毛刺
    // The HAL_GPIO_WritePin here sets the ODR (Output Data Register) or BSRR
    // 这里的 HAL_GPIO_WritePin 设置 ODR (输出数据寄存器) 或 BSRR
    HAL_GPIO_WritePin(user_sys_gpio_pin_config[gpio].port, user_sys_gpio_pin_config[gpio].pin,
                      (GPIO_PinState)gpio_gpio_output_status_reg[gpio]);

    // 2. Configure the Pin
    // 2. 配置引脚
    GPIO_InitStruct.Pin   = user_sys_gpio_pin_config[gpio].pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  // Push-Pull / 推挽
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // High speed for sharp edges / 高速以获得陡峭边沿

    // Note: GPIO Port Clock must be enabled externally
    // 注意：GPIO 端口时钟必须在外部启用
    HAL_GPIO_Init(user_sys_gpio_pin_config[gpio].port, &GPIO_InitStruct);
}

/**
 * @brief Set the logic level of the output pin.
 *        设置输出引脚的逻辑电平。
 *
 * @param gpio Index of the GPIO.
 *             GPIO 索引。
 * @retval None
 */
void user_output_set_level(uint8_t gpio)
{
    // Retrieve Port and Pin from configuration table
    // 从配置表中检索端口和引脚
    GPIO_TypeDef *port = user_sys_gpio_pin_config[gpio].port;
    uint16_t pin       = (uint16_t)user_sys_gpio_pin_config[gpio].pin;

    // Check the output status register and set/reset pin atomically
    // 检查输出状态寄存器并原子地 置位/复位 引脚
    // BSRR (Bit Set/Reset Register): Writing to lower 16 bits sets the pin HIGH
    // BRR  (Bit Reset Register)    : Writing to lower 16 bits sets the pin LOW
    // BSRR (位置位/复位寄存器)：写入低 16 位将引脚置为高电平
    // BRR  (位复位寄存器)     ：写入低 16 位将引脚置为低电平
    if (gpio_gpio_output_status_reg[gpio]) {
        port->BSRR = pin;  // Set High / 置高
    } else {
        port->BRR = pin;  // Set Low  / 置低
    }
}
