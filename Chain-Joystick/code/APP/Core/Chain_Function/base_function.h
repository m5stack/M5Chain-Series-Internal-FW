/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BASE_FUNCTION_H_
#define BASE_FUNCTION_H_

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"

#define CHECK_PASS        (1)     // Check passed
#define CHECK_ERROR       (0)     // Check not passed
#define SOFTWARE_VERSION  (0x01)  // Software version
#define PRODUCT_TYPE_HIGH (0)     // Define product type
#define PRODUCT_TYPE_LOW  (4)     // Define product type
#define PACK_HEAD_HIGH    (0xAA)  // Packet header
#define PACK_HEAD_LOW     (0x55)  // Packet header
#define PACK_END_HIGH     (0x55)  // Packet tail
#define PACK_END_LOW      (0xAA)  // Packet tail
#define DEFAULT_INDEX     (0x01)  // Default index

typedef enum {
    CHAIN_GET_UID                = 0xF8,  // Query unique identifier
    CHAIN_GET_BOOTLOADER_VERSION = 0xF9,  // Query bootloader version number
    CHAIN_GET_VERSION_DEVICE     = 0xFA,  // Query device software version number
    CHAIN_GET_DEVICE_TYPE        = 0xFB,  // Query device type
    CHAIN_ENUM_PLEASE            = 0xFC,  // Enumeration request
    CHAIN_HEARTBEAT              = 0xFD,  // Heartbeat packet
    CHAIN_ENUM                   = 0xFE,  // Enumeration
    CHAIN_IAP_UPDATE             = 0xFF,  // Upgrade, not open to external
} chain_base_cmd_t;                       // Command type

typedef enum {
    ERROR_DATA_PACK,              // Error data packet
    CHAIN_HEARTBEAT_DATA_PACK,    // Heartbeat data packet
    CHAIN_ENUM_DATA_PACK,         // Enumeration data packet
    CHAIN_ENUM_RETURN_DATA_PACK,  // Enumeration return data packet
    CHAIN_TRANSMIT_DATA_PACK,     // Transmit data packet (forwarded to other devices)
    CHAIN_HANDLE_DATA_PACK,       // Handle data packet (local data packet)
    CHAIN_ENUM_PLEASE_DATA_PACK,  // Enumeration request data packet
} data_pack_t;                    // Data packet type

typedef enum {
    OPERATION_FAIL    = 0x00,  // Operation failed
    OPERATION_SUCCESS = 0x01,  // Operation successful
} operation_t;                 // Operation status

extern __IO uint8_t g_uart_in_transmit_commplete;   // UART in transmission complete flag
extern __IO uint8_t g_uart_out_transmit_commplete;  // UART in transmission complete flag
extern __IO uint8_t g_cmd_buf[BUFFER_SIZE];         // Command buffer
extern __IO uint8_t g_cmd_size;                     // Buffer size, default 0
extern __IO uint8_t g_cmd_status;                   // Command status, default idle
extern __IO uint8_t g_tail_status;                  // Tail device marker
extern __IO uint8_t g_heart_beat_record;            // Heartbeat packet reception record
extern __IO uint8_t g_bootloader_version;           // IAP version number
extern __IO uint8_t g_firmware_version;             // Software version number
extern __IO uint16_t g_device_type;                 // Device type
extern __IO uint8_t g_uid[12];                      // Unique identifier
extern uint8_t g_heartbeat_data_pack_buf[9];        // Heartbeat packet
extern uint8_t g_heartbeat_reply_status_buf[3];     // Reply status
extern uint8_t g_reply_index;                       // Reply status index

void add_pack_head(void);                                            // Add packet header
void add_pack_end(void);                                             // Add packet tail
uint8_t head_end_check(uint8_t *buffer, uint16_t size);              // Check packet header and tail
uint8_t crc_check(uint8_t *buffer, uint16_t size);                   // Check CRC
uint8_t pack_check(uint8_t *buffer, uint16_t size);                  // Check data packet
uint8_t crc_construct(uint8_t *buffer, uint16_t size);               // Generate new CRC for forwarded packet
void uart_in_send(uint8_t *buffer, uint16_t size);                   // Forward data from slave
void uart_out_send(uint8_t *buffer, uint16_t size);                  // Forward data to host
void chain_get_bootloader_version_handle(void);                      // Query bootloader version number
void chain_get_firmware_version_handle(void);                        // Query device software version number
void chain_get_device_type_handle(void);                             // Query device type
void chain_enum_handle(uint8_t *buffer, uint16_t size);              // Handle enumeration packet
void chain_enum_return_handle(uint8_t *buffer, uint16_t size);       // Handle enumeration return packet
void chain_enum_please_handle(uint8_t *buffer, uint16_t size);       // Handle enumeration request packet
void chain_heartbeat_in_receive_handle(void);                        // Handle incoming heartbeat packet
void chain_heartbeat_out_receive_handle(void);                       // Handle outgoing heartbeat packet
void chain_deal_data_packet_handle(uint8_t *buffer, uint16_t size);  // Handle data packet
void chain_out_relay_handle(uint8_t *buffer, uint16_t size);         // Outgoing relay
void chain_in_relay_handle(uint8_t *buffer, uint16_t size);          // Incoming relay
void chain_command_complete_return(uint8_t cmd, uint8_t *buffer,
                                   uint16_t size);  // Return data packet for command completion
void chain_iap_update_handle(uint8_t data);         //  Handle IAP update command
void chain_get_uid_handle(uint8_t uid_type);        // Query unique identifier

#ifdef __cplusplus
}
#endif

#endif /* BASE_FUNCTION_H_ */
