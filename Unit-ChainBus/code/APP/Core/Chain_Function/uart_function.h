/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef UART_FUNCTION_H_
#define UART_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "base_function.h"
#include "i2c.h"
#include "adc.h"
#include <stdbool.h>

#define I2C_READ_MAX_SIZE (64)  // I2C读取的最大长度

#define DEBOUNCE_TIME_MS (50)

/**
 * @brief Command types for Chain_Uart communication.
 *
 * This enumeration defines command codes used for UART communication to perform various I2C,
 * GPIO, and system-level operations. Each command corresponds to a specific action or query within the system.
 */
typedef enum {
    CHAIN_I2C_INIT                  = 0x10, /**< Initialize I2C communication. */
    CHAIN_I2C_READ                  = 0x11, /**< Read data from I2C device. */
    CHAIN_I2C_WRITE                 = 0x12, /**< Write data to I2C device. */
    CHAIN_I2C_MEM_READ              = 0x13, /**< Read data from specific address on I2C device. */
    CHAIN_I2C_MEM_WRITE             = 0x14, /**< Write data to specific address on I2C device. */
    CHAIN_I2C_SCAN_ADDR             = 0x15, /**< Scan the I2C address of the connection. */
    CHAIN_GPIO_OUTPUT_INIT          = 0x30, /**< Initialize GPIO for output. */
    CHAIN_GPIO_SET_OUTPUT_LEVEL     = 0x31, /**< Set the level of output GPIO pin. */
    CHAIN_GPIO_GET_OUTPUT_LEVEL     = 0x32, /**< Get the level of output GPIO pin. */
    CHAIN_GPIO_INPUT_INIT           = 0x40, /**< Initialize GPIO for input. */
    CHAIN_GPIO_READ_GPIO_LEVEL      = 0x41, /**< Read the level of input GPIO pin. */
    CHAIN_GPIO_EXTERNAL_NVIC_INIT   = 0x50, /**< Initialize external interrupt for GPIO. */
    CHAIN_GPIO_ADC_INIT             = 0x60, /**< Initialize ADC for GPIO. */
    CHAIN_GPIO_ADC_READ             = 0x61, /**< Read data from GPIO ADC. */
    CHAIN_GET_WORK_STATION          = 0x70, /**< Query the working status of the system. */
    CHAIN_GPIO_EXTERNAL_NVIC_RETURN = 0xE0, /**< Return from external interrupt for GPIO. */
} uart_cmd_t;                               /**< Command type for UART communication. */

typedef enum {
    CHAIN_NOT_WORK_STATUS    = 0x00,  // Not configured working status
    CHAIN_OUTPUT_WORK_STATUS = 0x01,  // Output status
    CHAIN_INPUT_WORK_STATUS  = 0x02,  // Input status
    CHAIN_NVIC_WORK_STATUS   = 0x03,  // External interrupt working status
    CHAIN_ADC_WORK_STATUS    = 0x04,  // ADC working status
    CHAIN_I2C_WORK_STATUS    = 0x05   // I2C working status
} gpio_work_status_t;                 // GPIO pin working mode

typedef enum {
    CHAIN_GPIO_PIN_11 = 0x01,  // GPIO pin 11
    CHAIN_GPIO_PIN_12 = 0x02   // GPIO pin 12
} gpio_pin_t;                  // Enumeration for GPIO pin identifiers

typedef enum {
    CHAIN_GPIO_RESET = 0x00,  // Low level (reset)
    CHAIN_GPIO_SET   = 0x01   // High level (set)
} gpio_level_t;               // Enumeration for GPIO pin levels

typedef enum {
    CHAIN_GPIO_OUTPUT_PUSHPULL  = 0x00,  // Push-pull output mode
    CHAIN_GPIO_OUTPUT_OPENDRAIN = 0x01   // Open-drain output mode
} gpio_mode_t;                           // Enumeration for GPIO output modes

typedef enum {
    CHAIN_GPIO_PULL_UP   = 0x00,  // Pull-up resistor enabled
    CHAIN_GPIO_PULL_DOWN = 0x01,  // Pull-down resistor enabled
    CHAIN_GPIO_PULL_NO   = 0x02   // No pull-up or pull-down resistor
} gpio_up_mode_t;                 // Enumeration for external resistor configuration

typedef enum {
    CHAIN_GPIO_SPEED_FREQ_LOW       = 0x00,  // Low speed setting
    CHAIN_GPIO_SPEED_FREQ_MEDIUM    = 0x01,  // Medium speed setting
    CHAIN_GPIO_SPEED_FREQ_HIGH      = 0x02,  // High speed setting
    CHAIN_GPIO_SPEED_FREQ_VERY_HIGH = 0x03   // Very high speed setting
} gpio_speed_t;                              // Enumeration for GPIO speed settings

typedef enum {
    CHAIN_GPIO_MODE_IT_RISING         = 0x00,  // Interrupt on rising edge
    CHAIN_GPIO_MODE_IT_FALLING        = 0x01,  // Interrupt on falling edge
    CHAIN_GPIO_MODE_IT_RISING_FALLING = 0x02   // Interrupt on both rising and falling edges
} gpio_nvic_trigger_detection_t;               // Enumeration for external interrupt trigger sources

typedef enum {
    CHAIN_ADC_CHANNEL_DISABLE = 0x00,  // Disable the ADC channel (set to 0)
    CHAIN_ADC_CHANNEL_ENABLE  = 0x01,  // Enable the ADC channel (set to 1)
} adc_channel_t;                       // Type definition for ADC channel states

typedef enum {
    CHAIN_I2C_LOW_SPEED_100KHZ  = 0x00,  // Low speed (100 kHz)
    CHAIN_I2C_HIGH_SPEED_400KHZ = 0x01,  // Medium speed (400 kHz)
} i2c_speed_t;                           // Enumeration for I2C speed settings

typedef enum {
    CHAIN_I2C_READ_SUCCESS = 0x00,  // Read operation successful
    CHAIN_I2C_READ_FAIL    = 0x01   // Read operation failed
} i2c_read_status_t;                // Enumeration for I2C read status

typedef enum {
    CHAIN_I2C_WRITE_SUCCESS = 0x00,  // Write operation successful
    CHAIN_I2C_WRITE_FAIL    = 0x01   // Write operation failed
} i2c_write_status_t;                // Enumeration for I2C write status

typedef enum {
    CHAIN_UART_OPERATION_FAIL    = 0x00,  // Operation failed
    CHAIN_UART_OPERATION_SUCCESS = 0x01,  // Operation successful
    CHAIN_WORK_MODE_MISMATCHED   = 0x02,
} chain_uart_operation_t;  // Operation status

typedef struct {
    gpio_work_status_t mode;                                    // 工作模式
    uint16_t gpio_pin;                                          // gpio引脚
    i2c_speed_t i2c_speed;                                      // i2c通信速度，i2c，工作模式有效
    gpio_level_t gpio_level;                                    // gpio水平
    gpio_mode_t gpio_mode;                                      // gpio模式
    gpio_up_mode_t gpio_up_mode;                                // 上电阻状态
    gpio_nvic_trigger_detection_t gpio_nvic_trigger_detection;  // 外部中断触发方式
    adc_channel_t adc_channel;                                  // adc通道是否开启
} gpio_config_t;

extern __IO gpio_config_t g_gpio_status[2];
extern ADC_HandleTypeDef hadc1;

void chain_i2c_init(uint8_t speed);
void chain_i2c_read(uint8_t i2c_addr, uint8_t read_len);
void chain_i2c_write(uint8_t i2c_addr, uint8_t *buffer, uint16_t write_len);
void chain_i2c_mem_read(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_len, uint8_t read_len);
void chain_i2c_mem_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_len, uint8_t write_len, uint8_t *buffer);
void chain_i2c_scan_addr(void);
void chain_uart_output_init(uint8_t *buffer, uint16_t size);
void chain_uart_set_output_level(uint8_t gpio, uint8_t level);
void chain_uart_get_output_level(uint8_t gpio);
void chain_uart_input_init(uint8_t *buffer, uint16_t size);
void chain_uart_read_pin_level(uint8_t gpio);
void chain_uart_nvic_init(uint8_t *buffer, uint16_t size);
void chain_uart_adc_init(uint8_t adc_channel);
void chain_uart_adc_read(uint8_t adc_channel);
void chain_get_gpio_work_status(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_FUNCTION_H_ */
