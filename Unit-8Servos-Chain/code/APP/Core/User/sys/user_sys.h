/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_SYS_H__
#define __USER_SYS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* GPIO Pin Definitions / GPIO 引脚定义 */
#define GPIO0_Pin       (GPIO_PIN_0)
#define GPIO0_GPIO_Port (GPIOA)
#define GPIO1_Pin       (GPIO_PIN_1)
#define GPIO1_GPIO_Port (GPIOA)
#define GPIO2_Pin       (GPIO_PIN_2)
#define GPIO2_GPIO_Port (GPIOA)
#define GPIO3_Pin       (GPIO_PIN_3)
#define GPIO3_GPIO_Port (GPIOA)
#define GPIO4_Pin       (GPIO_PIN_6)
#define GPIO4_GPIO_Port (GPIOA)
#define GPIO5_Pin       (GPIO_PIN_7)
#define GPIO5_GPIO_Port (GPIOA)
#define GPIO6_Pin       (GPIO_PIN_0)
#define GPIO6_GPIO_Port (GPIOB)
#define GPIO7_Pin       (GPIO_PIN_1)
#define GPIO7_GPIO_Port (GPIOB)

#define GPIO_NUM_MAX            (8)           // Maximum number of GPIO pins / 最大 GPIO 引脚数量
#define GROVE_LOW_VOLTAGE_MV    (4500)        // Grove voltage low threshold in mV / Grove 电压低阈值 (mV)
#define DC_HIGH_VOLTAGE_MV      (8500)        // DC voltage high threshold in mV / DC 电压高阈值 (mV)
#define SYSTEM_CLOCK_FREQ       (64000000UL)  // System clock frequency in Hz / 系统时钟频率 (Hz)
#define SERVO_FREQ_DEFAULT      (50)          // Default servo frequency in Hz / 默认舵机频率 (Hz)
#define RGB_NUM_MAX             (16)          // Maximum number of RGB LEDs / 最大 RGB LED 数量
#define SYS_ADC_GET_ERROR_VALUE (0xFFFF)      // Error value for ADC readings / ADC 读取错误值
#define SYS_TIM_FREQ_DEFAULT    (50)          // Default timer frequency / 默认定时器频率
#define SYS_VREF_MV             (3300)        // System reference voltage in mV / 系统参考电压 (mV)
#define IAP_UPDATE_KEY          (0xA5)        // Key for IAP update / IAP 更新密钥
#define FIRMWARE_VERSION        (0x01)        // Firmware version identifier / 固件版本标识符
#define I2C_DEFAULT_ADDR        (0x25)        // Default I2C slave address / 默认 I2C 从机地址

/**
 * @brief Structure to define the hardware configuration for a system GPIO.
 * @note  Maps a logical pin index to physical ports, timers, ADC, and PWM channels.
 *        定义系统 GPIO 硬件配置的结构体。
 *        将逻辑引脚索引映射到物理端口、定时器、ADC 和 PWM 通道。
 */
typedef struct {
    GPIO_TypeDef* port;       // GPIO Port (e.g., GPIOA) / GPIO 端口
    TIM_HandleTypeDef* htim;  // Associated Timer Handle / 关联的定时器句柄
    uint32_t pin;             // GPIO Pin (e.g., GPIO_PIN_0) / GPIO 引脚
    uint32_t alternate;       // Alternate function selection / 复用功能选择
    uint32_t adc_channel;     // ADC Channel (if applicable) / ADC 通道（如果适用）
    uint32_t pwm_channel;     // PWM Channel (TIM channel) / PWM 通道（定时器通道）
} user_sys_gpio_pin_config_t;

/**
 * @brief Structure for system timing configuration.
 *        系统时间配置结构体。
 */
typedef struct {
    uint16_t freq;      // Frequency setting / 频率设置
    uint8_t buffer[2];  // Data buffer / 数据缓冲区
} user_sys_time_config_t;

/**
 * @brief Structure for RGB LED configuration using a union for bit-field access.
 *        使用联合体进行位域访问的 RGB LED 配置结构体。
 */
typedef struct {
    union {
        struct {
            uint8_t num : 5;      // Number of LEDs (0-31) / LED 数量 (0-31)
            uint8_t refresh : 1;  // Refresh flag / 刷新标志
            uint8_t rsvd : 2;     // Reserved / 保留位
        } flags;
        uint8_t value;  // Raw byte access / 原始字节访问
    } rgb_config;
} rgb_config_t;

/**
 * @brief Enumeration of supported GPIO operation modes.
 *        支持的 GPIO 操作模式枚举。
 */
typedef enum {
    USER_GPIO_INPUT_MODE  = 0x00,  // Input mode / 输入模式
    USER_GPIO_OUTPUT_MODE = 0x01,  // Output mode / 输出模式
    USER_GPIO_ADC_MODE    = 0x02,  // Analog/ADC mode / 模拟/ADC 模式
    USER_GPIO_SERVO_MODE  = 0x03,  // Servo control mode / 舵机控制模式
    USER_GPIO_RGB_MODE    = 0x04,  // RGB LED control mode / RGB LED 控制模式
    USER_GPIO_PWM_MODE    = 0x05,  // PWM output mode / PWM 输出模式
} user_gpio_mode_t;

/**
 * @brief Enumeration of GPIO pull resistor configurations.
 *        GPIO 上拉/下拉电阻配置枚举。
 */
typedef enum {
    USER_GPIO_PULL_NONE = 0x00,  // No pull-up/pull-down / 无上下拉
    USER_GPIO_PULL_UP   = 0x01,  // Pull-up resistor enabled / 启用上拉电阻
    USER_GPIO_PULL_DOWN = 0x02,  // Pull-down resistor enabled / 启用下拉电阻
} user_gpio_pull_t;

/**
 * @brief Enumeration of GPIO output levels.
 *        GPIO 输出电平枚举。
 */
typedef enum {
    USER_GPIO_LEVEL_LOW  = 0x00,  // Low level / 低电平
    USER_GPIO_LEVEL_HIGH = 0x01,  // High level / 高电平
} user_sys_gpio_level_t;

/**
 * @brief Global array storing configuration for 8 user GPIOs.
 *        存储 8 个用户 GPIO 配置的全局数组。
 */
extern __IO user_sys_gpio_pin_config_t user_sys_gpio_pin_config[8];

/**
 * @brief Initialize the system configuration data structures.
 * @note  Sets up the default pin mappings and hardware parameters.
 *        初始化系统配置数据结构。
 *        设置默认的引脚映射和硬件参数。
 *
 * @param None
 * @retval None
 */
void user_sys_config_init(void);

/**
 * @brief Update the GPIO hardware modes based on current register settings.
 * @note  This function re-initializes GPIOs (Input, Output, ADC, PWM, etc.)
 *        according to the mode registers.
 *        根据当前寄存器设置更新 GPIO 硬件模式。
 *        该函数根据模式寄存器重新初始化 GPIO（输入、输出、ADC、PWM 等）。
 *
 * @param gpio Index of the GPIO to update.
 *             要更新的 GPIO 索引。
 * @retval None
 */
void user_sys_gpio_mode_update(uint8_t gpio);

/**
 * @brief Update the LED states based on the current Grove voltage.
 *        根据当前 Grove 电压更新 LED 状态。
 *
 * @param None
 * @retval None
 */
void user_sys_grove_led_update(void);

/**
 * @brief Update the LED states based on the current DC voltage.
 *        根据当前 DC 电压更新 LED 状态。
 *
 * @param None
 * @retval None
 */
void user_sys_dc_led_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_SYS_H__ */
