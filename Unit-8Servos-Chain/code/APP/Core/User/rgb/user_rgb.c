/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_rgb.h"
#include "user_sys.h"
#include "servos2_chain_function.h"

// Global variables for port/pin to optimize access speed in the bit-banging loop
// 全局变量 port/pin，用于优化位操作循环中的访问速度
GPIO_TypeDef *port;
uint16_t pin;

// Internal buffer to store processed color data (e.g., GRB format)
// 用于存储处理后的颜色数据（例如 GRB 格式）的内部缓冲区
static uint32_t rgb_color_buf[16] = {0};

/**
 * @brief Initialize the GPIO pin for WS2812 data output.
 *        初始化用于 WS2812 数据输出的 GPIO 引脚。
 * @note  Configures the pin as Output Push-Pull with High Speed.
 *        Does NOT use interrupts or DMA, relies on CPU bit-banging.
 *        将引脚配置为高速推挽输出。
 *        不使用中断或 DMA，依赖 CPU 位操作。
 *
 * @param gpio The index of the GPIO in the system configuration.
 *             系统配置中的 GPIO 索引。
 */
void user_rgb_init(uint8_t gpio)
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Ensure the pin is low initially / 确保引脚初始为低电平
    HAL_GPIO_WritePin(user_sys_gpio_pin_config[gpio].port, user_sys_gpio_pin_config[gpio].pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin  = user_sys_gpio_pin_config[gpio].pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_HIGH;  // High speed is critical for sharp edges / 高速对于陡峭的边沿至关重要
    HAL_GPIO_Init(user_sys_gpio_pin_config[gpio].port, &GPIO_InitStruct);
}

/**
 * @brief Send a single 24-bit color value to the LED using bit-banging.
 *        使用位操作向 LED 发送单个 24 位颜色值。
 * @note  This function disables interrupts internally (in caller) to ensure strict timing.
 *        Uses direct register access (BSRR/BRR) for speed.
 *        此函数在内部（调用者处）禁用中断以确保严格的时序。
 *        使用直接寄存器访问 (BSRR/BRR) 以提高速度。
 *
 * @param color 24-bit color data (typically GRB format for WS2812).
 *              24 位颜色数据（WS2812 通常为 GRB 格式）。
 */
void user_rgb_send_data(uint32_t color)
{
    for (uint8_t i = 0; i < 24; i++)  // Loop through each bit of the 24-bit color value / 遍历 24 位颜色值的每一位
    {
        if (color & (1 << (23 - i)))  // Check if the current bit is set (1) / 检查当前位是否置位 (1)
        {
            // Send '1' Code: Long High, Short Low
            // 发送 '1' 码：长高电平，短低电平
            port->BSRR = pin;  // Set Pin High
            delay_600ns();
            __asm("NOP");
            __asm("NOP");
            __asm("NOP");

            port->BRR = pin;  // Set Pin Low
            delay_300ns();
            delay_150ns();
        } else {
            // Send '0' Code: Short High, Long Low
            // 发送 '0' 码：短高电平，长低电平
            port->BSRR = pin;  // Set Pin High
            delay_300ns();

            port->BRR = pin;  // Set Pin Low
            delay_900ns();
        }
    }
}

/**
 * @brief Update the internal color buffer from I2C registers.
 *        从 I2C 寄存器更新内部颜色缓冲区。
 * @note  Performs byte swapping if necessary (RGB <-> GRB or Endianness).
 *        如果需要，执行字节交换（RGB <-> GRB 或大小端转换）。
 */
void user_rgb_buffer_update(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    rgb_color_buf[index] = (uint32_t)(g << 16 | r << 8 | b);
}

/**
 * @brief Get the color component value from the internal buffer.
 *        从内部缓冲区获取颜色分量值。
 * @param index Index of the RGB LED.
 *              RGB LED 索引。
 * @param component Component index (0: R, 1: G, 2: B).
 *                  颜色分量索引 (0: R, 1: G, 2: B)。
 * @return uint8_t Color component value (0-255).
 *                 颜色分量值 (0-255)。
 */
uint8_t user_rgb_buffer_get(uint8_t index, uint8_t component)
{
    if (index < 16 && component < 3) {
        // 0(R)->8, 1(G)->16, 2(B)->0
        uint8_t shift = ((component + 1) % 3) * 8;
        return (uint8_t)(rgb_color_buf[index] >> shift & 0xFF);
    }
    return 0;
}

/**
 * @brief Main update loop for RGB LEDs.
 *        RGB LED 的主更新循环。
 * @note  Checks if a refresh is requested. If so, disables interrupts,
 *        sends the data stream, and re-enables interrupts.
 *        检查是否请求刷新。如果是，禁用中断，发送数据流，然后重新启用中断。
 */
void user_sys_rgb_update(uint8_t gpio)
{
    // Check if GPIO is in RGB Mode / 检查 GPIO 是否处于 RGB 模式
    if (gpio_mode_reg[gpio] == USER_GPIO_RGB_MODE) {
        // Check flags: Refresh requested AND number of LEDs > 0
        // 检查标志：请求刷新 且 LED 数量 > 0
        if (gpio_rgb_reg[gpio].rgb_config.flags.refresh && gpio_rgb_reg[gpio].rgb_config.flags.num) {
            // Set global port/pin for the bit-banging function
            // 为位操作函数设置全局端口/引脚
            port = user_sys_gpio_pin_config[gpio].port;
            pin  = (uint16_t)user_sys_gpio_pin_config[gpio].pin;

            // Disable interrupts to prevent timing glitches during transmission
            // 禁用中断以防止传输过程中的时序毛刺
            __disable_irq();

            // Send data for each LED / 为每个 LED 发送数据
            for (uint8_t j = 0; j < gpio_rgb_reg[gpio].rgb_config.flags.num; j++) {
                user_rgb_send_data(rgb_color_buf[j]);
            }

            // Re-enable interrupts / 重新启用中断
            __enable_irq();
        }
    }
    // Clear refresh flag / 清除刷新标志
    gpio_rgb_reg[gpio].rgb_config.flags.refresh = 0;
}
