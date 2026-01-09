/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    usart.c
 * @brief   This file provides code for the configuration
 *          of the USART instances.
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
#include "usart.h"

/* USER CODE BEGIN 0 */

__IO circular_buffer tx_in_buffer = {0};
__IO circular_buffer tx_out_buffer = {0};

/* USER CODE END 0 */

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the peripherals clocks
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
  /**USART1 GPIO Configuration
  PB6   ------> USART1_TX
  PB7   ------> USART1_RX
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USART1 DMA Init */

  /* USART1_RX Init */
  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_1, LL_DMAMUX_REQ_USART1_RX);

  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_BYTE);

  /* USART1_TX Init */
  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_2, LL_DMAMUX_REQ_USART1_TX);

  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_2, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MDATAALIGN_BYTE);

  /* USART1 interrupt Init */
  NVIC_SetPriority(USART1_IRQn, 0);
  NVIC_EnableIRQ(USART1_IRQn);

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
  USART_InitStruct.BaudRate = 115200;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART1, &USART_InitStruct);
  LL_USART_SetTXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
  LL_USART_SetRXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
  LL_USART_DisableFIFO(USART1);
  LL_USART_ConfigAsyncMode(USART1);

  /* USER CODE BEGIN WKUPType USART1 */

  /* USER CODE END WKUPType USART1 */

  LL_USART_Enable(USART1);

  /* Polling USART1 initialisation */
  while((!(LL_USART_IsActiveFlag_TEACK(USART1))) || (!(LL_USART_IsActiveFlag_REACK(USART1))))
  {
  }
  /* USER CODE BEGIN USART1_Init 2 */
  usart1_hart_init();
  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  /**USART2 GPIO Configuration
  PA2   ------> USART2_TX
  PA3   ------> USART2_RX
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USART2 DMA Init */

  /* USART2_RX Init */
  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_3, LL_DMAMUX_REQ_USART2_RX);

  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_3, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MDATAALIGN_BYTE);

  /* USART2_TX Init */
  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_4, LL_DMAMUX_REQ_USART2_TX);

  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_4, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_4, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_4, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_4, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_4, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_4, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_4, LL_DMA_MDATAALIGN_BYTE);

  /* USART2 interrupt Init */
  NVIC_SetPriority(USART2_IRQn, 0);
  NVIC_EnableIRQ(USART2_IRQn);

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
  USART_InitStruct.BaudRate = 115200;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART2, &USART_InitStruct);
  LL_USART_ConfigAsyncMode(USART2);

  /* USER CODE BEGIN WKUPType USART2 */

  /* USER CODE END WKUPType USART2 */

  LL_USART_Enable(USART2);

  /* Polling USART2 initialisation */
  while((!(LL_USART_IsActiveFlag_TEACK(USART2))) || (!(LL_USART_IsActiveFlag_REACK(USART2))))
  {
  }
  /* USER CODE BEGIN USART2_Init 2 */
  usart2_hart_init();
  /* USER CODE END USART2_Init 2 */

}

/* USER CODE BEGIN 1 */
/**
 * @brief Initializes USART1 for bidirectional communication using DMA.
 * @note This function configures USART1 to operate in both transmit (TX) and
 *       receive (RX) modes. It sets up the DMA channels for data transfer,
 *       enables idle line interrupts to detect the end of data reception,
 *       and configures necessary settings for efficient data transmission and
 * reception.
 *
 * @param  None
 * @retval None
 */
void usart1_hart_init(void) {
  // Set USART1 to operate in TX and RX mode
  LL_USART_SetTransferDirection(USART1, LL_USART_DIRECTION_TX_RX);

  // Enable idle line interrupt for USART1 to detect when data reception is
  // complete
  LL_USART_EnableIT_IDLE(USART1);

  /* Configure DMA for USART1 RX */
  // Set the peripheral address for DMA RX to the USART1 data register
  LL_DMA_SetPeriphAddress(
      DMA1, LL_DMA_CHANNEL_1,
      LL_USART_DMA_GetRegAddr(USART1, LL_USART_DMA_REG_DATA_RECEIVE));
  // Set the memory address where received data will be stored
  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1,
                          (uint32_t)g_uart_in_rx_buf[g_uart_in_rx_index]);
  // Set the amount of data to be received
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, BUFFER_SIZE);

  /* Enable DMA transfer complete and transfer error interrupts for RX */
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);
  LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_1);

  /* Start the DMA channel for USART1 RX and enable USART RX DMA */
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
  LL_USART_EnableDMAReq_RX(USART1);
  // Clear any existing idle line flags
  LL_USART_ClearFlag_IDLE(USART1);

  /* Configure DMA for USART1 TX */
  // Set the peripheral address for DMA TX to the USART1 data register
  LL_DMA_SetPeriphAddress(
      DMA1, LL_DMA_CHANNEL_2,
      LL_USART_DMA_GetRegAddr(USART1, LL_USART_DMA_REG_DATA_TRANSMIT));
  // Enable USART1 TX DMA request for data transmission
  LL_USART_EnableDMAReq_TX(USART1);
  // Enable DMA transfer complete interrupt for USART1 TX
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_2);
}

/**
 * @brief Initializes USART2 for bidirectional communication using DMA.
 * @note This function configures USART2 to operate in both transmit (TX) and
 *       receive (RX) modes. It sets up the DMA channels for data transfer,
 *       enables idle line interrupts to signal the end of data reception,
 *       and configures necessary settings for effective data transmission and
 * reception.
 *
 * @param  None
 * @retval None
 */
void usart2_hart_init(void) {
  // Set USART2 to operate in TX and RX mode
  LL_USART_SetTransferDirection(USART2, LL_USART_DIRECTION_TX_RX);

  // Enable idle line interrupt for USART2 to detect the end of data reception
  LL_USART_EnableIT_IDLE(USART2);

  /* Configure DMA for USART2 RX */
  // Set the peripheral address for DMA RX to the USART2 data register
  LL_DMA_SetPeriphAddress(
      DMA1, LL_DMA_CHANNEL_3,
      LL_USART_DMA_GetRegAddr(USART2, LL_USART_DMA_REG_DATA_RECEIVE));
  // Set the memory address for storing the received data
  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_3,
                          (uint32_t)g_uart_out_rx_buf[g_uart_out_rx_index]);
  // Set the amount of data to be received
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, BUFFER_SIZE);

  /* Enable DMA transfer complete and transfer error interrupts for RX */
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_3);
  LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_3);

  /* Start the DMA channel for USART2 RX and enable USART RX DMA */
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
  LL_USART_EnableDMAReq_RX(USART2);
  // Clear any existing idle line flags
  LL_USART_ClearFlag_IDLE(USART2);

  /* Configure DMA for USART2 TX */
  // Set the peripheral address for DMA TX to the USART2 data register
  LL_DMA_SetPeriphAddress(
      DMA1, LL_DMA_CHANNEL_4,
      LL_USART_DMA_GetRegAddr(USART2, LL_USART_DMA_REG_DATA_TRANSMIT));
  // Enable USART2 TX DMA request for data transmission
  LL_USART_EnableDMAReq_TX(USART2);
  // Enable DMA transfer complete interrupt for USART2 TX
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_4);
}

/**
 * @brief Queue data for DMA transmission on USART1.
 *
 * This function always enqueues the data first.
 * If the DMA is idle, it immediately starts transmission.
 * If the queue is full, the packet is discarded.
 *
 * @param buf Pointer to the data buffer.
 * @param size Number of bytes to transmit.
 */
void usart1_transmit_dma(uint8_t *buf, uint16_t size)
{
    __disable_irq();  // Enter critical section

    if (tx_in_buffer.packet_count < MAX_QUEUE_SIZE && size <= BUFFER_SIZE) {
        // Copy data into the queue
        memcpy(tx_in_buffer.send_queue[tx_in_buffer.tail].data, buf, size);
        tx_in_buffer.send_queue[tx_in_buffer.tail].length = size;

        // Advance tail and increment count
        tx_in_buffer.tail = (tx_in_buffer.tail + 1) % MAX_QUEUE_SIZE;
        tx_in_buffer.packet_count++;

        // If DMA is idle, start transmission
        if (g_uart_in_transmit_complete) {
            uint8_t index = tx_in_buffer.head;
            uint16_t len  = tx_in_buffer.send_queue[index].length;

            g_uart_in_transmit_complete = 0;

            LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t)tx_in_buffer.send_queue[index].data);
            LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, len);
            LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
        }
    } else {
        // Queue full, discard the packet
        // Optional: increment a counter or log the event
    }

    __enable_irq();  // Exit critical section
}

/**
 * @brief Queue data for DMA transmission on USART2.
 *
 * This function always enqueues the data first.
 * If the DMA is idle, it immediately starts transmission.
 * If the queue is full, the packet is discarded.
 *
 * @param buf Pointer to the data buffer.
 * @param size Number of bytes to transmit.
 */
void usart2_transmit_dma(uint8_t *buf, uint16_t size)
{
    __disable_irq();  // Enter critical section

    if (tx_out_buffer.packet_count < MAX_QUEUE_SIZE && size <= BUFFER_SIZE) {
        // Copy data into the queue
        memcpy(tx_out_buffer.send_queue[tx_out_buffer.tail].data, buf, size);
        tx_out_buffer.send_queue[tx_out_buffer.tail].length = size;

        // Advance tail and increment count
        tx_out_buffer.tail = (tx_out_buffer.tail + 1) % MAX_QUEUE_SIZE;
        tx_out_buffer.packet_count++;

        // If DMA is idle, start transmission
        if (g_uart_out_transmit_complete) {
            uint8_t index = tx_out_buffer.head;
            uint16_t len  = tx_out_buffer.send_queue[index].length;

            g_uart_out_transmit_complete = 0;

            LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)tx_out_buffer.send_queue[index].data);
            LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, len);
            LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
        }
    } else {
        // Queue full, discard the packet
        // Optional: increment a counter or log the event
    }

    __enable_irq();  // Exit critical section
}

/* USER CODE END 1 */
