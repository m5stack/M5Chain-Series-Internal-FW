#include "pir_function.h"

extern RGB_Color_TypeDef event_overlay;
extern __IO uint8_t fade_type;
extern uint8_t fade_frames_left;
extern __IO uint8_t fade_flag;

static uint8_t s_ir_induction_value = 0;
static uint8_t s_auto_send_status   = 1;
static uint32_t s_trigger_keep_time = 0;
;
static bool s_leave_trigger_keep = false;
/**
 * @brief Get the current ir induction value.
 * @note This function retrieves the current clockwise status and sends it back through a chain command.
 *
 * @param None
 * @retval None
 */
void chain_pir_get_ir_induction(void)
{
    chain_command_complete_return(CHAIN_PIR_GET_IR, (uint8_t *)&s_ir_induction_value, 1);
}

/**
 * @brief Handle PIR trigger event.
 * @note This function handles the PIR trigger event, combines ir induction value and report type code.
 * @param None
 * @retval None
 */
void handle_pir_triggle_event(void)
{
    uint8_t return_buf[2] = {s_ir_induction_value, CHAIN_PIR_REPORT_TYPE};
    chain_command_complete_return(CHAIN_PIR_AUTO_SEND_IR, return_buf, 2);
}

// Rising edge handler
static void handle_rising_edge(void)
{
    // PIR Detect someone --> Green
    fade_type        = 2;  // Green
    fade_frames_left = RGB_FADE_FRAMES;
    event_overlay.R  = 0;
    event_overlay.G  = RGB_FADE_MAX;
    event_overlay.B  = 0;
}

// Falling edge handler
static void handle_falling_edge(void)
{
    // PIR Detect nobody --> RED
    fade_type        = 1;  // Red
    fade_frames_left = RGB_FADE_FRAMES;
    event_overlay.R  = RGB_FADE_MAX;
    event_overlay.G  = 0;
    event_overlay.B  = 0;
}

void pir_status_update(void)
{
    static GPIO_PinState last_stable_value = GPIO_PIN_RESET;
    static GPIO_PinState last_read_value   = GPIO_PIN_RESET;
    static uint32_t last_change_time       = 0;
    static bool debouncing                 = false;
    static uint32_t start_time             = 0;
    static bool timer_active               = false;

    s_trigger_keep_time = g_trigger_keep_seconds * 1000;

    GPIO_PinState current_value = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    uint32_t current_time       = HAL_GetTick();

    // Signal change detected, start or reset debounce
    if (current_value != last_read_value) {
        debouncing       = true;
        last_change_time = current_time;
        last_read_value  = current_value;
        return;  // Waiting for the signal to stabilize
    }

    // During the debounce period, the signal remains stable.
    if (debouncing) {
        if (current_time - last_change_time >= 50) {
            // Debouncing completed, check if there is really a state change
            if (current_value != last_stable_value) {
                // Rising Edge: 0 -> 1
                if (current_value == GPIO_PIN_SET && last_stable_value == GPIO_PIN_RESET) {
                    handle_rising_edge();
                    // Update stable state
                    last_stable_value    = current_value;
                    s_ir_induction_value = current_value;
                    // Reset timer on rising edge
                    timer_active = false;

                    // Handling Trigger Event
                    if (s_auto_send_status) {
                        handle_pir_triggle_event();
                    }
                }
                // Falling Edge: 1 -> 0
                else if (current_value == GPIO_PIN_RESET && last_stable_value == GPIO_PIN_SET) {
                    // Update stable state
                    last_stable_value    = current_value;
                    s_ir_induction_value = current_value;
                    // Start timer on falling edge
                    start_time   = HAL_GetTick();
                    timer_active = true;
                }
            }
            debouncing = false;
        }
    }

    // Check if timer has expired
    if (timer_active == true) {
        if (HAL_GetTick() - start_time >= s_trigger_keep_time) {
            handle_falling_edge();
            s_ir_induction_value = GPIO_PIN_RESET;

            // Handling Trigger Event
            if (s_auto_send_status) {
                handle_pir_triggle_event();
            }
            timer_active = false;
        }
    }
}

void chain_pir_set_auto_send_status(uint8_t status)
{
    uint8_t operation_status = 1;
    if (status > 1) {
        operation_status = 0;
    } else {
        s_auto_send_status = status;
    }
    chain_command_complete_return(CHAIN_PIR_SET_AUTO_SEND_IR_STATUS, (uint8_t *)&operation_status, 1);
}

void chain_pir_get_auto_send_status(void)
{
    chain_command_complete_return(CHAIN_PIR_GET_AUTO_SEND_IR_STATUS, (uint8_t *)&s_auto_send_status, 1);
}

void chain_pir_set_trigger_keep_seconds(uint8_t seconds, uint8_t save_to_flash)
{
    uint8_t operation_status = 1;
    if (seconds > 255)  // limit 0~60 seconds
    {
        operation_status = 0;
    } else {
        s_leave_trigger_keep   = true;
        g_trigger_keep_seconds = seconds;
        if (save_to_flash) {
            operation_status = set_trigger_keep_seconds(g_trigger_keep_seconds) ? 1 : 0;
        }
    }
    chain_command_complete_return(CHAIN_SET_TRIGGER_DELAY, (uint8_t *)&operation_status, 1);
}

void chain_pir_get_trigger_delay_times(void)
{
    chain_command_complete_return(CHAIN_GET_TRIGGER_DELAY, (uint8_t *)&g_trigger_keep_seconds, 1);
}
