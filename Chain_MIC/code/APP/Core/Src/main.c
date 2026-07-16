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
#include "adc.h"
#include "dma.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mic_function.h"
#include "base_function.h"
#include "RGB.h"
#include "myflash.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// DMA receive buffers
__IO uint8_t g_uart_in_rx_buf[UART_BUFFER_SIZE][BUFFER_SIZE * 2];   // uart_in receive buffer for DMA
__IO uint8_t g_uart_out_rx_buf[UART_BUFFER_SIZE][BUFFER_SIZE * 2];  // uart_out receive buffer for DMA

// Transmission status for UART
__IO uint8_t g_uart_in_rx_index  = 0;  // Index for UART input (receiving) buffer position
__IO uint8_t g_uart_out_rx_index = 0;  // Index for UART output (receiving) buffer position

// Transmission complete flags for UART
__IO uint8_t g_uart_in_transmit_complete  = 1;  // Flag indicating if uart_in transmission is complete (1: complete)
__IO uint8_t g_uart_out_transmit_complete = 1;  // Flag indicating if uart_out transmission is complete (1: complete)

// Command buffer and related variables
__IO uint8_t g_cmd_buf[BUFFER_SIZE] = {0};                    // Command buffer initialized to zero
__IO uint8_t g_cmd_size             = 0;                      // Size of the command currently in the buffer
__IO uint8_t g_cmd_status           = CMD_SPACE_IDLE_STATUS;  // Status of the command processing

// Device status variables
__IO uint8_t g_tail_status        = CHAIN_TAIL_DEVICE;  // Status of the tail device in the chain
__IO uint8_t g_heart_beat_record  = 0;                  // Heartbeat record for monitoring device status
__IO uint8_t g_bootloader_version = 0;                  // Version of the bootloader
__IO uint8_t g_firmware_version   = SOFTWARE_VERSION;   // Version of the firmware

// Device type information
__IO uint16_t g_device_type =
    (uint8_t)((PRODUCT_TYPE_HIGH << 8) | PRODUCT_TYPE_LOW);  // Combined product type from high and low bytes

__IO uint16_t g_threshold = 0;

__IO uint8_t g_light = 0;  // Light status or value

__IO static uint32_t s_last_press_time = 0;  // Store the last press time in milliseconds

__IO static uint8_t s_key_flag = 0;

__IO uint8_t g_uid[12] = {0};  // Unique identifier

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief Set the Interrupt Vector Table for the application in SRAM.
 * @param None
 * @retval None
 */
void iap_set(void)
{
    uint8_t i;                                     // Loop index
    uint32_t *pVecTab = (uint32_t *)(0x20000000);  // Pointer to the vector table in SRAM

    // Copy the interrupt vector table from the application address to SRAM
    for (i = 0; i < 48; i++) {
        // Copy each vector entry to the SRAM vector table
        *(pVecTab++) = *(__IO uint32_t *)(APPLICATION_ADDRESS + (i << 2));
    }

    // Enable the SYSCFG peripheral clock
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    // Remap the memory to use SRAM for the vector table
    __HAL_SYSCFG_REMAPMEMORY_SRAM();
}

/**
 * @brief Initialize the chain device settings.
 * @param None
 * @retval None
 */
void chain_init(void)
{
    // Get the version of the bootloader
    g_bootloader_version = get_bootloader_version();

    if (get_threshold_value() == 0xFFFF) {
        // Set the threshold to a base value if it is maximum
        g_threshold = THRESHOLD_BASE;
        set_threshold_value(THRESHOLD_BASE);
    } else {
        // Otherwise, get the current threshold value
        g_threshold = get_threshold_value();
    }

    // Check if the RGB light setting is set to maximum (0xFF)
    if (get_rgb_light() == 0xFF) {
        // Set the light to a base color if it is maximum
        g_light = RGB_LIGHT_BASE;
        set_rgb_light(g_light);
    } else {
        // Otherwise, get the current RGB light setting
        g_light = get_rgb_light();
    }
}

void read_uid(void)
{
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();

    memcpy(&g_uid[0], &uid0, 4);
    memcpy(&g_uid[4], &uid1, 4);
    memcpy(&g_uid[8], &uid2, 4);
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */
    iap_set();
    chain_init();
    read_uid();
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
    MX_DMA_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_IWDG_Init();
    MX_TIM14_Init();
    MX_TIM1_Init();
    MX_TIM3_Init();
    MX_TIM16_Init();
    MX_TIM17_Init();
    MX_ADC1_Init();
    /* USER CODE BEGIN 2 */
    rgb_init();
    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_Start_IT(&hadc1);
    LL_TIM_EnableCounter(TIM3);
    HAL_Delay(1);
    LL_TIM_ClearFlag_UPDATE(TIM14);  // Clear update TIM14
    LL_TIM_EnableIT_UPDATE(TIM14);   // ENABLE TIM14
    LL_TIM_EnableCounter(TIM14);     // ENABLE TIM14
    HAL_Delay(1);
    LL_TIM_ClearFlag_UPDATE(TIM17);  // Clear update TIM17
    LL_TIM_EnableIT_UPDATE(TIM17);   // ENABLE TIM17
    LL_TIM_EnableCounter(TIM17);     // ENABLE TIM17
    HAL_Delay(1);
    LL_TIM_ClearFlag_UPDATE(TIM16);  // Clear update TIM16
    LL_TIM_EnableIT_UPDATE(TIM16);   // ENABLE TIM16
    LL_TIM_EnableCounter(TIM16);     // ENABLE TIM16
    HAL_Delay(1);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        if (g_cmd_status == CMD_SPACE_BUSY_STATUS) {
            switch (g_cmd_buf[0]) {
                case CHAIN_SET_RGB_VALUE:
                    chain_set_rgb_value((uint8_t *)(g_cmd_buf + 1), (g_cmd_size - 1));
                    break;
                case CHAIN_GET_UID:
                    chain_get_uid_handle(g_cmd_buf[1]);
                    break;
                case CHAIN_GET_RGB_VALUE:
                    chain_get_rgb_value((uint8_t *)(g_cmd_buf + 1), (g_cmd_size - 1));
                    break;
                case CHAIN_SET_RGB_LIGHT:
                    chain_set_light_value(g_cmd_buf[1], g_cmd_buf[2]);
                    break;
                case CHAIN_GET_RGB_LIGHT:
                    chain_get_light_value();
                    break;
                case CHAIN_MIC_GET_12ADC:
                    chain_mic_get_12value();
                    break;
                case CHAIN_MIC_GET_8ADC:
                    chain_mic_get_8value();
                    break;
                case CHAIN_MIC_SET_THRESHOLD_VALUE:
                    chain_mic_set_threshold_value((uint8_t *)(g_cmd_buf + 1), (g_cmd_size - 1));
                    break;
                case CHAIN_MIC_GET_THRESHOLD_VALUE:
                    chain_mic_get_threshold_value();
                    break;
                case CHAIN_MIC_SET_REPORT_MODE:
                    chain_mic_set_report_mode((uint8_t *)(g_cmd_buf + 1), (g_cmd_size - 1));
                    break;
                case CHAIN_MIC_GET_REPORT_MODE:
                    chain_mic_get_report_mode();
                    break;
                case CHAIN_MIC_SET_TRIGGER_INTERVAL:
                    chain_mic_set_trigger_interval((uint8_t *)(g_cmd_buf + 1), (g_cmd_size - 1));
                    break;
                case CHAIN_MIC_GET_TRIGGER_INTERVAL:
                    chain_mic_get_trigger_interval();
                    break;
                case CHAIN_GET_BOOTLOADER_VERSION:
                    chain_get_bootloader_version_handle();
                    break;
                case CHAIN_GET_VERSION_DEVICE:
                    chain_get_firmware_version_handle();
                    break;
                case CHAIN_GET_DEVICE_TYPE:
                    chain_get_device_type_handle();
                    break;
                case CHAIN_IAP_UPDATE:
                    chain_iap_update_handle(g_cmd_buf[1]);
                    break;
                default:
                    break;
            }
            g_cmd_status = CMD_SPACE_IDLE_STATUS;
        }
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        chain_mic_trigger();
        rgb_update();
        LL_IWDG_ReloadCounter(IWDG);
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv              = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.LSIState            = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN            = 8;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV4;
    RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
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
    /* User can add his own implementation to report the file name and line
       number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
