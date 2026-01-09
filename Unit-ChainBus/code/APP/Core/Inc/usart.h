/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define MAX_QUEUE_SIZE (3) // Maximum number of packets in the send queue

// Structure to hold a single data packet
typedef struct {
  uint8_t data[BUFFER_SIZE]; // Buffer to store the data of the packet
  uint16_t length;           // Length of the data packet
} data_packet_buf;

// Structure for a circular buffer to manage a queue of data packets
typedef struct {
  data_packet_buf send_queue[MAX_QUEUE_SIZE]; // Array of data packets in the queue
  uint8_t head;                              // Index to the head of the queue (next packet to send)
  uint8_t tail;                              // Index to the tail of the queue (last added packet)
  uint8_t packet_count;                      // Current number of packets in the queue
} circular_buffer;
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
// DMA receive buffers
extern __IO uint8_t g_uart_in_rx_buf[UART_BUFFER_SIZE][BUFFER_SIZE*2]; // uart_in data buffer for
                                                      // receiving data via DMA
extern __IO uint8_t g_uart_out_rx_buf[UART_BUFFER_SIZE][BUFFER_SIZE*2]; // uart_out data buffer
                                                       // for receiving data via
                                                       // DMA
extern __IO uint8_t g_uart_in_rx_index;   // Index for UART input (receiving) buffer position
extern __IO uint8_t g_uart_out_rx_index;  // Index for UART output (receiving) buffer position

// Serial transmission complete flags
extern __IO uint8_t
    g_uart_in_transmit_complete; // Serial 1 transmission complete flag
extern __IO uint8_t
    g_uart_out_transmit_complete; // Serial 2 transmission complete flag

// Function declarations
void usart1_hart_init(void); // Initialize hardware configuration for USART1
void usart2_hart_init(void); // Initialize hardware configuration for USART2
void usart1_transmit_dma(uint8_t *data,
                         uint16_t size); // Transmit data to USART1 using DMA
void usart2_transmit_dma(uint8_t *data,
                         uint16_t size); // Transmit data to USART2 using DMA

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */
