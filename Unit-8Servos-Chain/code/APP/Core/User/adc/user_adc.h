/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_ADC_H__
#define __USER_ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Get the ADC value for the specified channel.
 *        获取指定通道的 ADC 值。
 * @note  Returns the average of multiple samples.
 *        返回多个样本的平均值。
 *
 * @param adc_channel ADC channel number (0-15).
 *                    ADC 通道号（0-15）。
 * @retval Average ADC value or SYS_ADC_GET_ERROR_VALUE on failure.
 *         ADC 平均值，或在失败时返回 SYS_ADC_GET_ERROR_VALUE。
 */
uint16_t user_adc_get_adc_value(uint32_t adc_channel);

/**
 * @brief Initialize the ADC GPIO pin configuration.
 *        初始化 ADC GPIO 引脚配置。
 * @note  Sets the specified GPIO to Analog mode.
 *        将指定的 GPIO 设置为模拟模式。
 *
 * @param gpio Index of the GPIO to initialize.
 *             要初始化的 GPIO 索引。
 * @retval None
 */
void user_adc_init(uint8_t gpio);

/**
 * @brief Perform a full update cycle of ADC measurements.
 *        执行 ADC 测量的完整更新周期。
 * @note  Updates VREF, System Voltage, System Current, and user GPIO ADCs.
 *        更新 VREF、系统电压、系统电流和用户 GPIO ADC。
 *
 * @param None
 * @retval None
 */
void user_adc_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_ADC_H__ */