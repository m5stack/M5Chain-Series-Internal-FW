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

#define STM32G0xx_PAGE_SIZE             (0x800)      // Page size: 2048 bytes (2 KB)
#define STM32G0xx_FLASH_PAGE0_STARTADDR (0x8000000)  // Start address of flash page 0

#define STM32G0xx_FLASH_PAGE28_STARTADDR \
    (STM32G0xx_FLASH_PAGE0_STARTADDR + 28 * STM32G0xx_PAGE_SIZE)  // Start address of flash page 28
#define STM32G0xx_FLASH_PAGE29_STARTADDR \
    (STM32G0xx_FLASH_PAGE0_STARTADDR + 29 * STM32G0xx_PAGE_SIZE)  // Start address of flash page 29
#define STM32G0xx_FLASH_PAGE30_STARTADDR \
    (STM32G0xx_FLASH_PAGE0_STARTADDR + 30 * STM32G0xx_PAGE_SIZE)  // Start address of flash page 30
#define STM32G0xx_FLASH_PAGE31_STARTADDR \
    (STM32G0xx_FLASH_PAGE0_STARTADDR + 31 * STM32G0xx_PAGE_SIZE)  // Start address of flash page 31

#define PEDAL_OVERLAY_ADDR           (STM32G0xx_FLASH_PAGE28_STARTADDR + 0)
#define PEDAL_ENTER_CONFIG_TIME_ADDR (STM32G0xx_FLASH_PAGE29_STARTADDR + 0)
#define PEADL_WORK_MODE_ADDR         (STM32G0xx_FLASH_PAGE30_STARTADDR + 0)
#define RGB_LIGHT_ADDR               (STM32G0xx_FLASH_PAGE31_STARTADDR + 0)
#define BOOTLOADER_VERSION_ADDR      (APPLICATION_ADDRESS - 1)

bool set_pedal_overlay(uint8_t data);            // Set pedal overlay with specified data
uint8_t get_pedal_overlay(void);                 // Get current pedal overlay value
bool set_pedal_enter_config_time(uint8_t data);  // Set pedal enter config time with specified data
uint8_t get_pedal_enter_config_time(void);       // Get current pedal enter config time value
bool set_pedal_work_mode(uint8_t data);          // Set pedal work mode with specified data
uint8_t get_pedal_work_mode(void);               // Get current pedal work mode value
bool set_rgb_light(uint8_t data);                // Set RGB light with specified data
uint8_t get_rgb_light(void);                     // Get current RGB light value
uint8_t get_bootloader_version(void);            // Get bootloader version

#ifdef __cplusplus
}
#endif

#endif /* __MYFLASH_H */
