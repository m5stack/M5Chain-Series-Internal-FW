#include "base_function.h"

static uint8_t s_send_buf[BUFFER_SIZE] = {0};  // Data packet buffer
static uint8_t s_send_len              = 0;    // Length of the data to be sent

/**
 * @brief Add a packet header to the send buffer.
 * @note This function adds the high and low bytes of the packet header
 *       to the send buffer, incrementing the send length accordingly.
 *
 * @param None
 * @retval None
 */
void add_pack_head(void)
{
    s_send_buf[s_send_len++] = PACK_HEAD_HIGH;  // Add high byte of header
    s_send_buf[s_send_len++] = PACK_HEAD_LOW;   // Add low byte of header
}

/**
 * @brief Add a packet footer to the send buffer.
 * @note This function adds the high and low bytes of the packet footer
 *       to the send buffer, incrementing the send length accordingly.
 *
 * @param None
 * @retval None
 */
void add_pack_end(void)
{
    s_send_buf[s_send_len++] = PACK_END_HIGH;  // Add high byte of footer
    s_send_buf[s_send_len++] = PACK_END_LOW;   // Add low byte of footer
}

/**
 * @brief Check the packet header and footer.
 * @note This function verifies if the first two bytes of the buffer
 *       match the packet header and the last two bytes match the footer.
 *
 * @param buffer Pointer to the data buffer to check.
 * @param size   Size of the buffer.
 * @retval uint8_t CHECK_PASS if header and footer are correct,
 *         otherwise CHECK_ERROR.
 */
uint8_t head_end_check(uint8_t *buffer, uint16_t size)
{
    // Check if the first two bytes are the header and the last two bytes are the footer
    if (buffer[0] == PACK_HEAD_HIGH && buffer[1] == PACK_HEAD_LOW && buffer[size - 1] == PACK_END_LOW &&
        buffer[size - 2] == PACK_END_HIGH) {
        return CHECK_PASS;  // Return success if checks pass
    }
    return CHECK_ERROR;  // Return error if checks fail
}

/**
 * @brief Check the CRC of the data packet.
 * @note This function calculates the CRC for the data portion of the packet
 *       and compares it to the CRC value provided in the packet to ensure
 *       data integrity.
 *
 * @param buffer Pointer to the data buffer to check.
 * @param size   Size of the buffer.
 * @retval uint8_t CHECK_PASS if CRC is valid,
 *         otherwise CHECK_ERROR.
 */
uint8_t crc_check(uint8_t *buffer, uint16_t size)
{
    uint8_t crc  = buffer[size - 3];  // Extract the CRC from the packet
    uint8_t temp = 0;

    // Calculate CRC for the data (excluding header, footer, and CRC)
    for (uint16_t i = 0; i < (((buffer[3] << 8) | buffer[2]) - 1); i++) {
        temp += buffer[i + 4];  // Start after header, offset by 4
    }

    // Compare calculated CRC with the provided CRC
    if (crc == temp) {
        return CHECK_PASS;  // CRC matches
    }
    return CHECK_ERROR;  // CRC does not match
}

/**
 * @brief  Check the integrity of the data packet.
 * @note   This function verifies if the given data packet is valid by checking
 *         the packet header, footer, and CRC. It returns CHECK_PASS if the
 *         packet is valid, otherwise CHECK_ERROR.
 *
 * @param  buffer Pointer to the data buffer to check.
 * @param  size   Size of the buffer.
 * @retval uint8_t CHECK_PASS if the packet is valid, otherwise CHECK_ERROR.
 */
uint8_t pack_check(uint8_t *buffer, uint16_t size)
{
    // Ensure the buffer is large enough to contain a valid packet
    if (size <= 4) {
        return CHECK_ERROR;  // Packet is too small
    }

    // Check the packet header/footer and CRC
    if (head_end_check(buffer, size) == CHECK_PASS && crc_check(buffer, size) == CHECK_PASS) {
        return CHECK_PASS;  // Packet is valid
    }
    return CHECK_ERROR;  // Packet is invalid
}

/**
 * @brief  Construct a new CRC for the data packet.
 * @note   This function calculates the CRC by summing all bytes in the
 *         provided buffer and returns the calculated CRC value.
 *
 * @param  buffer Pointer to the data buffer to calculate CRC for.
 * @param  size   Size of the buffer.
 * @retval uint8_t The calculated CRC value.
 */
uint8_t crc_construct(uint8_t *buffer, uint16_t size)
{
    uint8_t temp = 0;

    // Calculate the CRC by summing all bytes in the buffer
    for (uint16_t i = 0; i < size; i++) {
        temp += buffer[i];
    }

    return temp;  // Return the calculated CRC
}

/**
 * @brief  Forward data from the slave side.
 * @note   This function sends the given data buffer using USART1 in DMA mode.
 *
 * @param  buffer Data buffer.
 * @param  size   Size of the buffer.
 * @retval None
 */
void uart_in_send(uint8_t *buffer, uint16_t size)
{
    usart1_transmit_dma(buffer, size);  // Send data using USART1 in DMA mode
}

/**
 * @brief  Forward data from the master side.
 * @note   This function sends the given data buffer using USART2 in DMA mode.
 *
 * @param  buffer Data buffer.
 * @param  size   Size of the buffer.
 * @retval None
 */
void uart_out_send(uint8_t *buffer, uint16_t size)
{
    usart2_transmit_dma(buffer, size);  // Send data using USART2 in DMA mode
}

/**
 * @brief  Query the bootloader version
 * @note   This function retrieves the current bootloader version and sends
 *         it back to the requester. It uses the command
 *         CHAIN_GET_BOOTLOADER_VERSION for the response.
 *
 * @param  None
 * @retval None
 */
void chain_get_bootloader_version_handle(void)
{
    // Return the bootloader version
    chain_command_complete_return(CHAIN_GET_BOOTLOADER_VERSION, (uint8_t *)&g_bootloader_version, 1);
}

/**
 * @brief  Query the firmware version
 * @note   This function retrieves the current firmware version and sends
 *         it back to the requester. It uses the command
 *         CHAIN_GET_VERSION_DEVICE for the response.
 *
 * @param  None
 * @retval None
 */
void chain_get_firmware_version_handle(void)
{
    // Return the firmware version
    chain_command_complete_return(CHAIN_GET_VERSION_DEVICE, (uint8_t *)&g_firmware_version, 1);
}

/**
 * @brief  Query the device type
 * @note   This function retrieves the type of the device and sends
 *         it back to the requester. It uses the command
 *         CHAIN_GET_DEVICE_TYPE for the response.
 *
 * @param  None
 * @retval None
 */
void chain_get_device_type_handle(void)
{
    // Return the device type
    chain_command_complete_return(CHAIN_GET_DEVICE_TYPE, (uint8_t *)&g_device_type, 2);
}

/**
 * @brief Handle enumeration packet
 * @note  This function processes an enumeration packet by incrementing
 *        the device count in the buffer and updating the CRC. It then
 *        forwards the buffer to either the slave or master based on
 *        the device's tail status.
 *
 * @param  buffer Data buffer
 * @param  size   Size of the buffer
 * @retval None
 */
void chain_enum_handle(uint8_t *buffer, uint16_t size)
{
    // Increment device count
    buffer[6]++;

    // Update CRC (assuming this is a simple checksum for demonstration)
    buffer[7]++;

    // Check if the current device is a tail device
    if (g_tail_status == CHAIN_TAIL_DEVICE) {  // If it is a tail device
        uart_in_send(buffer, size);            // Forward to the slave
    } else {                                   // If it is not a tail device
        uart_out_send(buffer, size);           // Forward to the master
    }
}

/**
 * @brief  Handles enumeration return packets
 * @note   This function forwards the incoming enumeration return packets
 *         to the input UART for processing.
 *
 * @param  buffer Data buffer
 * @param  size   Size of the buffer
 * @retval None
 */
void chain_enum_return_handle(uint8_t *buffer, uint16_t size)
{
    uart_in_send(buffer, size);  // Send the incoming buffer
}

/**
 * @brief  Handles enumeration request packets
 * @note   This function processes incoming enumeration request packets and
 *         forwards them to the input UART for further handling.
 *
 * @param  buffer Data buffer
 * @param  size   Size of the buffer
 * @retval None
 */
void chain_enum_please_handle(uint8_t *buffer, uint16_t size)
{
    uart_in_send(buffer, size);  // Send the incoming buffer
}

/**
 * @brief  Handles incoming heartbeat packets
 * @note   This function sends the current heartbeat data buffer to the input
 *         UART, which may be used to notify other devices of the system's
 *         status.
 *
 * @param  None
 * @retval None
 */
void chain_heartbeat_in_receive_handle(void)
{
    uart_in_send(g_heartbeat_data_pack_buf, sizeof(g_heartbeat_data_pack_buf));  // Send heartbeat data
}

/**
 * @brief  Handles outgoing heartbeat packets
 * @note   This function updates the heartbeat reply status buffer to indicate
 *         that a heartbeat reply has been received. The status is cyclically
 *         updated using the index.
 *
 * @param  None
 * @retval None
 */
void chain_heartbeat_out_receive_handle(void)
{
    g_heartbeat_reply_status_buf[(g_reply_index + 2) % 3] = 1;  // Update the heartbeat reply status
}

/**
 * @brief  Handles data packet processing
 * @note   This function processes incoming data packets when the command status
 *         is idle. It resets the command size and copies data from the buffer
 *         into the command buffer.
 *
 * @param  buffer Data buffer
 * @param  size   Size of the buffer
 * @retval None
 */
void chain_deal_data_packet_handle(uint8_t *buffer, uint16_t size)
{
    if (g_cmd_status == CMD_SPACE_IDLE_STATUS) {
        g_cmd_size   = 0;                      // Reset command size
        g_cmd_status = CMD_SPACE_BUSY_STATUS;  // Set command status to busy
        for (uint8_t i = 0; i < (((buffer[3] << 8) | buffer[2]) - 2); i++) {
            g_cmd_buf[g_cmd_size++] = buffer[i + 5];  // Copy data from the buffer
        }
    }
}

/**
 * @brief  Outgoing relay
 * @note   This function modifies the outgoing buffer by decrementing the index
 *         and updating the CRC before forwarding it to the lower level
 *         for processing.
 *
 * @param  buffer Data buffer
 * @param  size   Size of the buffer
 * @retval None
 */
void chain_out_relay_handle(uint8_t *buffer, uint16_t size)
{
    buffer[4]--;                  // Index decrement
    buffer[size - 3]--;           // CRC update
    uart_out_send(buffer, size);  // Forward to the lower level
}

/**
 * @brief  Incoming relay
 * @note   This function modifies the incoming buffer by incrementing the index
 *         and updating the CRC before forwarding it to the upper level
 *         for further processing.
 *
 * @param  buffer Data buffer
 * @param  size   Size of the buffer
 * @retval None
 */
void chain_in_relay_handle(uint8_t *buffer, uint16_t size)
{
    buffer[4]++;                 // Index increment
    buffer[size - 3]++;          // CRC update
    uart_in_send(buffer, size);  // Forward to the upper level
}

/**
 * @brief  Command completion return data packet
 * @note   This function constructs and sends a data packet indicating the
 *         completion of a command. It adds a header, the command identifier,
 *         the data buffer, and a CRC for data integrity.
 *
 * @param  cmd Command identifier
 * @param  buffer Data buffer
 * @param  size Size of the buffer
 * @retval None
 */
void chain_command_complete_return(uint8_t cmd, uint8_t *buffer, uint16_t size)
{
    // Clear sending length
    s_send_len = 0;
    // Calculate sending length
    uint16_t len = size + 3;
    // Add packet header
    add_pack_head();
    s_send_buf[s_send_len++] = (uint8_t)(len & 0xFF);
    s_send_buf[s_send_len++] = (uint8_t)((len >> 8) & 0xFF);
    s_send_buf[s_send_len++] = DEFAULT_INDEX;
    s_send_buf[s_send_len++] = cmd;
    for (uint8_t i = 0; i < size; i++) {
        s_send_buf[s_send_len++] = buffer[i];  // Add data from buffer
    }
    s_send_buf[s_send_len] = crc_construct((uint8_t *)(s_send_buf + 4),
                                           (s_send_len - 4));  // Calculate CRC
    s_send_len++;
    add_pack_end();
    uart_in_send(s_send_buf, s_send_len);  // Send the complete packet
}

/**
 * @brief  GPIO initialization
 * @note   This function initializes GPIO ports and pins used in the IAP (In-Application Programming) process.
 *         It enables the clock for GPIOA and configures specified pins as input with pull-up resistors.
 *
 * @param  None
 * @retval None
 */
static void iap_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure GPIO pins : PB6 PB7 */
    GPIO_InitStruct.Pin  = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;      // Configure pull-up resistors
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);  // Initialize GPIOB with the configuration
}

/**
 * @brief  Handle IAP update command
 * @note   This function processes the IAP update command. If the received
 *         data matches the expected value (0x86), it sends a command completion
 *         return packet, initializes GPIO, and checks the status of the pins
 *         for a reset condition. Depending on the pin status, it either
 *         resets the microcontroller or reinitializes the USART.
 *
 * @param  data Verification bit
 * @retval None
 */
void chain_iap_update_handle(uint8_t data)
{
    uint32_t reset_wait_start = HAL_GetTick();  // Get the current tick count
    if (data == 0x86)                           // Check for expected data
    {
        chain_command_complete_return(CHAIN_IAP_UPDATE, (uint8_t *)&data, 1);  // Send completion packet
        HAL_Delay(10);                                                         // Delay to ensure packet is sent
        LL_USART_DeInit(USART1);                                               // Deinitialize USART1
        LL_USART_Disable(USART1);                                              // Disable USART1
        LL_USART_DisableIT_RXNE(USART1);                                       // Disable USART1 RX interrupt
        iap_gpio_init();                                                       // Initialize GPIO for IAP
        // Wait for reset conditions on GPIO pins
        while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) || HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)) {
            if (HAL_GetTick() - reset_wait_start > 1000)  // Check timeout
            {
                break;  // Exit if timeout
            }
        }
        NVIC_SystemReset();  // Reset the microcontroller again
    }
}

/**
 * @brief  Handle the Get UID command
 * @note   Returns a portion of the device UID based on the provided uid_type.
 *         - If uid_type is 0: returns the first 4 bytes of the UID.
 *         - If uid_type is 1: returns the first 12 bytes of the UID.
 *         - Any other uid_type will result in a failure response and no UID data.
 *
 * @param  uid_type UID data segment selector:
 *         - 0: Return g_uid[0..3] (4 bytes)
 *         - 1: Return g_uid[0..11] (12 bytes)
 *
 * @retval None
 */
void chain_get_uid_handle(uint8_t uid_type)
{
    uint8_t s_ret_buf[16]    = {0};                // Response buffer
    uint8_t s_ret_buf_size   = 0;                  // Size of the response buffer
    uint8_t operation_result = OPERATION_SUCCESS;  // Default result is success

    // Check if uid_type is valid (only 0 or 1 are allowed)
    if (uid_type > 1) {
        operation_result = OPERATION_FAIL;
    }

    // First byte in response: operation result
    s_ret_buf[s_ret_buf_size++] = operation_result;

    // If request is valid, append the requested UID segment
    if (operation_result == OPERATION_SUCCESS) {
        if (uid_type == 0) {
            // Copy first 4 bytes of UID
            for (uint8_t i = 0; i < 4; i++) {
                s_ret_buf[s_ret_buf_size++] = g_uid[i];
            }
        } else if (uid_type == 1) {
            // Copy first 12 bytes of UID
            for (uint8_t i = 0; i < 12; i++) {
                s_ret_buf[s_ret_buf_size++] = g_uid[i];
            }
        }
    }

    // Send the response packet containing the result and UID data
    chain_command_complete_return(CHAIN_GET_UID, s_ret_buf, s_ret_buf_size);
}
