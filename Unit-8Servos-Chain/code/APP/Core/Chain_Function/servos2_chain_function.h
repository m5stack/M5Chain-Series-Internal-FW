/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __SERVOS2_CHAIN_FUNCTION_H__
#define __SERVOS2_CHAIN_FUNCTION_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include "base_function.h"
#include "user_sys.h"

/**
 * @brief Command codes for Servos2 chain operations
 *        Servos2 链式操作的命令码定义
 */
typedef enum {
    SERVOS2_CHAIN_SET_MODE_SINGLE          = 0x10,  // Set single GPIO mode / 设置单个GPIO模式
    SERVOS2_CHAIN_SET_MODE_ALL             = 0x11,  // Set all GPIO modes / 设置所有GPIO模式
    SERVOS2_CHAIN_GET_MODE_SINGLE          = 0x12,  // Get single GPIO mode / 获取单个GPIO模式
    SERVOS2_CHAIN_GET_MODE_ALL             = 0x13,  // Get all GPIO modes / 获取所有GPIO模式
    SERVOS2_CHAIN_SET_INPUT_PU_PD_SINGLE   = 0x30,  // Set single input pull-up/down / 设置单个输入上下拉
    SERVOS2_CHAIN_SET_INPUT_PU_PD_ALL      = 0x31,  // Set all input pull-up/down / 设置所有输入上下拉
    SERVOS2_CHAIN_GET_INPUT_PU_PD_SINGLE   = 0x32,  // Get single input pull-up/down / 获取单个输入上下拉
    SERVOS2_CHAIN_GET_INPUT_PU_PD_ALL      = 0x33,  // Get all input pull-up/down / 获取所有输入上下拉
    SERVOS2_CHAIN_GET_INPUT_STATUS_SINGLE  = 0x34,  // Get single input status / 获取单个输入状态
    SERVOS2_CHAIN_GET_INPUT_STATUS_ALL     = 0x35,  // Get all input status / 获取所有输入状态
    SERVOS2_CHAIN_SET_OUTPUT_STATUS_SINGLE = 0x40,  // Set single output status / 设置单个输出状态
    SERVOS2_CHAIN_SET_OUTPUT_STATUS_ALL    = 0x41,  // Set all output status / 设置所有输出状态
    SERVOS2_CHAIN_GET_OUTPUT_STATUS_SINGLE = 0x42,  // Get single output status / 获取单个输出状态
    SERVOS2_CHAIN_GET_OUTPUT_STATUS_ALL    = 0x43,  // Get all output status / 获取所有输出状态
    SERVOS2_CHAIN_GET_ADC_VALUE_SINGLE     = 0x50,  // Get single ADC value / 获取单个ADC值
    SERVOS2_CHAIN_GET_ADC_VALUE_ALL        = 0x51,  // Get all ADC values / 获取所有ADC值
    SERVOS2_CHAIN_SET_SERVO_ANGLE_SINGLE   = 0x60,  // Set single servo angle / 设置单个舵机角度
    SERVOS2_CHAIN_SET_SERVO_ANGLE_ALL      = 0x61,  // Set all servo angles / 设置所有舵机角度
    SERVOS2_CHAIN_GET_SERVO_ANGLE_SINGLE   = 0x62,  // Get single servo angle / 获取单个舵机角度
    SERVOS2_CHAIN_GET_SERVO_ANGLE_ALL      = 0x63,  // Get all servo angles / 获取所有舵机角度
    SERVOS2_CHAIN_SET_RGB_CONFIG_SINGLE    = 0x70,  // Set single RGB config / 设置单个RGB配置
    SERVOS2_CHAIN_SET_RGB_CONFIG_ALL       = 0x71,  // Set all RGB config / 设置所有RGB配置
    SERVOS2_CHAIN_GET_RGB_CONFIG_SINGLE    = 0x72,  // Get single RGB config / 获取单个RGB配置
    SERVOS2_CHAIN_GET_RGB_CONFIG_ALL       = 0x73,  // Get all RGB config / 获取所有RGB配置
    SERVOS2_CHAIN_SET_RGB_BUFFER_SINGLE    = 0x74,  // Set single RGB buffer / 设置单个RGB缓冲
    SERVOS2_CHAIN_SET_RGB_BUFFER_ALL       = 0x75,  // Set all RGB buffer / 设置所有RGB缓冲
    SERVOS2_CHAIN_GET_RGB_BUFFER_SINGLE    = 0x76,  // Get single RGB buffer / 获取单个RGB缓冲
    SERVOS2_CHAIN_GET_RGB_BUFFER_ALL       = 0x77,  // Get all RGB buffer / 获取所有RGB缓冲
    SERVOS2_CHAIN_SET_PWM_DUTY_SINGLE      = 0x80,  // Set single PWM duty / 设置单个PWM占空比
    SERVOS2_CHAIN_SET_PWM_DUTY_ALL         = 0x81,  // Set all PWM duty / 设置所有PWM占空比
    SERVOS2_CHAIN_GET_PWM_DUTY_SINGLE      = 0x82,  // Get single PWM duty / 获取单个PWM占空比
    SERVOS2_CHAIN_GET_PWM_DUTY_ALL         = 0x83,  // Get all PWM duty / 获取所有PWM占空比
    SERVOS2_CHAIN_SET_TIME_CONFIG_SINGLE   = 0x90,  // Set single timer config / 设置单个定时器配置
    SERVOS2_CHAIN_SET_TIME_CONFIG_ALL      = 0x91,  // Set all timer config / 设置所有定时器配置
    SERVOS2_CHAIN_GET_TIME_CONFIG_SINGLE   = 0x92,  // Get single timer config / 获取单个定时器配置
    SERVOS2_CHAIN_GET_TIME_CONFIG_ALL      = 0x93,  // Get all timer config / 获取所有定时器配置
    SERVOS2_CHAIN_GET_VREF                 = 0xA0,  // Get voltage reference / 获取参考电压
    SERVOS2_CHAIN_GET_GROVE_VOLTAGE        = 0xA1,  // Get Grove voltage / 获取Grove电压
    SERVOS2_CHAIN_GET_DC_VOLTAGE           = 0xA2,  // Get DC voltage / 获取DC电压
    SERVOS2_CHAIN_GET_SYS_CURRENT          = 0xA3,  // Get system current / 获取系统电流
} servos2_chain_cmd_t;

// --- Global Register & Flag Declarations ---
// --- 全局寄存器和标志声明 ---

// Change flags (Set when I2C write occurs, cleared by system update loop)
// 变化标志（I2C 写入时置位，由系统更新循环清除）
extern volatile uint8_t gpio_servo_angle_changed[8];  // Per-GPIO servo angle change flag / 每个GPIO的舵机角度变化标志
extern volatile uint8_t gpio_servo_angle_changed_any;  // Any servo angle changed / 任意舵机角度变化标志
extern volatile uint8_t gpio_pwm_duty_changed[8];  // Per-GPIO PWM duty change flag / 每个GPIO的PWM占空比变化标志
extern volatile uint8_t gpio_pwm_duty_changed_any;  // Any PWM duty changed / 任意PWM占空比变化标志
extern volatile uint8_t gpio_rgb_buf_changed_any;   // RGB buffer changed / RGB缓冲区变化标志
extern volatile uint8_t tim_freq_changed[2];   // Per-timer frequency change flag / 每个定时器的频率变化标志
extern volatile uint8_t tim_freq_changed_any;  // Any timer frequency changed / 任意定时器频率变化标志

// Register Data Stores
// 寄存器数据存储
extern volatile uint8_t gpio_mode_reg[8];              // GPIO mode register / GPIO模式寄存器
extern volatile uint8_t gpio_gpio_input_pu_pd_reg[8];  // GPIO input pull-up/down register / GPIO输入上下拉寄存器
extern volatile uint8_t gpio_gpio_output_status_reg[8];  // GPIO output status register / GPIO输出状态寄存器
extern volatile uint16_t gpio_adc_value_reg[8];          // ADC value register / ADC值寄存器
extern volatile uint8_t gpio_servo_angle_reg[8];         // Servo angle register / 舵机角度寄存器
extern volatile rgb_config_t gpio_rgb_reg[8];            // RGB configuration register / RGB配置寄存器
extern volatile uint8_t gpio_pwm_duty_reg[8];            // PWM duty register / PWM占空比寄存器
extern volatile uint32_t gpio_rgb_color_buf_reg[16];     // RGB color buffer register / RGB颜色缓冲寄存器
extern volatile user_sys_time_config_t tim_freq_reg[2];  // Timer frequency register / 定时器频率寄存器
extern volatile uint16_t vref_mv_reg;                    // Voltage reference in mV / 参考电压(毫伏)
extern volatile uint16_t grove_voltage_mv_reg;           // Grove voltage in mV / Grove电压(毫伏)
extern volatile uint16_t dc_voltage_mv_reg;              // DC voltage in mV / DC电压(毫伏)
extern volatile uint16_t sys_current_ma_reg;             // System current in mA / 系统电流(毫安)
extern volatile uint8_t fw_version_reg;                  // Firmware version / 固件版本
extern volatile uint8_t i2c_addr_reg;                    // I2C address / I2C地址

/* Function Prototypes / 函数原型 ----------------------------------------*/

// Mode control functions / 模式控制函数
void servos2_chain_set_mode_single(uint8_t gpio, uint8_t mode);
void servos2_chain_set_mode_all(uint8_t *mode, uint8_t size);
void servos2_chain_get_mode_single(uint8_t gpio);
void servos2_chain_get_mode_all(void);

// Input pull-up/down functions / 输入上下拉函数
void servos2_chain_set_input_pu_pd_single(uint8_t gpio, uint8_t pu_pd);
void servos2_chain_set_input_pu_pd_all(uint8_t *pu_pd, uint8_t size);
void servos2_chain_get_input_pu_pd_single(uint8_t gpio);
void servos2_chain_get_input_pu_pd_all(void);

// Input status functions / 输入状态函数
void servos2_chain_get_input_status_single(uint8_t gpio);
void servos2_chain_get_input_status_all(void);

// Output status functions / 输出状态函数
void servos2_chain_set_output_status_single(uint8_t gpio, uint8_t status);
void servos2_chain_set_output_status_all(uint8_t *status, uint8_t size);
void servos2_chain_get_output_status_single(uint8_t gpio);
void servos2_chain_get_output_status_all(void);

// ADC functions / ADC函数
void servos2_chain_get_adc_value_single(uint8_t gpio);
void servos2_chain_get_adc_value_all(void);

// Servo angle functions / 舵机角度函数
void servos2_chain_set_servo_angle_single(uint8_t gpio, uint8_t angle);
void servos2_chain_set_servo_angle_all(uint8_t *angle, uint8_t size);
void servos2_chain_get_servo_angle_single(uint8_t gpio);
void servos2_chain_get_servo_angle_all(void);

// RGB configuration functions / RGB配置函数
void servos2_chain_set_rgb_config_single(uint8_t gpio, uint8_t rgb);
void servos2_chain_set_rgb_config_all(uint8_t *rgb, uint8_t size);
void servos2_chain_get_rgb_config_single(uint8_t gpio);
void servos2_chain_get_rgb_config_all(void);

// RGB buffer functions / RGB缓冲函数
void servos2_chain_set_rgb_buffer_single(uint8_t *buffer, uint8_t size);
void servos2_chain_set_rgb_buffer_all(uint8_t *buffer, uint8_t size);
void servos2_chain_get_rgb_buffer_single(uint8_t index);
void servos2_chain_get_rgb_buffer_all(void);

// PWM duty functions / PWM占空比函数
void servos2_chain_set_pwm_duty_single(uint8_t gpio, uint8_t duty);
void servos2_chain_set_pwm_duty_all(uint8_t *duty, uint8_t size);
void servos2_chain_get_pwm_duty_single(uint8_t gpio);
void servos2_chain_get_pwm_duty_all(void);

// Timer configuration functions / 定时器配置函数
void servos2_chain_set_time_config_single(uint8_t *buffer, uint8_t size);
void servos2_chain_set_time_config_all(uint8_t *buffer, uint8_t size);
void servos2_chain_get_time_config_single(uint8_t time);
void servos2_chain_get_time_config_all(void);

// System voltage and current functions / 系统电压和电流函数
void servos2_chain_get_vref(void);
void servos2_chain_get_grove_voltage(void);
void servos2_chain_get_dc_voltage(void);
void servos2_chain_get_sys_current(void);

#ifdef __cplusplus
}
#endif

#endif /* __SERVOS2_CHAIN_FUNCTION_H__ */
