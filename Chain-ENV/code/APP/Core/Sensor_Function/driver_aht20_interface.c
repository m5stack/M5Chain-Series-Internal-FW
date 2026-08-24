/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file      driver_aht20_interface_template.c
 * @brief     driver aht20 interface template source file
 * @version   1.0.0
 * @author    Shifeng Li
 * @date      2022-10-31
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2022/10/31  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */

#include "driver_aht20_interface.h"
#include "i2c.h"
#include <stdarg.h>
#include <stdio.h>

#define AHT20_I2C_HANDLE     hi2c2
#define AHT20_I2C_TIMEOUT_MS 100U

aht20_handle_t aht20_handle; /**< aht20 handle */

/**
 * @brief  interface iic bus init
 * @return status code
 *         - 0 success
 *         - 1 iic init failed
 * @note   none
 */
uint8_t aht20_interface_iic_init(void)
{
    if (HAL_I2C_GetState(&AHT20_I2C_HANDLE) == HAL_I2C_STATE_RESET) {
        MX_I2C2_Init();
    }

    return 0;
}

/**
 * @brief  interface iic bus deinit
 * @return status code
 *         - 0 success
 *         - 1 iic deinit failed
 * @note   none
 */
uint8_t aht20_interface_iic_deinit(void)
{
    if (HAL_I2C_DeInit(&AHT20_I2C_HANDLE) != HAL_OK) {
        return 1;
    }

    return 0;
}

/**
 * @brief      interface iic bus read
 * @param[in]  addr iic device write address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t aht20_interface_iic_read_cmd(uint8_t addr, uint8_t *buf, uint16_t len)
{
    if (HAL_I2C_Master_Receive(&AHT20_I2C_HANDLE, addr, buf, len, AHT20_I2C_TIMEOUT_MS) != HAL_OK) {
        return 1;
    }

    return 0;
}

/**
 * @brief     interface iic bus write
 * @param[in] addr iic device write address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t aht20_interface_iic_write_cmd(uint8_t addr, uint8_t *buf, uint16_t len)
{
    if (HAL_I2C_Master_Transmit(&AHT20_I2C_HANDLE, addr, buf, len, AHT20_I2C_TIMEOUT_MS) != HAL_OK) {
        return 1;
    }

    return 0;
}

/**
 * @brief     interface delay ms
 * @param[in] ms time
 * @note      none
 */
void aht20_interface_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/**
 * @brief     interface print format data
 * @param[in] fmt format data
 * @note      none
 */
void aht20_interface_debug_print(const char *const fmt, ...)
{
    va_list args;
    char str[128];

    va_start(args, fmt);
    (void)vsnprintf(str, sizeof(str), fmt, args);
    va_end(args);
}
