/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "stm32g0xx_hal_flash_ex.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Enum to represent the different states in the IAP update process
typedef enum {
    IAP_WAITING_ENABLE = 0x00,  // Waiting for enable command
    IAP_ENABLE_COMPLETE,        // Enable command received successfully
    IAP_RECEIVING_DATA,         // In the process of receiving data
    IAP_RECEIVE_COMPLETE,       // Data reception completed successfully
    IAP_UPGRADE_COMPLETE        // Upgrade process completed
} iap_update_t;                 // IAP upgrade status enumeration

// Define constants used in the application
#define APPLICATION_ADDRESS (0x08002000)  // Starting address for the application in flash memory
#define FW_LENGTH           (0xC000)      // Length of the firmware to be updated
#define FW_CRC_ADDR \
    ((APPLICATION_ADDRESS) + (FW_LENGTH) - 4)        // Address for firmware CRC check (last 4 bytes of firmware)
#define UART_RX_LEN                     (2056)       // Length of the UART receive buffer
#define STM32G0xx_PAGE_SIZE             (0x800)      // Size of each flash memory page
#define STM32G0xx_FLASH_PAGE0_STARTADDR (0x8000000)  // Starting address of the device flash memory
#define CHECK_PASS                      (1)          // CRC check passed
#define CHECK_ERROR                     (0)          // CRC check failed
#define RECEIVE_START                   (0x99)       // Marker to indicate the start of data reception
#define RECEIVE_ENABLE                  (0x88)       // Response indicating readiness to receive data
#define RECEIVE_SUCCESS                 (0x77)       // Marker indicating successful data reception
#define RECEIVE_FAIL                    (0x66)       // Marker indicating reception failure; request to resend data
#define RECEIVE_ACCOMPLISH              (0x55)  // Marker indicating reception is complete; ready to jump to the application
#define PACKET_DATA_CONTINUE            (0x01)  // Indicates that more data packets will follow
#define PACKET_DATA_END                 (0x02)  // Indicates that this is the last data packet

#define DELAY_TIME (500)

// Define a function pointer type for the application entry point
typedef void (*pFunction)(void);  // Pointer to a function returning void

// Static variables for managing the IAP process
static uint8_t s_receive_buffer[UART_RX_LEN];             // Buffer for received data
static uint16_t s_indexs = 0;                             // Index for the current position in the receive buffer
static uint32_t s_jump_address;                           // Address to jump to for the application
static pFunction s_jump_to_application;                   // Function pointer to the application entry point
static uint8_t s_data;                                    // Variable to store received data byte
static uint8_t s_iap_update_status = IAP_WAITING_ENABLE;  // Current state of the IAP update process

uint32_t lastTime = 0;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief Compute the CRC32 checksum for the firmware.
 * @note This function calculates the CRC32 checksum for the firmware located
 *       at the APPLICATION_ADDRESS and compares it with the expected CRC
 *       value stored at FW_CRC_ADDR. It returns 1 if the CRC matches,
 *       otherwise returns 0.
 *
 * @retval 1 if the calculated CRC matches the stored CRC, 0 otherwise.
 */
uint8_t compute_fw_crc32(void)
{
    uint32_t crcsum, crc_read, len;                     // Define variables for CRC calculation
    uint8_t *ps_data = (uint8_t *)APPLICATION_ADDRESS;  // Pointer to the firmware data
    len              = FW_LENGTH - 4;                   // Calculate the length of the data, excluding the CRC part
    crc_read         = *(uint32_t *)FW_CRC_ADDR;        // Read the stored CRC value
    crcsum           = HAL_CRC_Calculate(&hcrc, (uint32_t *)ps_data, len) ^ 0xFFFFFFFF;  // Calculate CRC and invert it
    if (crc_read == crcsum) {  // Compare the calculated CRC with the read CRC
        return 1;              // If they match, return 1
    } else {
        return 0;  // If they don't match, return 0
    }
}

/**
 * @brief Send a character over UART2.
 * @note This function waits for the transmit data register to be empty
 *       and then sends a character. It also waits for the transmission
 *       to complete before returning.
 *
 * @param ch The character to send via UART.
 * @retval None
 */
void uart2_ll_send_char(char ch)
{
    while (!LL_USART_IsActiveFlag_TXE(USART2)) {  // Wait until the transmit data register is empty
    }
    LL_USART_TransmitData8(USART2, ch);          // Send the character
    while (!LL_USART_IsActiveFlag_TC(USART2)) {  // Wait for the transmission to complete
    }
}

/**
 * @brief Reset all peripherals.
 * @note This function deinitializes USART2, disables it, and clears all
 *       interrupt flags and configuration registers to their reset values.
 *       It also disables the SysTick timer and clears the SRAM parity
 *       error flag.
 *
 * @param None
 * @retval None
 */
void reset_all_periph(void)
{
    LL_USART_DeInit(USART2);                 // Deinitialize USART2
    LL_USART_Disable(USART2);                // Disable USART2
    LL_USART_DisableIT_RXNE(USART2);         // Disable receive interrupt
    SysTick->CTRL = 0;                       // Disable the SysTick timer
    SYSCFG->CFGR1 &= SYSCFG_CFGR1_MEM_MODE;  // Reset SYSCFG configuration register
    NVIC->ICER[0] = 0xFFFFFFFF;              // Clear all interrupt enable bits
    NVIC->ICPR[0] = 0xFFFFFFFF;              // Clear all interrupt pending bits
    /* Set EXTICRx registers to reset value */
    EXTI->EXTICR[0] = 0;  // Reset external interrupt configuration register
    EXTI->EXTICR[1] = 0;  // Reset external interrupt configuration register
    EXTI->EXTICR[2] = 0;  // Reset external interrupt configuration register
    EXTI->EXTICR[3] = 0;  // Reset external interrupt configuration register
    /* Set CFGR2 register to reset value: clear SRAM parity error flag */
    SYSCFG->CFGR2 |= (uint32_t)SYSCFG_CFGR2_SRAM_PE;  // Clear the SRAM parity error flag
    LL_RCC_DeInit();                                  // Deinitialize the clock
    HAL_DeInit();                                     // Deinitialize the HAL library
}

/**
 * @brief Jump to the user application.
 * @note This function checks the firmware integrity and application address
 *       validity before jumping to the user application located at
 *       APPLICATION_ADDRESS. It initializes the stack pointer and
 *       executes the application.
 *
 * @param None
 * @retval None
 */
void jump_app(void)
{
    // Check if the firmware CRC32 is valid
    if (compute_fw_crc32()) {
        /* Check if the application address context is available */
        if (((*(__IO uint32_t *)APPLICATION_ADDRESS) & 0x2FFE0000) == 0x20000000) {
            // Reset all peripherals
            reset_all_periph();

            // Read the application entry point address
            s_jump_address        = *(__IO uint32_t *)(APPLICATION_ADDRESS + 4);
            s_jump_to_application = (pFunction)s_jump_address;

            // Initialize user application's Stack Pointer
            __set_MSP(*(__IO uint32_t *)APPLICATION_ADDRESS);

            // Jump to the user application
            s_jump_to_application();
        }
    }
}

/**
 * @brief USART2 Interrupt Handler.
 * @note This function handles the USART2 interrupt. It processes incoming
 *       data based on the current status of the IAP (In-Application Programming)
 *       and manages the reception of data packets.
 *
 * @param None
 * @retval None
 */
void USART2_IRQHandler(void)
{
    /* USER CODE BEGIN UART2_IRQn 0 */
    // Check if data is available in the receive register
    if (LL_USART_IsActiveFlag_RXNE(USART2)) {
        // Read the received data
        s_data = LL_USART_ReceiveData8(USART2);

        switch (s_iap_update_status) {
            case IAP_WAITING_ENABLE:  // Waiting for enable command
                if (s_data == RECEIVE_START) {
                    // If start command is received, change status
                    lastTime            = HAL_GetTick() + DELAY_TIME * 1000;
                    s_iap_update_status = IAP_RECEIVING_DATA;
                    uart2_ll_send_char(RECEIVE_ENABLE);  // Send enable response
                } else {
                    // Send failure response for invalid command
                    uart2_ll_send_char(RECEIVE_FAIL);
                }
                break;

            case IAP_RECEIVING_DATA:                    // Receiving data state
                s_receive_buffer[s_indexs++] = s_data;  // Store received data

                // Validate the first byte of the received packet
                if (s_indexs == 1) {
                    if (s_receive_buffer[0] != PACKET_DATA_CONTINUE && s_receive_buffer[0] != PACKET_DATA_END) {
                        s_indexs = 0;                      // Reset index if invalid
                        uart2_ll_send_char(RECEIVE_FAIL);  // Send failure message
                    }
                }

                // Check if a complete packet is received
                if (s_indexs == UART_RX_LEN) {
                    if (s_receive_buffer[0] == PACKET_DATA_CONTINUE) {
                        s_iap_update_status = IAP_RECEIVE_COMPLETE;  // Mark as complete
                    } else if (s_receive_buffer[0] == PACKET_DATA_END) {
                        s_iap_update_status = IAP_UPGRADE_COMPLETE;  // Mark upgrade as complete
                    }
                    s_indexs = 0;  // Reset index for the next packet
                }
                break;

            default:
                break;
        }
    }
    /* USER CODE END UART2_IRQn 0 */
    /* USER CODE BEGIN UART2_IRQn 1 */

    /* USER CODE END UART2_IRQn 1 */
}

/**
 * @brief Get the flash memory page based on the given address.
 * @note This function calculates and returns the page number for
 *       a specific address within the flash memory, based on the
 *       starting address of the first page and the page size.
 *
 * @param Addr The address in flash memory.
 * @retval The page number corresponding to the address.
 */
static uint32_t get_page(uint32_t Addr)
{
    // Calculate and return the page number
    return (Addr - STM32G0xx_FLASH_PAGE0_STARTADDR) / FLASH_PAGE_SIZE;
}

/**
 * @brief Write code to flash memory based on received data.
 * @note This function handles unlocking the flash, erasing the
 *       appropriate pages, and writing data received in the
 *       `s_receive_buffer` to flash memory.
 * @param None
 * @retval None
 */
void write_code(void)
{
    uint16_t Number_Bytes_Transferred = 0;  // Number of bytes to be transferred
    uint32_t Add_Flash, end_add_flash;      // Start and end addresses for data transfer
    uint64_t Data       = 0;                // Data to be written (8 bytes at a time)
    uint16_t Data_index = 8;                // Data index, starts at 8 (data starts from s_receive_buffer[8])
    uint32_t Page_error = 0;                // Used to record the address of any erase error
    FLASH_EraseInitTypeDef my_flash;        // Structure to configure flash erase parameters
    uint32_t PageNum = 0;                   // Page number of the target address

    // Parse the received buffer for the flash address
    Add_Flash = s_receive_buffer[1] << 24 | s_receive_buffer[2] << 16 | s_receive_buffer[3] << 8 | s_receive_buffer[4];

    // Calculate the end address for data transfer
    end_add_flash = Add_Flash + 2048;

    // Prevent out-of-bound access: ensure the address is not below the application start address
    if (Add_Flash < APPLICATION_ADDRESS) {
        return;
    }

    // Get the number of bytes to be transferred
    Number_Bytes_Transferred = (s_receive_buffer[5] << 8) + s_receive_buffer[6];

    // If there are bytes to transfer, proceed to write
    if (Number_Bytes_Transferred > 0) {
        FLASH_WaitForLastOperation(50);  // Wait for last operation to complete

    // Unlock flash for writing
    step_unlock:
        if (HAL_FLASH_Unlock() != HAL_OK) {
            goto step_unlock;  // Retry unlocking if it fails
        }

        // Get the page number for the target address
        PageNum = get_page(Add_Flash);

        // Configure flash erase parameters
        my_flash.TypeErase = FLASH_TYPEERASE_PAGES;  // Erase by pages
        my_flash.Page      = PageNum;                // Page to erase
        my_flash.NbPages   = 1;                      // Number of pages to erase

    // Erase the flash page
    step_erase:
        FLASH_WaitForLastOperation(50);  // Wait for last operation to complete
        if (HAL_FLASHEx_Erase(&my_flash, &Page_error) != HAL_OK) {
            goto step_erase;  // Retry erase if it fails
        }

        // Loop to write data
        while (Add_Flash < end_add_flash) {
            // Assemble 64-bit data from the receive buffer
            Data = s_receive_buffer[Data_index] | ((uint64_t)s_receive_buffer[Data_index + 1] << 8) |
                   ((uint64_t)s_receive_buffer[Data_index + 2] << 16) |
                   ((uint64_t)s_receive_buffer[Data_index + 3] << 24) |
                   ((uint64_t)s_receive_buffer[Data_index + 4] << 32) |
                   ((uint64_t)s_receive_buffer[Data_index + 5] << 40) |
                   ((uint64_t)s_receive_buffer[Data_index + 6] << 48) |
                   ((uint64_t)s_receive_buffer[Data_index + 7] << 56);

        // Write data to flash
        step_write:
            FLASH_WaitForLastOperation(50);  // Wait for last operation to complete
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Add_Flash, Data) != HAL_OK) {
                goto step_write;  // Retry writing if it fails
            }

            // Update the address and data index
            Add_Flash += 8;   // Increment address by 8 bytes
            Data_index += 8;  // Increment data index by 8 bytes
        }

        // Lock the flash after writing is complete
        HAL_FLASH_Lock();
        FLASH_WaitForLastOperation(50);  // Wait for last operation to complete
    }

    // Clear the receive buffer
    memset(s_receive_buffer, 0, sizeof(s_receive_buffer));  // Clear the buffer
}

/**
 * @brief IAP USART communication handler.
 * @note This function continuously checks the IAP update status
 *       and processes data accordingly. It handles received data
 *       completion and upgrade completion, sending appropriate
 *       responses via USART.
 * @param None
 * @retval None
 */
void iap_usart(void)
{
    lastTime = HAL_GetTick() + DELAY_TIME;

    while (1) {
        if (HAL_GetTick() > lastTime) {
            jump_app();
        }

        switch (s_iap_update_status) {
            case IAP_RECEIVE_COMPLETE:
                lastTime = HAL_GetTick() + DELAY_TIME * 1000;
                // Disable USART RX interrupt after receiving complete data,
                // write the code, then re-enable
                LL_USART_DisableIT_RXNE(USART2);
                write_code();
                LL_USART_EnableIT_RXNE(USART2);
                uart2_ll_send_char(RECEIVE_SUCCESS);       // Send success response
                s_iap_update_status = IAP_RECEIVING_DATA;  // Update status to receiving data
                break;

            case IAP_UPGRADE_COMPLETE:
                lastTime = HAL_GetTick() + DELAY_TIME * 1000;
                // After upgrade completion, send success signal,
                // disable USART RX interrupt, write code, and prepare to jump
                uart2_ll_send_char(RECEIVE_SUCCESS);
                LL_USART_DisableIT_RXNE(USART2);
                write_code();
                LL_USART_EnableIT_RXNE(USART2);
                uart2_ll_send_char(RECEIVE_ACCOMPLISH);  // Send completion signal
                LL_mDelay(10);                           // Delay for 10ms
                jump_app();                              // Jump to the application program
                break;

            default:
                break;
        }
    }
}

/**
 * @brief Initialize GPIO for IAP.
 * @note This function configures the GPIO pins for input with
 *       pull-up resistors, which are used for IAP operations.
 *
 * @param None
 * @retval None
 */
void iap_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable the GPIO port clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure GPIO pins : PB6 and PB7 */
    GPIO_InitStruct.Pin  = GPIO_PIN_2 | GPIO_PIN_3;  // Configure pins PB6 and PB7
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;          // Set mode to input
    GPIO_InitStruct.Pull = GPIO_PULLUP;              // Enable pull-up mode
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);          // Initialize GPIOB with the defined settings
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_CRC_Init();
    //  MX_USART2_UART_Init();
    /* USER CODE BEGIN 2 */
    iap_gpio_init();
    LL_mDelay(200);
    if ((!(!!(GPIOA->IDR & GPIO_PIN_2))) && (!(!!(GPIOA->IDR & GPIO_PIN_3)))) {
        MX_USART2_UART_Init();
        LL_USART_EnableIT_RXNE(USART2);  // 使能串口接收中断
        iap_usart();
    } else {
        jump_app();
    }
    MX_USART2_UART_Init();
    LL_USART_EnableIT_RXNE(USART2);  // 使能串口接收中断
    iap_usart();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2) {
    }

    /* HSI configuration and activation */
    LL_RCC_HSI_Enable();
    while (LL_RCC_HSI_IsReady() != 1) {
    }

    /* Main PLL configuration and activation */
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 8, LL_RCC_PLLR_DIV_2);
    LL_RCC_PLL_Enable();
    LL_RCC_PLL_EnableDomain_SYS();
    while (LL_RCC_PLL_IsReady() != 1) {
    }

    /* Set AHB prescaler*/
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

    /* Sysclk activation on the main PLL */
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
    }

    /* Set APB1 prescaler*/
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
    LL_SetSystemCoreClock(64000000);

    /* Update the time base */
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
