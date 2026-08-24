#include "myflash.h"

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
static uint64_t my_flash_read_double_word(uint32_t address)
{
    return *((__IO uint64_t *)(address));  // Read and return the 64-bit value
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

    return (status == HAL_OK);  // Return success status
}

static void flash_config_read_page(uint64_t *data, uint8_t double_word_count)
{
    for (uint8_t i = 0; i < double_word_count; i++) {
        data[i] = my_flash_read_double_word(STM32G0xx_FLASH_PAGE31_STARTADDR + ((uint32_t)i * 8U));
    }
}

static bool flash_config_write_page(uint64_t *data, uint8_t double_word_count)
{
    bool status = true;

    __disable_irq();
    status = my_flash_earse_pages(STM32G0xx_FLASH_PAGE31_STARTADDR);
    if (status) {
        for (uint8_t i = 0; i < double_word_count; i++) {
            if (!my_flash_write_double_word(STM32G0xx_FLASH_PAGE31_STARTADDR + ((uint32_t)i * 8U), data[i])) {
                status = false;
                break;
            }
        }
    }
    __enable_irq();

    return status;
}

/**
 * @brief Write one byte in the shared config flash page.
 * @param byte_index Byte offset from STM32G0xx_FLASH_PAGE31_STARTADDR.
 * @param data New byte value.
 * @retval true if the byte is written successfully, false otherwise.
 */
bool flash_config_set_byte(uint8_t byte_index, uint8_t data)
{
    return flash_config_write_bytes(byte_index, &data, 1U);
}

/**
 * @brief Write bytes in the first 16 bytes of the shared config flash page.
 * @param byte_index Start byte offset from STM32G0xx_FLASH_PAGE31_STARTADDR.
 * @param data Data buffer to write.
 * @param len Number of bytes to write.
 * @retval true if all bytes are written successfully, false otherwise.
 */
bool flash_config_write_bytes(uint8_t byte_index, const uint8_t *data, uint8_t len)
{
    uint64_t page_data[2] = {0};
    uint8_t *bytes        = (uint8_t *)page_data;

    if (data == NULL || len == 0U || byte_index >= sizeof(page_data) || len > (sizeof(page_data) - byte_index)) {
        return false;
    }

    flash_config_read_page(page_data, 2U);

    for (uint8_t i = 0; i < len; i++) {
        bytes[byte_index + i] = data[i];
    }

    return flash_config_write_page(page_data, 2U);
}

/**
 * @brief Read one byte from the shared config flash page.
 * @param byte_index Byte offset from STM32G0xx_FLASH_PAGE31_STARTADDR.
 * @retval Value stored in flash, or 0xFF if the offset is invalid.
 */
uint8_t flash_config_get_byte(uint8_t byte_index)
{
    if (byte_index >= 16U) {
        return 0xFFU;
    }

    return *((__IO uint8_t *)(STM32G0xx_FLASH_PAGE31_STARTADDR + byte_index));
}

/**
 * @brief Read bytes from the first 16 bytes of the shared config flash page.
 * @param byte_index Start byte offset from STM32G0xx_FLASH_PAGE31_STARTADDR.
 * @param data Destination buffer.
 * @param len Number of bytes to read.
 * @retval None
 */
void flash_config_read_bytes(uint8_t byte_index, uint8_t *data, uint8_t len)
{
    if (data == NULL || len == 0U || byte_index >= 16U || len > (16U - byte_index)) {
        return;
    }

    for (uint8_t i = 0; i < len; i++) {
        data[i] = flash_config_get_byte(byte_index + i);
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
    bool status = flash_config_set_byte(0U, data);

    return (status && get_rgb_light() == data);
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
