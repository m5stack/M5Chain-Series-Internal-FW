/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MYFLASH_H
#define __MYFLASH_H

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdbool.h>
#include "stm32g0xx_hal_flash_ex.h"
#include "RGB.h"

#define STM32G0xx_PAGE_SIZE             (0x800)      // Page size: 2048 bytes (2 KB)
#define STM32G0xx_FLASH_PAGE0_STARTADDR (0x8000000)  // Start address of flash page 0
#define STM32G0xx_FLASH_PAGE31_STARTADDR \
    (STM32G0xx_FLASH_PAGE0_STARTADDR + 31 * STM32G0xx_PAGE_SIZE)        // Start address of flash page 24
#define RGB_LIGHT_ADDR          (STM32G0xx_FLASH_PAGE31_STARTADDR + 0)  // Address for RGB light data
#define BOOTLOADER_VERSION_ADDR (APPLICATION_ADDRESS - 1)               // Address for bootloader version

uint8_t get_bootloader_version(void);  // Get bootloader version
bool set_rgb_light(uint8_t data);      // Set RGB light with specified data
uint8_t get_rgb_light(void);           // Get current RGB light value
bool flash_config_set_byte(uint8_t byte_index, uint8_t data);
bool flash_config_write_bytes(uint8_t byte_index, const uint8_t *data, uint8_t len);
uint8_t flash_config_get_byte(uint8_t byte_index);
void flash_config_read_bytes(uint8_t byte_index, uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* __MYFLASH_H */
