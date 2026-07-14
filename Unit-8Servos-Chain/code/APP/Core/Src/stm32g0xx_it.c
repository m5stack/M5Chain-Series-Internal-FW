/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32g0xx_it.c
 * @brief   Interrupt Service Routines.
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
#include "stm32g0xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "string.h"
#include "base_function.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern __IO uint8_t g_uart_in_rx_buf[UART_BUFFER_SIZE][BUFFER_SIZE * 2];
extern __IO uint8_t g_uart_out_rx_buf[UART_BUFFER_SIZE][BUFFER_SIZE * 2];
extern __IO uint8_t g_uart_in_rx_index;
extern __IO uint8_t g_uart_out_rx_index;
extern __IO uint8_t g_uart_in_transmit_complete;
extern __IO uint8_t g_uart_out_transmit_complete;
extern __IO uint8_t g_cmd_buf[BUFFER_SIZE];
extern __IO uint8_t g_cmd_size;
extern __IO uint8_t g_cmd_status;
extern __IO uint8_t g_tail_status;
extern __IO uint8_t g_heart_beat_record;
extern __IO uint32_t g_last_received_time;
extern __IO uint8_t g_send_complete_flag;

uint8_t g_enum_please_nums                      = ENUM_PLEASE_NUM;
static uint8_t s_enum_please_data_packet_buf[9] = {0xAA, 0x55, 0x03, 0x00, 0xFF, 0xFC, 0xFB, 0x55, 0xAA};
uint8_t g_heartbeat_data_pack_buf[9]            = {0xAA, 0x55, 0x03, 0x00, 0xFF, 0xFD, 0xFC, 0x55, 0xAA};
uint8_t g_heartbeat_reply_status_buf[3]         = {0};
uint8_t g_reply_index                           = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
extern __IO circular_buffer tx_in_buffer;
extern __IO circular_buffer tx_out_buffer;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M0+ Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
    while (1) {
    }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVC_IRQn 0 */

  /* USER CODE END SVC_IRQn 0 */
  /* USER CODE BEGIN SVC_IRQn 1 */

  /* USER CODE END SVC_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32G0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g0xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles Flash global interrupt.
  */
void FLASH_IRQHandler(void)
{
  /* USER CODE BEGIN FLASH_IRQn 0 */

  /* USER CODE END FLASH_IRQn 0 */
  HAL_FLASH_IRQHandler();
  /* USER CODE BEGIN FLASH_IRQn 1 */

  /* USER CODE END FLASH_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel 1 interrupt.
  */
void DMA1_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */

  /* USER CODE END DMA1_Channel1_IRQn 0 */

  /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */

  /* USER CODE END DMA1_Channel1_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel 2 and channel 3 interrupts.
  */
void DMA1_Channel2_3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel2_3_IRQn 0 */
    // Check if the DMA Channel 2 transfer complete flag is set
    if (LL_DMA_IsActiveFlag_TC2(DMA1)) {
        // Clear the transfer complete flag for DMA Channel 2
        LL_DMA_ClearFlag_TC2(DMA1);
        // Disable the DMA Channel to stop any further transmission
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);

        __disable_irq();  // Enter critical section

        // Remove the completed packet from the queue
        if (tx_in_buffer.packet_count > 0) {
            tx_in_buffer.head = (tx_in_buffer.head + 1) % MAX_QUEUE_SIZE;
            tx_in_buffer.packet_count--;
        }

        // If more packets are available, start the next DMA transfer
        if (tx_in_buffer.packet_count > 0) {
            uint8_t index   = tx_in_buffer.head;
            uint16_t length = tx_in_buffer.send_queue[index].length;

            LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t)tx_in_buffer.send_queue[index].data);
            LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, length);
            LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);

            g_uart_in_transmit_complete = 0;  // Mark transmission as ongoing
        } else {
            g_uart_in_transmit_complete = 1;  // Mark transmission as complete
        }

        __enable_irq();  // Exit critical section
    }
  /* USER CODE END DMA1_Channel2_3_IRQn 0 */

  /* USER CODE BEGIN DMA1_Channel2_3_IRQn 1 */

  /* USER CODE END DMA1_Channel2_3_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel 4, channel 5 and DMAMUX1 interrupts.
  */
void DMA1_Ch4_5_DMAMUX1_OVR_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Ch4_5_DMAMUX1_OVR_IRQn 0 */

    // Check if the DMA Channel 4 transfer complete flag is set
    if (LL_DMA_IsActiveFlag_TC4(DMA1)) {
        // Clear the transfer complete flag for DMA Channel 4
        LL_DMA_ClearFlag_TC4(DMA1);
        // Disable the DMA Channel to stop any further transmission
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);

        __disable_irq();  // Enter critical section

        // Remove the completed packet from the queue
        if (tx_out_buffer.packet_count > 0) {
            tx_out_buffer.head = (tx_out_buffer.head + 1) % MAX_QUEUE_SIZE;
            tx_out_buffer.packet_count--;
        }

        // If more packets are available, start the next DMA transfer
        if (tx_out_buffer.packet_count > 0) {
            uint8_t index   = tx_out_buffer.head;
            uint16_t length = tx_out_buffer.send_queue[index].length;

            LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)tx_out_buffer.send_queue[index].data);
            LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, length);
            LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);

            g_uart_out_transmit_complete = 0;  // Mark transmission as ongoing
        } else {
            g_uart_out_transmit_complete = 1;  // Mark transmission as complete
        }

        __enable_irq();  // Exit critical section
    }
  /* USER CODE END DMA1_Ch4_5_DMAMUX1_OVR_IRQn 0 */

  /* USER CODE BEGIN DMA1_Ch4_5_DMAMUX1_OVR_IRQn 1 */

  /* USER CODE END DMA1_Ch4_5_DMAMUX1_OVR_IRQn 1 */
}

/**
  * @brief This function handles TIM14 global interrupt.
  */
void TIM14_IRQHandler(void)
{
  /* USER CODE BEGIN TIM14_IRQn 0 */
    if (LL_TIM_IsActiveFlag_UPDATE(TIM14) == SET) {
        LL_TIM_ClearFlag_UPDATE(TIM14);  // Clear the update interrupt flag for TIM14

        // Decrement the count for the "enum please" packets to send
        g_enum_please_nums--;

        // Trigger a DMA transmission for the "enum please" data packet
        usart1_transmit_dma(s_enum_please_data_packet_buf, sizeof(s_enum_please_data_packet_buf));

        // Check if the number of packets to send has reached zero
        if (g_enum_please_nums == 0) {
            LL_TIM_DisableIT_UPDATE(TIM14);  // Disable update interrupt for TIM14
            LL_TIM_DisableCounter(TIM14);    // Stop TIM14 counter
        }
    }
  /* USER CODE END TIM14_IRQn 0 */
  /* USER CODE BEGIN TIM14_IRQn 1 */

  /* USER CODE END TIM14_IRQn 1 */
}

/**
  * @brief This function handles TIM16 global interrupt.
  */
void TIM16_IRQHandler(void)
{
  /* USER CODE BEGIN TIM16_IRQn 0 */
    // Check if the update interrupt flag is set for TIM16
    if (LL_TIM_IsActiveFlag_UPDATE(TIM16) == SET) {
        LL_TIM_ClearFlag_UPDATE(TIM16);  // Clear the update interrupt flag for TIM16

        // Check if the device is currently not the tail in the chain
        if (g_tail_status == CHAIN_NON_TAIL_DEVICE) {
            // Update the tail status to indicate that this device is now the tail
            g_tail_status = CHAIN_TAIL_DEVICE;

            // Enable TIM14 to start sending "enum please" packets
            LL_TIM_EnableIT_UPDATE(TIM14);  // Enable update interrupt for TIM14
            LL_TIM_EnableCounter(TIM14);    // Start the TIM14 counter

            // Set the number of "enum please" packets to send
            g_enum_please_nums = ENUM_PLEASE_NUM;
        }
    }
  /* USER CODE END TIM16_IRQn 0 */
  /* USER CODE BEGIN TIM16_IRQn 1 */

  /* USER CODE END TIM16_IRQn 1 */
}

/**
  * @brief This function handles TIM17 global interrupt.
  */
void TIM17_IRQHandler(void)
{
  /* USER CODE BEGIN TIM17_IRQn 0 */
    if (LL_TIM_IsActiveFlag_UPDATE(TIM17) == SET) {
        LL_TIM_ClearFlag_UPDATE(TIM17);  // Clear the update interrupt flag for TIM17

        // Transmit the heartbeat data packet using USART2 with DMA
        usart2_transmit_dma(g_heartbeat_data_pack_buf, sizeof(g_heartbeat_data_pack_buf));
    }
  /* USER CODE END TIM17_IRQn 0 */
  /* USER CODE BEGIN TIM17_IRQn 1 */

  /* USER CODE END TIM17_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt / USART1 wake-up interrupt through EXTI line 25.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

    // Check if USART1 idle flag is active and idle interrupt is enabled
    if (LL_USART_IsActiveFlag_IDLE(USART1) && LL_USART_IsEnabledIT_IDLE(USART1)) {
        LL_USART_ClearFlag_IDLE(USART1);                // Clear the idle flag
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);  // Temporarily disable DMA channel for RX

        // Calculate received data length
        uint16_t s_len = BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_1);

        // Prevent buffer overflow by checking if buffer is full
        if (s_len == 0) {
            // If overflow occurs, discard the current data
            LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);  // Re-enable DMA reception
            return;
        }

        // Save current buffer index for processing and update to next index for DMA
        uint8_t current_index = g_uart_in_rx_index;
        g_uart_in_rx_index    = (g_uart_in_rx_index + 1) % UART_BUFFER_SIZE;

        // Set new buffer address and size for next DMA transfer
        LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)g_uart_in_rx_buf[g_uart_in_rx_index]);
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, BUFFER_SIZE);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);  // Re-enable DMA
        LL_USART_EnableDMAReq_RX(USART1);              // Re-enable DMA request for USART1 RX

        if (s_len < MIN_PACKET_SIZE) return;

        const uint8_t *incoming_buffer = g_uart_in_rx_buf[current_index];
        const uint8_t *end             = incoming_buffer + s_len;
        const uint8_t *p               = incoming_buffer;

        while (p <= end - MIN_PACKET_SIZE) {
            // check header
            if (p[0] != PACK_HEAD_HIGH || p[1] != PACK_HEAD_LOW) {
                p++;
                continue;
            }

            // get payload length (little-endian)
            uint16_t payload_len  = p[2] | ((uint16_t)p[3] << 8);
            uint16_t total_length = 6 + payload_len;  // header(2) + len(2) + payload + tail(2)

            // payload length exception protection
            if (payload_len > (BUFFER_SIZE - 6) || total_length < MIN_PACKET_SIZE) {
                p++;
                continue;
            }

            // check if remaining data is enough for this packet
            if (p + total_length > end) {
                break;  // data is not enough for one packet, wait for next time
            }

            // check packet tail
            if (p[total_length - 2] != PACK_END_HIGH || p[total_length - 1] != PACK_END_LOW) {
                p++;
                continue;
            }

            // handle valid data packet
            uint8_t packet_type = p[5];

            switch (packet_type) {
                case CHAIN_ENUM:
                    chain_enum_handle(p, total_length);
                    break;

                case CHAIN_HEARTBEAT:
                    chain_heartbeat_in_receive_handle();
                    break;

                default:
                    if (p[4] != 1) {
                        chain_out_relay_handle(p, total_length);
                    } else if (pack_check(p, total_length) == CHECK_PASS) {
                        chain_deal_data_packet_handle(p, total_length);
                    }
                    break;
            }

            // Move to next packet
            p += total_length;
        }
    }

  /* USER CODE END USART1_IRQn 0 */
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles USART2 global interrupt / USART2 wake-up interrupt through EXTI line 26.
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */

    // Check if USART2 idle flag is active and idle interrupt is enabled
    if (LL_USART_IsActiveFlag_IDLE(USART2) && LL_USART_IsEnabledIT_IDLE(USART2)) {
        // Clear idle interrupt flag
        LL_USART_ClearFlag_IDLE(USART2);

        // Temporarily disable DMA channel for USART2 RX
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);

        // Calculate length of received data
        uint16_t s_len = BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_3);

        // Prevent buffer overflow by checking if buffer is full
        if (s_len == 0) {
            // If overflow occurs, discard current data and re-enable DMA
            LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
            return;
        }

        // Save current buffer index for processing and update to next index for DMA
        uint8_t current_index = g_uart_out_rx_index;
        g_uart_out_rx_index   = (g_uart_out_rx_index + 1) % UART_BUFFER_SIZE;

        // Set new DMA receive buffer address and size
        LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)g_uart_out_rx_buf[g_uart_out_rx_index]);
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, BUFFER_SIZE);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);  // Re-enable DMA

        // Re-enable USART2 DMA request for RX
        LL_USART_EnableDMAReq_RX(USART2);

        // Process received data if length is greater than 0
        if (s_len < MIN_PACKET_SIZE) return;

        const uint8_t *incoming_buffer = g_uart_out_rx_buf[current_index];
        const uint8_t *end             = incoming_buffer + s_len;
        const uint8_t *p               = incoming_buffer;

        // Parse incoming buffer data
        while (p <= end - MIN_PACKET_SIZE) {
            // Check for packet start signature (0xAA, 0x55)
            if (p[0] != PACK_HEAD_HIGH || p[1] != PACK_HEAD_LOW) {
                p++;
                continue;
            }

            // Get packet length (little-endian)
            uint16_t packet_length = p[2] | ((uint16_t)p[3] << 8);
            uint16_t total_length  = 6 + packet_length;  // header(2) + len(2) + payload + tail(2)

            // Validate packet length to avoid out-of-bounds access
            if (packet_length > BUFFER_SIZE || total_length < MIN_PACKET_SIZE) {
                p++;
                continue;
            }

            // Check if we have enough data for complete packet
            if (p + total_length > end) {
                break;  // Not enough data, wait for next reception
            }

            // Check for packet end signature (0x55, 0xAA)
            if (p[total_length - 2] != PACK_END_HIGH || p[total_length - 1] != PACK_END_LOW) {
                p++;
                continue;
            }

            // Extract and handle packet based on type
            uint8_t packet_type = p[5];

            switch (packet_type) {
                case CHAIN_HEARTBEAT:

                    break;

                case CHAIN_ENUM:
                    chain_enum_return_handle(p, total_length);
                    break;

                case CHAIN_ENUM_PLEASE:
                    chain_enum_please_handle(p, total_length);
                    break;

                default:
                    // Handle other packets (relay data)
                    chain_in_relay_handle(p, total_length);
                    break;
            }
            // Reset counter and update tail status
            LL_TIM_SetCounter(TIM16, 0);
            g_tail_status = CHAIN_NON_TAIL_DEVICE;
            p += total_length;  // Move to the next packet
        }
    }

  /* USER CODE END USART2_IRQn 0 */
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
