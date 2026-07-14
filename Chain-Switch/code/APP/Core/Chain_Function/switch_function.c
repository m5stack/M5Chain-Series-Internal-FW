#include <switch_function.h>

extern RGB_Color_TypeDef event_overlay;
extern __IO uint8_t fade_type;
extern __IO uint8_t fade_frames_left;
extern __IO uint8_t fade_flag;
extern __IO uint16_t s_switch_threshold_open;
extern __IO uint16_t s_switch_threshold_close;
extern __IO uint8_t fade_start_flag;

#define BOUNDARY_THRESHOLD      10   // gradient boundary
#define SLIP_BOUNDARY_THRESHOLD 100  // slip gradient boundary

// Switch ADC Value
static uint16_t s_switch_adc_12value = 0;  // Variable to hold the latest 12-bit ADC value for switch measurement
static uint8_t s_switch_adc_8value   = 0;  // Variable to hold the latest 8-bit mapped ADC value for switch measurement
static uint8_t s_return_status =
    0;  // Variable to indicate the return status of operations (1 for success, 0 for failure)
static uint8_t s_switch_status = 0;  // the status of switch (0:close, 1:open)

static uint16_t s_last_switch_adc_12value = 0;  // the last time 12-bit ADC value for make slip gradient

static uint32_t s_slip_last_tick =
    0;  // Variable to record last slip timestamp, used to compare the s_slip_cur_timestamp
static uint32_t s_slip_cur_tick =
    0;  // Variable to record the latest slip timestamp, used to compare the s_slip_last_timestamp

static uint8_t s_gradient_status =
    NONE;  // Variable of status to judge the latest ADC value that should be at which switch_gradient_status_t
static uint8_t s_slip_gradient_status =
    NONE;  // Variable of status to judge the latest ADC value that should be at which switch_gradient_status_t
           // OPEN:middle->up  CLOSE: middle->down NONE:didn't start slip gradient  MIDDLE: executing slip gradient
static uint8_t s_auto_send_status = 1;

/**
 * @brief Handle Switch trigger event.
 * @note This function handles the Switch trigger event, combines switch status and report type code.
 * @param None
 * @retval None
 */
void handle_switch_trigger_event(void)
{
    uint8_t return_buf[2] = {s_switch_status, CHAIN_SWITCH_REPORT_TYPE};
    chain_command_complete_return(CHAIN_SWITCH_REPORT_SWITCH_STATUS, return_buf, 2);
}

/**
 * @brief Update the switch status based on the current ADC value with hysteresis.
 * @note  The function checks ADC value and sets 'g_switch_status' accordingly to determine switch state.
 *        It avoids switch shake around the threshold.
 * @param None
 * @retval None
 */
void switch_status_update(void)
{
    uint8_t is_update = 0;

    if (s_switch_adc_12value <= s_switch_threshold_close) {
        if (s_switch_status != 0) {
            s_switch_status = 0;
            is_update       = 1;
        }
    } else if (s_switch_adc_12value >= s_switch_threshold_open) {
        if (s_switch_status != 1) {
            s_switch_status = 1;
            is_update       = 1;
        }
    } else {
        if (s_switch_status != 1) {
            s_switch_status = 1;
            is_update       = 1;
        }
    }
    if (is_update && s_auto_send_status) {
        handle_switch_trigger_event();
    }
}

/**
 * @brief Set the RGB color for slip (real-time, push) gradient effect.
 * @note  This function produces a real-time color change as the ADC value moves between thresholds.
 *        If fast enough slip detected (over threshold and within time), updates color, otherwise stops effect.
 * @param None
 * @retval None
 */
void set_rgb_slip_gradient(void)
{
    //	if ((s_switch_adc_12value < s_switch_threshold_open) && (s_switch_adc_12value > s_switch_threshold_close)) {
    int32_t diff = 0;
    switch (s_slip_gradient_status) {
        case NONE:
            s_slip_cur_tick           = HAL_GetTick();
            s_slip_last_tick          = s_slip_cur_tick;
            s_last_switch_adc_12value = s_switch_adc_12value;
            s_slip_gradient_status    = MIDDLE;  // start count
            break;
        case MIDDLE:
            s_slip_cur_tick = HAL_GetTick();
            diff            = s_switch_adc_12value - s_last_switch_adc_12value;
            if ((diff > SLIP_BOUNDARY_THRESHOLD) &&
                (s_slip_cur_tick - s_slip_last_tick <= 1000)) {  // if diff > 10 and did not timeout
                //					add_color(OPEN);	// set color
                //					send_rgb_value_to_switch(0, SET_COLOR);		// show color
                fade_frames_left          = 0;
                event_overlay.R           = 0;
                event_overlay.G           = (uint8_t)(s_switch_adc_12value * 256 / 4096);
                event_overlay.B           = 0;
                fade_flag                 = 1;
                s_last_switch_adc_12value = s_switch_adc_12value;  // updata adc_value
                s_slip_last_tick          = s_slip_cur_tick;       // update timestamp
            } else if ((diff < -SLIP_BOUNDARY_THRESHOLD) && s_slip_cur_tick - s_slip_last_tick <= 1000) {
                //					add_color(CLOSE);		// set color
                //					send_rgb_value_to_switch(0, SET_COLOR);		// show color
                fade_frames_left          = 0;
                event_overlay.R           = (uint8_t)(255 - s_switch_adc_12value * 256 / 4096);
                event_overlay.G           = 0;
                event_overlay.B           = 0;
                fade_flag                 = 1;
                s_last_switch_adc_12value = s_switch_adc_12value;  // updata adc_value
                s_slip_last_tick          = s_slip_cur_tick;       // update timestamp
            } else if (s_slip_cur_tick - s_slip_last_tick > 1000) {
                s_slip_gradient_status = NONE;
                //					send_rgb_value_to_switch(0, USER_COLOR);		// show color
                fade_frames_left = 0;
                event_overlay.R  = 0;
                event_overlay.G  = 0;
                event_overlay.B  = 0;
                fade_flag        = 1;
            }
            break;
    }
    //	}
    //	else {
    //		s_slip_gradient_status = NONE;
    //	}
}

/**
 * @brief Update the current gradient effect status according to ADC value.
 * @note  Compares ADC value to thresholds, sets s_gradient_status to OPEN/MIDDLE/CLOSE/NONE.
 *        Provides basic state machine for effect handling.
 * @param None
 * @retval None
 */
void update_gradient_status()
{
Switch_Status:
    switch (s_gradient_status) {
        case NONE:
        case MIDDLE:
            if (s_switch_adc_12value >= s_switch_threshold_open) {
                s_gradient_status = OPEN;
                fade_start_flag   = 1;
            } else if (s_switch_adc_12value <= s_switch_threshold_close) {
                s_gradient_status = CLOSE;
                fade_start_flag   = 1;
            } else {
                s_gradient_status = MIDDLE;
            }
            break;
        case OPEN:
            if (s_switch_adc_12value < s_switch_threshold_open - BOUNDARY_THRESHOLD) {
                s_gradient_status = NONE;
                fade_start_flag   = 1;
                goto Switch_Status;
            }
            break;
        case CLOSE:
            if (s_switch_adc_12value > s_switch_threshold_close + BOUNDARY_THRESHOLD) {
                s_gradient_status = NONE;
                fade_start_flag   = 1;
                goto Switch_Status;
            }
            break;
    }
}

/**
 * @brief Handles the main execution of the RGB gradient effect according to the current state.
 * @note  Triggers or stops gradient animation based on s_gradient_status and related flags.
 *        Also manages displaying user's custom color once effect is complete.
 * @param None
 * @retval None
 */
void set_rgb_gradient_effect()
{
    if (s_auto_send_status) {
        if (s_gradient_status ==
            CLOSE) {  // if last status is CLOSE, adc_value must > threshold_close + 10 to avoid boundary shake
            if (fade_start_flag == 1) {
                fade_type        = 1;  // R分量
                fade_frames_left = RGB_FADE_FRAMES;
                event_overlay.R  = RGB_FADE_MAX;
                event_overlay.G  = 0;
                event_overlay.B  = 0;
            }
        } else if (s_gradient_status ==
                   OPEN) {  // if last status is OPEN, adc_value must < threshold_open - 10 to avoid boundary shake
            if (fade_start_flag == 1) {
                fade_type        = 2;  // G分量
                fade_frames_left = RGB_FADE_FRAMES;
                event_overlay.R  = 0;
                event_overlay.G  = RGB_FADE_MAX;
                event_overlay.B  = 0;
            }
        } else {
            set_rgb_slip_gradient();
        }
    }
}

/**
 * @brief Map a 12-bit ADC value to an 8-bit value.
 * @note This function scales the input value from a 12-bit range (0 to 4095)
 *       to an 8-bit range (0 to 255).
 *
 * @param input The 12-bit ADC value (0 to 4095).
 * @retval The corresponding 8-bit value (0 to 255).
 */
uint8_t map12_to8(uint16_t input)
{
    return (uint8_t)((input * 255) / 4095);  // Map 12-bit value to 8-bit value
}

/**
 * @brief Retrieve ADC values, calculate switch readings and show led gradient effects.
 * @note This function performs multiple ADC conversions to average the result.
 *       It updates the global 12-bit and 8-bit switch values based on the current
 *       clockwise status.
 *
 * @param None
 * @retval None
 */
void adc_value_update(void)
{
    uint8_t adc_convert_success_nums = 0;  ///< Counter for successful ADC conversions
    uint32_t adc_sum                 = 0;  ///< Sum of successful ADC readings

    for (uint8_t i = 0; i < 15; i++) {
        HAL_ADCEx_Calibration_Start(&hadc1);  // Start ADC calibration
        HAL_ADC_Start(&hadc1);                // Start ADC conversion
        if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK) {
            if (HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc1), HAL_ADC_STATE_REG_EOC)) {
                adc_sum += HAL_ADC_GetValue(&hadc1);  // Accumulate ADC value
                adc_convert_success_nums++;           // Increment success counter
            }
        }
        HAL_ADC_Stop(&hadc1);  // Stop ADC conversion
    }

    if (adc_convert_success_nums > 0) {
        // Calculate the average ADC value and map to 8-bit based on clockwise status
        s_switch_adc_12value = adc_sum / adc_convert_success_nums;  // Average value
        s_switch_adc_8value  = (g_slip_mode_status == CHAIN_SWITCH_DOWNUP_INC) ? map12_to8(s_switch_adc_12value)
                                                                               : (255 - map12_to8(s_switch_adc_12value));

        if (g_slip_mode_status != CHAIN_SWITCH_DOWNUP_INC) {
            s_switch_adc_12value = 4095 - s_switch_adc_12value;  // Invert 12-bit value for counter-clockwise
        }
        update_gradient_status();   // update gradient status when adc value in thresholds
        set_rgb_gradient_effect();  // show led gradient effects when adc value in thresholds
    }
}

/**
 * @brief Get the current 12-bit switch ADC value and return it.
 * @note This function retrieves the ADC value and sends it back through a chain command.
 *
 * @param None
 * @retval None
 */
void chain_switch_get_12value(void)
{
    chain_command_complete_return(CHAIN_SWITCH_GET_12ADC, (uint8_t *)&s_switch_adc_12value, 2);
}

/**
 * @brief Get the current 8-bit switch ADC value and return it.
 * @note This function retrieves the ADC value and sends it back through a chain command.
 *
 * @param None
 * @retval None
 */
void chain_switch_get_8value(void)
{
    chain_command_complete_return(CHAIN_SWITCH_GET_8ADC, (uint8_t *)&s_switch_adc_8value, 1);
}

/**
 * @brief Set the slip direction status.
 * @note This function updates the slip direction status and sends back the result of the operation.
 *
 * @param sta The desired status to set (either CHAIN_SWITCH_DOWNUP_DEC or CHAIN_SWITCH_DOWNUP_INC).
 * @param flag A flag indicating whether to save the status to memory.
 *             1 means the status will be saved to memory,
 *             0 means the status will not be saved to memory
 * @retval None
 */
void chain_switch_set_slip_mode(uint8_t sta, uint8_t flag)
{
    s_return_status = 1;
    if (sta == CHAIN_SWITCH_DOWNUP_DEC || sta == CHAIN_SWITCH_DOWNUP_INC) {
        if (flag && sta != get_slip_mode_status()) {
            set_slip_mode_status(sta);  // Persist status to flash
        }
        g_slip_mode_status = sta;  // Update the global clockwise status
        s_return_status    = 1;    // Success
    } else {
        s_return_status = 0;  // Failure
    }
    chain_command_complete_return(CHAIN_SWITCH_SET_SLIP_MODE, (uint8_t *)&s_return_status, 1);
}

/**
 * @brief Get the current clockwise direction status.
 * @note This function retrieves the current clockwise status and sends it back through a chain command.
 *
 * @param None
 * @retval None
 */
void chain_switch_get_slip_mode(void)
{
    chain_command_complete_return(CHAIN_SWITCH_GET_SLIP_MODE, (uint8_t *)&g_slip_mode_status, 1);
}

/**
 * @brief Get the current switch slip status.
 * @note This function retrieves the current switch slip status and sends it back through a chain command.
 *
 * @param None
 * @retval None
 */
void chain_switch_get_slip_status(void)
{
    chain_command_complete_return(CHAIN_SWITCH_GET_SWITCH_STATUS, (uint8_t *)&s_switch_status, 1);
}

/**
 * @brief Set the slip open&close threshold of switch.
 * @note This function updates the slip open&close threshold value and sends back the result of the operation.
 *
 * @param open_low_values  The low-byte of open-threshold.
 * @param open_high_values The high-byte of open-threshold.
 * @param close_low_values The low-byte of close-threshold.
 * @param close_high_values The high-byte of close-threshold.
 * @param flag  A flag indicating whether to save the status to memory.
 *             1 means the threshold will be saved to memory,
 *             0 means the threshold will not be saved to memory
 * @retval None
 */
void chain_switch_set_switch_threshold(uint8_t open_low_values, uint8_t open_high_values, uint8_t close_low_values,
                                       uint8_t close_high_values, uint8_t flag)
{
    s_return_status = 1;

    s_switch_threshold_open  = open_low_values + (open_high_values << 8);
    s_switch_threshold_close = close_low_values + (close_high_values << 8);

    if ((s_switch_threshold_open > 4095) || (s_switch_threshold_close > 4095) ||
        (s_switch_threshold_open <= s_switch_threshold_close)) {
        s_return_status = 0;
    }

    if (s_return_status) {
        uint32_t data     = get_slip_threshold();  // read origin threshold which is stored at flash
        uint32_t cur_data = open_low_values;
        cur_data          = (cur_data << 8) + open_high_values;
        cur_data          = (cur_data << 8) + close_low_values;
        cur_data          = (cur_data << 8) + close_high_values;

        g_threshold = cur_data;
        if (flag && (data != cur_data)) {  // write threshold into flash
            s_return_status =
                set_switch_threshold(cur_data, open_low_values, open_high_values, close_low_values, close_high_values);
        }
    }
    chain_command_complete_return(CHAIN_SWITCH_SET_SWITCH_THRESHOLD, (uint8_t *)&s_return_status, 1);
}

/**
 * @brief Get the slip open&close threshold of switch.
 * @note This function retrieves the slip open&close threshold value and sends back the result of the operation.
 *
 * @param none
 * @retval None
 */
void chain_switch_get_switch_threshold(void)
{
    uint8_t threshold_buf[4] = {0};
    int j                    = 3;
    for (int i = 0; i < 4; i++) {
        threshold_buf[i] = g_threshold >> (8 * j--);
    }
    chain_command_complete_return(CHAIN_SWITCH_GET_SWITCH_THRESHOLD, threshold_buf, 4);
}

void chain_switch_set_auto_send_status(uint8_t status)
{
    uint8_t operation_status = 0;
    if (status <= 1) {
        s_auto_send_status = status;
        operation_status   = 1;
    }
    chain_command_complete_return(CHAIN_SWITCH_SET_REPORT_SWITCH_STATUS, (uint8_t *)&operation_status, 1);
}

void chain_switch_get_auto_send_status(void)
{
    chain_command_complete_return(CHAIN_SWITCH_GET_REPORT_SWITCH_STATUS, (uint8_t *)&s_auto_send_status, 1);
}
