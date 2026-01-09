/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "tof_function.h"
#include "vl53l0x_api.h"
#include "i2c.h"

__IO static uint16_t s_tof_distance         = 0;
__IO static uint8_t s_tof_measurement_time  = 33;
__IO static chain_tof_mode_t chain_tof_mode = CHAIN_TOF_MODE_CONTINUOUS;
__IO static uint8_t s_measurement_status    = 1;
__IO static uint8_t s_operation_status      = 0;
__IO static uint8_t s_measure_complete_flag = 0;

static VL53L0X_RangingMeasurementData_t data;
static VL53L0X_Dev_t vl53l0x_c;
static VL53L0X_DEV dev = &vl53l0x_c;
static uint8_t ret     = 0;

void chain_tof_init(void)
{
    dev->I2cHandle  = &hi2c2;
    dev->I2cDevAddr = 0x52;
    VL53L0X_Initialize(dev);
    VL53L0X_StartMeasurement(dev);
}

static void get_distance(void)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    Status               = VL53L0X_GetMeasurementDataReady(dev, &ret);
    if (Status != VL53L0X_ERROR_NONE) {
        VL53L0X_ClearInterruptMask(dev, 0);
        if (chain_tof_mode == CHAIN_TOF_MODE_SINGLE && s_measurement_status == 1) {
            s_measurement_status = 0;
        }
        return;
    }

    if (ret != 1) {
        return;
    }

    Status = VL53L0X_GetRangingMeasurementData(dev, &data);
    if (Status != VL53L0X_ERROR_NONE) {
        VL53L0X_ClearInterruptMask(dev, 0);
        if (chain_tof_mode == CHAIN_TOF_MODE_SINGLE && s_measurement_status == 1) {
            s_measurement_status = 0;
        }
        return;
    }

    s_tof_distance = data.RangeMilliMeter;

    VL53L0X_ClearInterruptMask(dev, 0);

    if (chain_tof_mode == CHAIN_TOF_MODE_SINGLE && s_measurement_status == 1) {
        s_measurement_status = 0;
    }

    s_measure_complete_flag = 1;
}

void chain_tof_update_distance(void)
{
    switch (chain_tof_mode) {
        case CHAIN_TOF_MODE_STOP:
            break;

        case CHAIN_TOF_MODE_SINGLE:
            if (s_measurement_status == 1) {
                get_distance();
            }
            break;

        case CHAIN_TOF_MODE_CONTINUOUS:
            get_distance();
            break;

        default:
            break;
    }
}

void chain_tof_get_distance(void)
{
    uint16_t distance = s_tof_distance;
    chain_command_complete_return(CHAIN_TOF_GET_DISTANCE, (uint8_t *)&distance, 2);
    s_measure_complete_flag = 0;
}

void chain_tof_set_measurement_time(uint8_t time)
{
    s_operation_status = OPERATION_SUCCESS;

    if (time < 20 || time > 200) {
        s_operation_status = OPERATION_FAIL;
        chain_command_complete_return(CHAIN_TOF_SET_MEASURE_TIME, (uint8_t *)&s_operation_status, 1);
        return;
    }

    if (s_tof_measurement_time == time) {
        chain_command_complete_return(CHAIN_TOF_SET_MEASURE_TIME, (uint8_t *)&s_operation_status, 1);
        return;
    }

    VL53L0X_Error Status = VL53L0X_ERROR_NONE;

    switch (chain_tof_mode) {
        case CHAIN_TOF_MODE_STOP:
            Status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(dev, time * 1000U);
            if (Status != VL53L0X_ERROR_NONE) {
                s_operation_status = OPERATION_FAIL;
            } else {
                s_tof_measurement_time = time;
            }

            break;

        case CHAIN_TOF_MODE_SINGLE:
            if (s_measurement_status == 1) {
                Status = VL53L0X_StopMeasurement(dev);
                VL53L0X_ClearInterruptMask(dev, 0);
                if (Status == VL53L0X_ERROR_NONE) {
                    Status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(dev, time * 1000U);
                    if (Status == VL53L0X_ERROR_NONE) {
                        Status = VL53L0X_StartMeasurement(dev);
                    }
                }
            } else {
                Status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(dev, time * 1000U);
            }

            if (Status != VL53L0X_ERROR_NONE) {
                s_operation_status = OPERATION_FAIL;
            } else {
                s_tof_measurement_time = time;
            }
            break;

        case CHAIN_TOF_MODE_CONTINUOUS:
            Status = VL53L0X_StopMeasurement(dev);
            VL53L0X_ClearInterruptMask(dev, 0);
            if (Status == VL53L0X_ERROR_NONE) {
                Status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(dev, time * 1000U);
                if (Status == VL53L0X_ERROR_NONE) {
                    Status = VL53L0X_StartMeasurement(dev);
                }
            }

            if (Status != VL53L0X_ERROR_NONE) {
                s_operation_status = OPERATION_FAIL;
            } else {
                s_tof_measurement_time = time;
            }
            break;

        default:
            s_operation_status = OPERATION_FAIL;
            break;
    }

    chain_command_complete_return(CHAIN_TOF_SET_MEASURE_TIME, (uint8_t *)&s_operation_status, 1);
}

void chain_tof_get_measurement_time(void)
{
    chain_command_complete_return(CHAIN_TOF_GET_MEASURE_TIME, (uint8_t *)&s_tof_measurement_time, 1);
}

void chain_tof_set_measurement_mode(uint8_t mode)
{
    VL53L0X_Error status = VL53L0X_ERROR_NONE;
    chain_tof_mode_t old_mode;

    s_operation_status = OPERATION_SUCCESS;

    if (mode > CHAIN_TOF_MODE_CONTINUOUS) {
        s_operation_status = OPERATION_FAIL;
        chain_command_complete_return(CHAIN_TOF_SET_MEASURE_MODE, (uint8_t *)&s_operation_status, 1);
        return;
    }

    chain_tof_mode_t new_mode = (chain_tof_mode_t)mode;

    if (new_mode == chain_tof_mode) {
        chain_command_complete_return(CHAIN_TOF_SET_MEASURE_MODE, (uint8_t *)&s_operation_status, 1);
        return;
    }

    old_mode = chain_tof_mode;

    status = VL53L0X_StopMeasurement(dev);
    VL53L0X_ClearInterruptMask(dev, 0);
    if (status != VL53L0X_ERROR_NONE) {
        s_operation_status = OPERATION_FAIL;
        if (old_mode == CHAIN_TOF_MODE_CONTINUOUS) {
            VL53L0X_StartMeasurement(dev);
        }
        chain_command_complete_return(CHAIN_TOF_SET_MEASURE_MODE, (uint8_t *)&s_operation_status, 1);
        return;
    }

    switch (new_mode) {
        case CHAIN_TOF_MODE_STOP:
            s_measurement_status = 0;
            break;

        case CHAIN_TOF_MODE_SINGLE:
            status = VL53L0X_SetDeviceMode(dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
            if (status != VL53L0X_ERROR_NONE) {
                s_operation_status = OPERATION_FAIL;
            }
            s_measurement_status = 0;
            break;

        case CHAIN_TOF_MODE_CONTINUOUS:
            status = VL53L0X_SetDeviceMode(dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
            if (status != VL53L0X_ERROR_NONE) {
                s_operation_status = OPERATION_FAIL;
                break;
            }
            status = VL53L0X_StartMeasurement(dev);
            if (status != VL53L0X_ERROR_NONE) {
                s_operation_status = OPERATION_FAIL;
            }
            s_measurement_status = 1;
            break;

        default:
            s_operation_status = OPERATION_FAIL;
            break;
    }

    if (s_operation_status != OPERATION_SUCCESS) {
        switch (old_mode) {
            case CHAIN_TOF_MODE_CONTINUOUS:
                VL53L0X_SetDeviceMode(dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
                VL53L0X_StartMeasurement(dev);
                s_measurement_status = 1;
                break;
            case CHAIN_TOF_MODE_SINGLE:
                VL53L0X_SetDeviceMode(dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
                s_measurement_status = 0;
                break;
            case CHAIN_TOF_MODE_STOP:
                s_measurement_status = 0;
                break;
        }
    } else {
        chain_tof_mode = new_mode;
    }

    chain_command_complete_return(CHAIN_TOF_SET_MEASURE_MODE, (uint8_t *)&s_operation_status, 1);
}

void chain_tof_get_measurement_mode(void)
{
    chain_command_complete_return(CHAIN_TOF_GET_MEASURE_MODE, (uint8_t *)&chain_tof_mode, 1);
}

void chain_tof_set_measurement_status(uint8_t status)
{
    VL53L0X_Error ret  = VL53L0X_ERROR_NONE;
    s_operation_status = OPERATION_SUCCESS;

    if (status > 1U) {
        s_operation_status = OPERATION_FAIL;
        chain_command_complete_return(CHAIN_TOF_SET_MEASURE_STATUS, (uint8_t *)&s_operation_status, 1);
        return;
    }

    if (s_measurement_status == status) {
        chain_command_complete_return(CHAIN_TOF_SET_MEASURE_STATUS, (uint8_t *)&s_operation_status, 1);
        return;
    }

    switch (chain_tof_mode) {
        case CHAIN_TOF_MODE_STOP:
            if (status == 1U) {
                s_operation_status = OPERATION_FAIL;
            } else {
                s_measurement_status = 0U;
            }
            break;

        case CHAIN_TOF_MODE_SINGLE:
            if (status == 1U) {
                ret = VL53L0X_StartMeasurement(dev);
            } else {
                ret = VL53L0X_StopMeasurement(dev);
            }

            if (ret == VL53L0X_ERROR_NONE) {
                VL53L0X_ClearInterruptMask(dev, 0);
                s_measurement_status = status;
            } else {
                s_operation_status = OPERATION_FAIL;
            }
            break;

        case CHAIN_TOF_MODE_CONTINUOUS:
            if (status == 0U) {
                s_operation_status = OPERATION_FAIL;
            } else {
                s_measurement_status = 1U;
            }
            break;

        default:
            s_operation_status = OPERATION_FAIL;
            break;
    }

    chain_command_complete_return(CHAIN_TOF_SET_MEASURE_STATUS, (uint8_t *)&s_operation_status, 1);
}

void chain_tof_get_measurement_status(void)
{
    uint8_t measurement_status = (uint8_t)s_measurement_status;
    chain_command_complete_return(CHAIN_TOF_GET_MEASURE_STATUS, (uint8_t *)&measurement_status, 1);
}

void chain_tof_get_measurement_complete_flag(void)
{
    uint8_t measure_complete_flag = (uint8_t)s_measure_complete_flag;
    chain_command_complete_return(CHAIN_TOF_GET_MEASURE_COMPLETE_FLAG, (uint8_t *)&measure_complete_flag, 1);
}
