/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "myflash.h"

__IO uint16_t g_flash_data[12] = {0};

/**
 * @brief Get the page number based on the address.
 * @note This function calculates the page number by determining how far
 *       the provided address is from the starting address of the first flash page.
 *
 * @param addr The memory address to check.
 * @retval The page number corresponding to the provided address.
 */
static uint32_t get_page(uint32_t addr)
{
    return (addr - STM32G0xx_FLASH_PAGE0_STARTADDR) / STM32G0xx_PAGE_SIZE;  // Return the page number
}

/**
 * @brief Read a double word (64 bits) from a given memory address.
 * @note This function dereferences the provided address and returns the 64-bit
 *       value stored there. It uses the volatile pointer type to ensure that
 *       the compiler does not optimize out the read operation.
 *
 * @param address The memory address from which to read.
 * @retval The 64-bit value read from the memory address.
 */
static uint64_t my_flash_read_half_word(uint32_t address)
{
    return *((__IO uint16_t *)(address));  // Read and return the 64-bit value
}

/**
 * @brief Erase a specific flash page.
 * @note This function initiates the page erase operation for the specified flash
 *       memory address. It unlocks the flash memory, performs the erase, and
 *       then locks the flash memory again. The function returns true if the
 *       operation is successful; otherwise, it returns false.
 *
 * @param page_address The address of the page to be erased.
 * @retval Success status (true if successful, false otherwise).
 */
static bool my_flash_earse_pages(uint32_t page_address)
{
    uint32_t page_error = 0;          // Variable to hold error information if the erase fails
    FLASH_EraseInitTypeDef my_flash;  // Structure for flash erase initialization

    my_flash.TypeErase = FLASH_TYPEERASE_PAGES;   // Specify that we are erasing pages
    my_flash.Page      = get_page(page_address);  // Get the page number based on the address
    my_flash.NbPages   = 1;                       // Specify that we want to erase one page

    HAL_FLASH_Unlock();                                                    // Unlock the flash memory for write access
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&my_flash, &page_error);  // Perform the erase operation
    HAL_FLASH_Lock();                                                      // Lock the flash memory again

    // Return true if the erase was successful, false otherwise
    return (status == HAL_OK) ? true : false;
}

/**
 * @brief Write a double word to the specified memory address.
 * @note This function unlocks the flash memory, programs a double word at
 *       the given address, and then locks the flash memory again.
 *
 * @param address The memory address to which the data will be written.
 * @param data The 64-bit data to be written.
 * @retval true if the write operation is successful, false otherwise.
 */
static bool my_flash_write_double_word(uint32_t address, uint64_t data)
{
    HAL_FLASH_Unlock();  // Unlock the flash memory for writing
    HAL_StatusTypeDef status =
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);  // Program the double word
    HAL_FLASH_Lock();                                                    // Lock the flash memory again
    return (status == HAL_OK);                                           // Return success status
}

/**
 * @brief Read parameters stored in Flash memory.
 * @note This function reads twelve half-words from the specified Flash page
 *       and stores them in the global flash data array.
 *
 * @param None
 * @retval None
 */
void read_flash_data(void)
{
    // Read stored parameters from Flash memory
    for (uint8_t i = 0; i < 12; i++) {
        g_flash_data[i] = my_flash_read_half_word(STM32G0xx_FLASH_PAGE31_STARTADDR + i * sizeof(uint16_t));
    }
}

/**
 * @brief Write data back to Flash memory.
 * @note This function erases the specified Flash page and writes the current
 *       global flash data array in chunks of four half-words (64 bits).
 *
 * @param None
 * @retval None
 */
void write_flash_data(void)
{
    // Erase the Flash page before writing new data
    my_flash_earse_pages(STM32G0xx_FLASH_PAGE31_STARTADDR);

    // Write data in chunks of 4 half-words (64 bits)
    for (uint8_t i = 0; i < 12; i += 4) {
        uint64_t temp = 0;

        // Prepare a double-word (64 bits) from four half-words
        for (uint8_t j = 0; j < 4 && (i + j) < 12; j++) {
            temp |= ((uint64_t)g_flash_data[i + j] << (j * 16));  // Combine into 64-bit value
        }

        // Write the double-word to Flash memory
        my_flash_write_double_word(STM32G0xx_FLASH_PAGE31_STARTADDR + i * sizeof(uint16_t), temp);
    }
}

/**
 * @brief Modify the brightness of the RGB light.
 * @note This function reads the current state of the RGB light data,
 *       modifies the specified byte with the new brightness value,
 *       erases the flash page, and writes the updated data back to flash.
 *
 * @param data The new brightness value to set (0-255).
 * @retval true if the brightness modification is successful, false otherwise.
 */
bool set_rgb_light(uint8_t data)
{
    g_flash_data[0] = (g_flash_data[0] & 0x00FF) | ((data & 0x00FF) << 8);
    __disable_irq();
    write_flash_data();
    __enable_irq();
    return (get_rgb_light() == data);
}

/**
 * @brief Get the current RGB brightness value.
 * @note This function reads the current brightness value stored at the
 *       defined RGB light address.
 *
 * @param None
 * @retval The current RGB brightness value.
 */
uint8_t get_rgb_light(void)
{
    return *((__IO uint8_t *)(RGB_LIGHT_ADDR));  // Read and return the current RGB brightness value
}

/**
 * @brief Get the bootloader version number.
 * @note This function reads the bootloader version number stored at the
 *       defined address.
 *
 * @param None
 * @retval The bootloader version number.
 */
uint8_t get_bootloader_version(void)
{
    return *((__IO uint8_t *)(BOOTLOADER_VERSION_ADDR));  // Read and return the bootloader version number
}

/**
 * @brief Set the mapping range using the provided buffer.
 * @note This function copies the data from the input buffer to the global
 *       flash data array starting from index 4 and writes it to Flash memory.
 *
 * @param buffer Pointer to the array containing the mapping range values.
 * @param size   Number of elements to be set in the mapping range.
 * @retval true  If the mapping range was successfully set.
 * @retval false If the size exceeds the maximum limit or an error occurred.
 */
bool set_map_range(uint16_t *buffer, uint16_t size)
{
    // Copy data from the input buffer to the global flash data array
    for (uint8_t i = 0; i < size; i++) {
        g_flash_data[i + 4] = buffer[i];  // Store values starting from index 4
    }

    // Write updated flash data to memory
    __disable_irq();
    write_flash_data();
    __enable_irq();
    return true;  // Return true indicating success
}

/**
 * @brief Get the mapping range into the provided buffer.
 * @note This function reads the current flash data and copies the mapping
 *       range values into the provided buffer.
 *
 * @param buffer Pointer to the array where the mapping range values will be stored.
 * @param size   Number of elements to be retrieved from the mapping range.
 * @retval The number of elements copied to the buffer.
 */
uint8_t get_map_range(uint16_t *buffer, uint16_t size)
{
    // Read the current flash data into the global array
    read_flash_data();

    // Copy data from the global flash data array to the output buffer
    for (uint8_t i = 0; i < size; i++) {
        buffer[i] = g_flash_data[i + 4];  // Retrieve values starting from index 4
    }

    return size;  // Return the number of elements copied
}
