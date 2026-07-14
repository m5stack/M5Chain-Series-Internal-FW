/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_adc.h"
#include "user_sys.h"
#include "adc.h"
#include "servos2_chain_function.h"

/**
 * @brief Get the average ADC value of a specific channel.
 *        获取指定通道的 ADC 平均值。
 * @note  This function configures the ADC channel, performs multiple samplings (20 times),
 *        and calculates the average value to reduce noise. It handles channel configuration
 *        and timeout management.
 *        该函数配置 ADC 通道，执行多次采样（20次），并计算平均值以减少噪声。
 *        它处理了通道配置和超时管理。
 *
 * @param adc_channel The ADC channel to sample (e.g., ADC_CHANNEL_5).
 *                    需要采样的 ADC 通道 (例如 ADC_CHANNEL_5)。
 * @retval Average ADC value or SYS_ADC_GET_ERROR_VALUE on failure.
 *         ADC 平均值，或在失败时返回 SYS_ADC_GET_ERROR_VALUE。
 */
uint16_t user_adc_get_adc_value(uint32_t adc_channel)
{
    uint8_t valid_count            = 0;  // Record the number of successful collections / 记录成功采集的次数
    uint32_t sum                   = 0;  // Accumulator for the samples / 样本累加器
    ADC_ChannelConfTypeDef sConfig = {0};

    // Clear the Channel Selection Register (Critical for STM32G0/C0 series to reset sequence)
    // 清除通道选择寄存器（对于 STM32G0/C0 系列重置序列至关重要）
    hadc1.Instance->CHSELR = 0;

    sConfig.Channel      = adc_channel;
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_79CYCLES_5;

    // Configure the ADC channel. If configuration fails, return error immediately.
    // 配置 ADC 通道。如果配置失败，直接返回错误。
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return SYS_ADC_GET_ERROR_VALUE;
    }

    // Loop to sample 20 times / 循环采集 20 次
    for (uint8_t i = 0; i < 20; i++) {
        HAL_ADC_Start(&hadc1);

        // Wait for conversion completion, timeout set to 2ms (sufficient for single conversion)
        // 等待转换完成，超时设为 2ms (足够单次转换了)
        if (HAL_ADC_PollForConversion(&hadc1, 2) == HAL_OK) {
            sum += HAL_ADC_GetValue(&hadc1);
            valid_count++;
        } else {
            // If timeout occurs, stop to reset state and skip this sample (continue to next loop)
            // 如果发生超时，停止以复位状态并跳过此次采样（继续下一次循环）
            HAL_ADC_Stop(&hadc1);
        }
    }

    HAL_ADC_Stop(&hadc1);  // End sampling, stop ADC / 采集结束，停止 ADC

    // Calculate average only if there are enough successful samples
    // 只有当成功采集次数足够多才计算平均值
    if (valid_count > 0) {
        return (uint16_t)(sum / valid_count);  // The divisor is the actual successful count / 除数是实际成功的次数
    } else {
        return SYS_ADC_GET_ERROR_VALUE;  // All failed, return error / 全部失败，返回错误
    }
}

/**
 * @brief Initialize the ADC function for a specific GPIO.
 *        初始化指定 GPIO 的 ADC 功能。
 * @note  This function configures the specified pin as an analog input.
 *        Note: Ensure the GPIO Port Clock is enabled before calling this.
 *        该函数将指定的引脚配置为模拟输入。
 *        注意：调用此函数前请确保已开启 GPIO 端口时钟。
 *
 * @param gpio The index of the GPIO in the user configuration array.
 *             用户配置数组中的 GPIO 索引。
 * @retval None
 */
void user_adc_init(uint8_t gpio)
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin              = user_sys_gpio_pin_config[gpio].pin;
    GPIO_InitStruct.Mode             = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull             = GPIO_NOPULL;
    HAL_GPIO_Init(user_sys_gpio_pin_config[gpio].port, &GPIO_InitStruct);
}

/**
 * @brief Update system voltage, current, and user ADC channel values.
 *        更新系统电压、电流以及用户 ADC 通道的值。
 * @note  This function sequentially measures VREFINT, system voltage (CH5),
 *        system current (CH4), and configured user GPIO ADC channels.
 *        It calculates the actual voltage values using LL macros.
 *        该函数依次测量 VREFINT、系统电压 (CH5)、系统电流 (CH4) 和配置的用户 GPIO ADC 通道。
 *        它使用 LL 宏计算实际电压值。
 *
 * @param None
 * @retval None
 */
void user_adc_update(void)
{
    uint32_t temp = 0;  // Changed to uint32_t to match LL macro return type / 修改为 uint32_t 以匹配 LL 宏返回类型
    uint16_t adc_raw = 0;

    // 1. Reference Voltage / 参考电压
    adc_raw = user_adc_get_adc_value(ADC_CHANNEL_VREFINT);
    if (adc_raw != SYS_ADC_GET_ERROR_VALUE) {
        vref_mv_reg = __LL_ADC_CALC_VREFANALOG_VOLTAGE(adc_raw, LL_ADC_RESOLUTION_12B);
    }

    // 2. grove Voltage / grove电压
    adc_raw = user_adc_get_adc_value(ADC_CHANNEL_5);
    if (adc_raw != SYS_ADC_GET_ERROR_VALUE) {
        temp                 = __LL_ADC_CALC_DATA_TO_VOLTAGE(adc_raw, vref_mv_reg, LL_ADC_RESOLUTION_12B);
        grove_voltage_mv_reg = temp * 2;  // Voltage divider factor x2 / 分压系数 x2
    }

    // 3. DC Voltage / DC电压
    adc_raw = user_adc_get_adc_value(ADC_CHANNEL_16);
    if (adc_raw != SYS_ADC_GET_ERROR_VALUE) {
        temp              = __LL_ADC_CALC_DATA_TO_VOLTAGE(adc_raw, vref_mv_reg, LL_ADC_RESOLUTION_12B);
        dc_voltage_mv_reg = temp * 11;  // Voltage divider factor x11 / 分压系数 x11
    }

    // 4. System Current / 系统电流
    adc_raw = user_adc_get_adc_value(ADC_CHANNEL_4);
    if (adc_raw != SYS_ADC_GET_ERROR_VALUE) {
        temp               = __LL_ADC_CALC_DATA_TO_VOLTAGE(adc_raw, vref_mv_reg, LL_ADC_RESOLUTION_12B);
        sys_current_ma_reg = temp * 2;  // Current sense gain/shunt factor / 电流检测增益/分流系数
    }
}