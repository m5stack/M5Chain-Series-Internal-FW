/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "uart_function.h"

static uint8_t s_ret_buf[256]       = {0};
static uint8_t s_ret_buf_size       = 0;
static uint8_t s_i2c_buffer[256]    = {0};
static uint16_t s_adc_buffer[2]     = {0};
__IO gpio_config_t g_gpio_status[2] = {
    {.mode = CHAIN_NOT_WORK_STATUS, .gpio_level = CHAIN_GPIO_RESET, .gpio_pin = GPIO_PIN_11},
    {.mode = CHAIN_NOT_WORK_STATUS, .gpio_level = CHAIN_GPIO_RESET, .gpio_pin = GPIO_PIN_12}};

static uint32_t last_interrupt_time_pin11 = 0;
static uint32_t last_interrupt_time_pin12 = 0;

static void return_error(uart_cmd_t cmd)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = CHAIN_UART_OPERATION_FAIL;
    chain_command_complete_return(cmd, s_ret_buf, s_ret_buf_size);
}

static void return_success(uart_cmd_t cmd)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = CHAIN_UART_OPERATION_SUCCESS;
    chain_command_complete_return(cmd, s_ret_buf, s_ret_buf_size);
}

static void return_mode_match_error(uart_cmd_t cmd)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = CHAIN_WORK_MODE_MISMATCHED;
    chain_command_complete_return(cmd, s_ret_buf, s_ret_buf_size);
}

void clear_i2c_flag(void)
{
    __disable_irq();
    // 清除I2C2的Busy标志（如果需要）
    if (__HAL_I2C_GET_FLAG(&hi2c2, I2C_FLAG_BUSY)) {
        // 需要确保I2C不在忙状态
        __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_BUSY);
    }
    // 清除其他标志（例如超时）
    __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_TXE);
    __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_BERR);
    __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_ARLO);
    __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_OVR);
    __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_PECERR);
    __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_ALERT);
    __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_TIMEOUT);
    __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_AF);     // 清除应答失败标志
    __HAL_I2C_CLEAR_FLAG(&hi2c2, I2C_FLAG_STOPF);  // 清除NACK接收标志
    HAL_I2C_DeInit(&hi2c2);
    MX_I2C2_Init(g_gpio_status[0].i2c_speed);
    __enable_irq();
}

bool check_i2c_mode(void)
{
    if (g_gpio_status[0].mode == CHAIN_I2C_WORK_STATUS && g_gpio_status[1].mode == CHAIN_I2C_WORK_STATUS) {
        return true;
    }
    return false;
}

void chain_i2c_init(uint8_t speed)
{
    if (speed >= CHAIN_I2C_LOW_SPEED_100KHZ && speed <= CHAIN_I2C_HIGH_SPEED_400KHZ) {
        __disable_irq();
        HAL_I2C_MspDeInit(&hi2c2);
        MX_I2C2_Init(speed);
        __enable_irq();
        g_gpio_status[0].mode = CHAIN_I2C_WORK_STATUS;
        g_gpio_status[1].mode = CHAIN_I2C_WORK_STATUS;
        return_success(CHAIN_I2C_INIT);
    } else {
        return_error(CHAIN_I2C_INIT);
    }
}

void chain_i2c_read(uint8_t i2c_addr, uint8_t read_len)
{
    if (check_i2c_mode()) {
        if (i2c_addr <= 127 && read_len <= I2C_READ_MAX_SIZE) {
            HAL_StatusTypeDef status = HAL_I2C_Master_Receive(&hi2c2, (i2c_addr << 1), s_i2c_buffer, read_len, 1000);
            if (status == HAL_OK) {
                s_ret_buf_size              = 0;
                s_ret_buf[s_ret_buf_size++] = CHAIN_UART_OPERATION_SUCCESS;
                for (uint8_t i = 0; i < read_len; i++) {
                    s_ret_buf[s_ret_buf_size++] = s_i2c_buffer[i];
                }
                chain_command_complete_return(CHAIN_I2C_READ, s_ret_buf, s_ret_buf_size);
            } else {
                if (status == HAL_ERROR) {
                    clear_i2c_flag();
                }
                return_error(CHAIN_I2C_READ);
            }
        } else {
            return_error(CHAIN_I2C_READ);
        }
    } else {
        return_mode_match_error(CHAIN_I2C_READ);
    }
}

void chain_i2c_write(uint8_t i2c_addr, uint8_t *buffer, uint16_t write_len)
{
    if (check_i2c_mode()) {
        if (i2c_addr <= 127 && write_len <= I2C_READ_MAX_SIZE) {
            HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c2, (i2c_addr << 1), buffer, write_len, 1000);
            if (status == HAL_OK) {
                return_success(CHAIN_I2C_WRITE);
            } else {
                if (status == HAL_ERROR) {
                    clear_i2c_flag();
                }
                return_error(CHAIN_I2C_WRITE);
            }
        } else {
            return_error(CHAIN_I2C_WRITE);
        }
    } else {
        return_mode_match_error(CHAIN_I2C_WRITE);
    }
}

void chain_i2c_mem_read(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_len, uint8_t read_len)
{
    if (check_i2c_mode()) {
        if (i2c_addr <= 127 && (reg_len >= 1 && reg_len <= 2) && read_len <= I2C_READ_MAX_SIZE) {
            HAL_StatusTypeDef status =
                HAL_I2C_Mem_Read(&hi2c2, (i2c_addr << 1), reg_addr, reg_len, s_i2c_buffer, read_len, 1000);
            if (status == HAL_OK) {
                s_ret_buf_size              = 0;
                s_ret_buf[s_ret_buf_size++] = CHAIN_UART_OPERATION_SUCCESS;
                for (uint8_t i = 0; i < read_len; i++) {
                    s_ret_buf[s_ret_buf_size++] = s_i2c_buffer[i];
                }
                chain_command_complete_return(CHAIN_I2C_MEM_READ, s_ret_buf, s_ret_buf_size);
            } else {
                if (status == HAL_ERROR) {
                    clear_i2c_flag();
                }
                return_error(CHAIN_I2C_MEM_READ);
            }
        } else {
            return_error(CHAIN_I2C_MEM_READ);
        }
    } else {
        return_mode_match_error(CHAIN_I2C_MEM_READ);
    }
}

void chain_i2c_mem_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t reg_len, uint8_t write_len, uint8_t *buffer)
{
    if (check_i2c_mode()) {
        if (i2c_addr <= 127 && (reg_len >= 1 && reg_len <= 2) && write_len <= I2C_READ_MAX_SIZE) {
            HAL_StatusTypeDef status =
                HAL_I2C_Mem_Write(&hi2c2, (i2c_addr << 1), reg_addr, reg_len, buffer, write_len, 1000);
            if (status == HAL_OK) {
                return_success(CHAIN_I2C_MEM_WRITE);
            } else {
                if (status == HAL_ERROR) {
                    clear_i2c_flag();
                }
                return_error(CHAIN_I2C_MEM_WRITE);
            }
        } else {
            return_error(CHAIN_I2C_MEM_WRITE);
        }
    } else {
        return_mode_match_error(CHAIN_I2C_MEM_WRITE);
    }
}

void chain_i2c_scan_addr(void)
{
    if (check_i2c_mode()) {
        uint8_t i2c_addr_num = 0;
        s_ret_buf_size       = 0;
        s_ret_buf[s_ret_buf_size++];
        s_ret_buf[s_ret_buf_size++];
        for (uint8_t i = 1; i < 128; i++) {
            HAL_StatusTypeDef result = HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(i << 1), 1, 10);
            if (result == HAL_OK) {
                i2c_addr_num++;
                s_ret_buf[s_ret_buf_size++] = i;
            }
        }
        s_ret_buf[0] = CHAIN_UART_OPERATION_SUCCESS;
        s_ret_buf[1] = i2c_addr_num;
        chain_command_complete_return(CHAIN_I2C_SCAN_ADDR, s_ret_buf, s_ret_buf_size);
    } else {
        return_mode_match_error(CHAIN_I2C_SCAN_ADDR);
    }
}

void GPIO_OUTPUT_Init(gpio_pin_t gpio_pin, gpio_mode_t gpio_mode, gpio_up_mode_t gpio_up_mode)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode                = LL_GPIO_MODE_OUTPUT;  // 输出模式
    // GPIO配置
    switch (gpio_pin) {
        case CHAIN_GPIO_PIN_12:
            HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);
            GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
            break;
        case CHAIN_GPIO_PIN_11:
            HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11);
            GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
            break;
        default:
            break;
    }

    // 实际输出电平去配置
    if (g_gpio_status[gpio_pin - 1].gpio_level == CHAIN_GPIO_RESET) {
        LL_GPIO_ResetOutputPin(GPIOA, GPIO_InitStruct.Pin);
    } else {
        LL_GPIO_SetOutputPin(GPIOA, GPIO_InitStruct.Pin);
    }

    // 输出模式配置
    switch (gpio_mode) {
        case CHAIN_GPIO_OUTPUT_PUSHPULL:
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            break;
        case CHAIN_GPIO_OUTPUT_OPENDRAIN:
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
            break;
        default:
            break;
    }

    // 上拉下拉配置
    switch (gpio_up_mode) {
        case CHAIN_GPIO_PULL_UP:
            GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
            break;
        case CHAIN_GPIO_PULL_DOWN:
            GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
            break;
        case CHAIN_GPIO_PULL_NO:
            GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
            break;
        default:
            break;
    }

    // 速度默认配置中速
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;

    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void chain_uart_output_init(uint8_t *buffer, uint16_t size)
{
    if (size == 3 && buffer[0] >= 1 && buffer[0] <= 2 && buffer[1] >= CHAIN_GPIO_OUTPUT_PUSHPULL &&
        buffer[1] <= CHAIN_GPIO_OUTPUT_OPENDRAIN && buffer[2] >= CHAIN_GPIO_PULL_UP &&
        buffer[2] <= CHAIN_GPIO_PULL_NO) {
        if (g_gpio_status[0].mode == CHAIN_I2C_WORK_STATUS && g_gpio_status[1].mode == CHAIN_I2C_WORK_STATUS) {
            __disable_irq();
            HAL_I2C_MspDeInit(&hi2c2);
            g_gpio_status[0].mode = CHAIN_NOT_WORK_STATUS;
            g_gpio_status[1].mode = CHAIN_NOT_WORK_STATUS;
            GPIO_OUTPUT_Init(buffer[0], buffer[1], buffer[2]);
            __enable_irq();
        } else {
            __disable_irq();
            GPIO_OUTPUT_Init(buffer[0], buffer[1], buffer[2]);
            __enable_irq();
        }
        g_gpio_status[buffer[0] - 1].mode         = CHAIN_OUTPUT_WORK_STATUS;
        g_gpio_status[buffer[0] - 1].gpio_mode    = buffer[1];
        g_gpio_status[buffer[0] - 1].gpio_up_mode = buffer[2];
        return_success(CHAIN_GPIO_OUTPUT_INIT);
    } else {
        return_error(CHAIN_GPIO_OUTPUT_INIT);
    }
}

void chain_uart_set_output_level(uint8_t gpio, uint8_t level)
{
    if (gpio >= 1 && gpio <= 2 && level >= CHAIN_GPIO_RESET && level <= CHAIN_GPIO_SET) {
        if (g_gpio_status[gpio - 1].mode == CHAIN_OUTPUT_WORK_STATUS) {
            if (level == CHAIN_GPIO_RESET) {
                LL_GPIO_ResetOutputPin(GPIOA, g_gpio_status[gpio - 1].gpio_pin);
            } else {
                LL_GPIO_SetOutputPin(GPIOA, g_gpio_status[gpio - 1].gpio_pin);
            }
            g_gpio_status[gpio - 1].gpio_level = level;
            return_success(CHAIN_GPIO_SET_OUTPUT_LEVEL);
        } else {
            return_mode_match_error(CHAIN_GPIO_SET_OUTPUT_LEVEL);
        }
    } else {
        return_error(CHAIN_GPIO_SET_OUTPUT_LEVEL);
    }
}

void chain_uart_get_output_level(uint8_t gpio)
{
    if (gpio >= 1 && gpio <= 2) {
        s_ret_buf_size              = 0;
        s_ret_buf[s_ret_buf_size++] = CHAIN_UART_OPERATION_SUCCESS;
        s_ret_buf[s_ret_buf_size++] = g_gpio_status[gpio - 1].gpio_level;
        chain_command_complete_return(CHAIN_GPIO_GET_OUTPUT_LEVEL, s_ret_buf, s_ret_buf_size);
    } else {
        return_error(CHAIN_GPIO_GET_OUTPUT_LEVEL);
    }
}

void GPIO_INPUT_Init(gpio_pin_t gpio, gpio_up_mode_t gpio_up_mode)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode                = LL_GPIO_MODE_INPUT;
    // GPIO配置
    switch (gpio) {
        case CHAIN_GPIO_PIN_12:
            HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);
            GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
            break;
        case CHAIN_GPIO_PIN_11:
            HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11);
            GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
            break;
        default:
            break;
    }
    // 上拉下拉配置
    switch (gpio_up_mode) {
        case CHAIN_GPIO_PULL_UP:
            GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
            break;
        case CHAIN_GPIO_PULL_DOWN:
            GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
            break;
        case CHAIN_GPIO_PULL_NO:
            GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
            break;
        default:
            break;
    }
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void chain_uart_input_init(uint8_t *buffer, uint16_t size)
{
    if (size == 2 && buffer[0] >= 1 && buffer[0] <= 2 && buffer[1] >= CHAIN_GPIO_PULL_UP &&
        buffer[1] <= CHAIN_GPIO_PULL_NO) {
        if (g_gpio_status[0].mode == CHAIN_I2C_WORK_STATUS && g_gpio_status[1].mode == CHAIN_I2C_WORK_STATUS) {
            __disable_irq();
            HAL_I2C_MspDeInit(&hi2c2);
            g_gpio_status[0].mode = CHAIN_NOT_WORK_STATUS;
            g_gpio_status[1].mode = CHAIN_NOT_WORK_STATUS;
            GPIO_INPUT_Init(buffer[0], buffer[1]);
            __enable_irq();
        } else {
            __disable_irq();
            GPIO_INPUT_Init(buffer[0], buffer[1]);
            __enable_irq();
        }
        g_gpio_status[buffer[0] - 1].mode         = CHAIN_INPUT_WORK_STATUS;
        g_gpio_status[buffer[0] - 1].gpio_up_mode = buffer[1];
        return_success(CHAIN_GPIO_INPUT_INIT);
    } else {
        return_error(CHAIN_GPIO_INPUT_INIT);
    }
}

bool check_input_mode(gpio_pin_t gpio_pin)
{
    if (g_gpio_status[gpio_pin - 1].mode == CHAIN_INPUT_WORK_STATUS) {
        return true;
    }
    return false;
}

void chain_uart_read_pin_level(uint8_t gpio)
{
    if (gpio == CHAIN_GPIO_PIN_11 || gpio == CHAIN_GPIO_PIN_12) {
        s_ret_buf_size              = 0;
        s_ret_buf[s_ret_buf_size++] = CHAIN_UART_OPERATION_SUCCESS;
        s_ret_buf[s_ret_buf_size++] = HAL_GPIO_ReadPin(GPIOA, g_gpio_status[gpio - 1].gpio_pin);
        chain_command_complete_return(CHAIN_GPIO_READ_GPIO_LEVEL, s_ret_buf, s_ret_buf_size);
    } else {
        return_error(CHAIN_GPIO_READ_GPIO_LEVEL);
    }
}

void GPIO_NVIC_Init(gpio_pin_t gpio, gpio_up_mode_t gpio_up_mode, gpio_nvic_trigger_detection_t trigger_detection)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};  // 初始化结构体
    // GPIO配置
    switch (gpio) {
        case CHAIN_GPIO_PIN_12:
            HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);
            GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
            break;
        case CHAIN_GPIO_PIN_11:
            HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11);
            GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
            break;
        default:
            break;
    }
    // GPIO配置
    switch (trigger_detection) {
        case CHAIN_GPIO_MODE_IT_RISING:
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
            break;
        case CHAIN_GPIO_MODE_IT_FALLING:
            GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
            break;
        case CHAIN_GPIO_MODE_IT_RISING_FALLING:
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
            break;
        default:
            break;
    }
    // 上拉下拉配置
    switch (gpio_up_mode) {
        case CHAIN_GPIO_PULL_UP:
            GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
            break;
        case CHAIN_GPIO_PULL_DOWN:
            GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
            break;
        case CHAIN_GPIO_PULL_NO:
            GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
            break;
        default:
            break;
    }
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

// 上升沿回调
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    uint32_t current_time = HAL_GetTick();

    if (GPIO_Pin == GPIO_PIN_12) {
        if ((current_time - last_interrupt_time_pin12) >= DEBOUNCE_TIME_MS) {
            last_interrupt_time_pin12 = current_time;  // 共享时间戳

            s_ret_buf_size              = 0;
            s_ret_buf[s_ret_buf_size++] = CHAIN_GPIO_MODE_IT_RISING;
            s_ret_buf[s_ret_buf_size++] = CHAIN_GPIO_PIN_12;
            chain_command_complete_return(CHAIN_GPIO_EXTERNAL_NVIC_RETURN, s_ret_buf, s_ret_buf_size);
        }
    }

    if (GPIO_Pin == GPIO_PIN_11) {
        if ((current_time - last_interrupt_time_pin11) >= DEBOUNCE_TIME_MS) {
            last_interrupt_time_pin11 = current_time;  // 共享时间戳

            s_ret_buf_size              = 0;
            s_ret_buf[s_ret_buf_size++] = CHAIN_GPIO_MODE_IT_RISING;
            s_ret_buf[s_ret_buf_size++] = CHAIN_GPIO_PIN_11;
            chain_command_complete_return(CHAIN_GPIO_EXTERNAL_NVIC_RETURN, s_ret_buf, s_ret_buf_size);
        }
    }
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
}

// 下降沿回调
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    uint32_t current_time = HAL_GetTick();

    if (GPIO_Pin == GPIO_PIN_12) {
        if ((current_time - last_interrupt_time_pin12) >= DEBOUNCE_TIME_MS) {
            last_interrupt_time_pin12 = current_time;  // 使用相同的时间戳

            s_ret_buf_size              = 0;
            s_ret_buf[s_ret_buf_size++] = CHAIN_GPIO_MODE_IT_FALLING;
            s_ret_buf[s_ret_buf_size++] = CHAIN_GPIO_PIN_12;
            chain_command_complete_return(CHAIN_GPIO_EXTERNAL_NVIC_RETURN, s_ret_buf, s_ret_buf_size);
        }
    }

    if (GPIO_Pin == GPIO_PIN_11) {
        if ((current_time - last_interrupt_time_pin11) >= DEBOUNCE_TIME_MS) {
            last_interrupt_time_pin11 = current_time;  // 使用相同的时间戳

            s_ret_buf_size              = 0;
            s_ret_buf[s_ret_buf_size++] = CHAIN_GPIO_MODE_IT_FALLING;
            s_ret_buf[s_ret_buf_size++] = CHAIN_GPIO_PIN_11;
            chain_command_complete_return(CHAIN_GPIO_EXTERNAL_NVIC_RETURN, s_ret_buf, s_ret_buf_size);
        }
    }
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
}

void EXTI4_15_IRQHandler(void)
{
    if (g_gpio_status[0].mode == CHAIN_NVIC_WORK_STATUS) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    }
    if (g_gpio_status[1].mode == CHAIN_NVIC_WORK_STATUS) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    }
}

void chain_uart_nvic_init(uint8_t *buffer, uint16_t size)
{
    if (size == 3 && buffer[0] >= 1 && buffer[0] <= 2 && buffer[1] >= CHAIN_GPIO_PULL_UP &&
        buffer[1] <= CHAIN_GPIO_PULL_NO && buffer[2] >= CHAIN_GPIO_MODE_IT_RISING &&
        buffer[2] <= CHAIN_GPIO_MODE_IT_RISING_FALLING) {
        if (g_gpio_status[0].mode == CHAIN_I2C_WORK_STATUS && g_gpio_status[1].mode == CHAIN_I2C_WORK_STATUS) {
            __disable_irq();
            HAL_I2C_MspDeInit(&hi2c2);
            g_gpio_status[0].mode = CHAIN_NOT_WORK_STATUS;
            g_gpio_status[1].mode = CHAIN_NOT_WORK_STATUS;
            GPIO_NVIC_Init(buffer[0], buffer[1], buffer[2]);
            __enable_irq();
        } else {
            __disable_irq();
            GPIO_NVIC_Init(buffer[0], buffer[1], buffer[2]);
            __enable_irq();
        }

        g_gpio_status[buffer[0] - 1].mode                        = CHAIN_NVIC_WORK_STATUS;
        g_gpio_status[buffer[0] - 1].gpio_up_mode                = buffer[1];
        g_gpio_status[buffer[0] - 1].gpio_nvic_trigger_detection = buffer[2];
        return_success(CHAIN_GPIO_EXTERNAL_NVIC_INIT);
    } else {
        return_error(CHAIN_GPIO_EXTERNAL_NVIC_INIT);
    }
}

void chain_uart_adc_init(uint8_t adc_channel)
{
    if (adc_channel >= 1 && adc_channel <= 2) {
        if (g_gpio_status[0].mode == CHAIN_I2C_WORK_STATUS && g_gpio_status[1].mode == CHAIN_I2C_WORK_STATUS) {
            __disable_irq();
            HAL_I2C_MspDeInit(&hi2c2);
            g_gpio_status[0].mode = CHAIN_NOT_WORK_STATUS;
            g_gpio_status[1].mode = CHAIN_NOT_WORK_STATUS;
            __enable_irq();
        }

        GPIO_InitTypeDef GPIO_InitStruct = {0};

        if (adc_channel == 1) {
            if (g_gpio_status[0].mode != CHAIN_ADC_WORK_STATUS) {
                g_gpio_status[0].mode = CHAIN_ADC_WORK_STATUS;
                HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11);
                GPIO_InitStruct.Pin  = GPIO_PIN_11;
                GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
            }
        } else if (adc_channel == 2) {
            if (g_gpio_status[1].mode != CHAIN_ADC_WORK_STATUS) {
                g_gpio_status[1].mode = CHAIN_ADC_WORK_STATUS;
                HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);
                GPIO_InitStruct.Pin  = GPIO_PIN_12;
                GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
            }
        }
        __disable_irq();
        MX_ADC1_Init();
        __enable_irq();

        return_success(CHAIN_GPIO_ADC_INIT);
    } else {
        return_error(CHAIN_GPIO_ADC_INIT);
    }
}

uint16_t get_value(uint32_t channel)
{
    uint32_t temp                  = 0;
    uint8_t counts                 = 0;
    ADC_ChannelConfTypeDef sConfig = {0};
    hadc1.Instance->CHSELR         = 0;
    sConfig.Channel                = channel;
    sConfig.Rank                   = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime           = ADC_SAMPLETIME_12CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADCEx_Calibration_Start(&hadc1);

    for (uint8_t i = 0; i < 5; i++) {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 10);
        if (HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc1), HAL_ADC_STATE_REG_EOC)) {
            temp += HAL_ADC_GetValue(&hadc1);
            counts++;
        }
    }

    return (temp / counts);
}

void chain_uart_adc_read(uint8_t adc_channel)
{
    uint16_t adc_value    = 0;
    uint8_t mismatch_flag = 0;
    if (adc_channel == 1) {
        if (g_gpio_status[0].mode != CHAIN_ADC_WORK_STATUS) {
            mismatch_flag = 1;
        }
    } else if (adc_channel == 2) {
        if (g_gpio_status[1].mode != CHAIN_ADC_WORK_STATUS) {
            mismatch_flag = 1;
        }
    } else {
        mismatch_flag = 1;
    }

    if (mismatch_flag == 1) {
        return_mode_match_error(CHAIN_GPIO_ADC_READ);
    } else {
        s_ret_buf_size = 0;

        if (adc_channel == 1) {
            adc_value = get_value(ADC_CHANNEL_15);
        } else if (adc_channel == 2) {
            adc_value = get_value(ADC_CHANNEL_16);
        }
        s_ret_buf[s_ret_buf_size++] = CHAIN_UART_OPERATION_SUCCESS;
        s_ret_buf[s_ret_buf_size++] = (uint8_t)(adc_value & 0xFF);
        s_ret_buf[s_ret_buf_size++] = (uint8_t)((adc_value >> 8) & 0xFF);
        chain_command_complete_return(CHAIN_GPIO_ADC_READ, s_ret_buf, s_ret_buf_size);
    }
}

void chain_get_gpio_work_status(void)
{
    s_ret_buf_size              = 0;
    s_ret_buf[s_ret_buf_size++] = g_gpio_status[0].mode;
    s_ret_buf[s_ret_buf_size++] = g_gpio_status[1].mode;
    chain_command_complete_return(CHAIN_GET_WORK_STATION, s_ret_buf, s_ret_buf_size);
}
