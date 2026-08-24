#ifndef _IMU_SENSOR_H_
#define _IMU_SENSOR_H_

#include <stdint.h>
#include "i2c.h"
#include "lsm6ds3tr-c_reg.h"

typedef struct {
    int16_t accel_raw[3];
    int16_t gyro_raw[3];
    int16_t temperature_raw;
    uint8_t accel_data_ready;
    uint8_t gyro_data_ready;
    uint8_t temperature_data_ready;
} imu_sensor_raw_data_t;

typedef struct {
    float accel_mg[3];
    float gyro_mdps[3];
    float temperature_c;
    uint8_t accel_data_ready;
    uint8_t gyro_data_ready;
    uint8_t temperature_data_ready;
} imu_sensor_unit_data_t;

typedef struct {
    imu_sensor_raw_data_t raw;
    imu_sensor_unit_data_t unit;
} imu_sensor_data_t;

typedef enum {
    IMU_SENSOR_ACCEL_RANGE_2G  = 0,
    IMU_SENSOR_ACCEL_RANGE_16G = 1,
    IMU_SENSOR_ACCEL_RANGE_4G  = 2,
    IMU_SENSOR_ACCEL_RANGE_8G  = 3,
} imu_sensor_accel_range_t;

typedef enum {
    IMU_SENSOR_GYRO_RANGE_250DPS  = 0,
    IMU_SENSOR_GYRO_RANGE_125DPS  = 1,
    IMU_SENSOR_GYRO_RANGE_500DPS  = 2,
    IMU_SENSOR_GYRO_RANGE_1000DPS = 4,
    IMU_SENSOR_GYRO_RANGE_2000DPS = 6,
} imu_sensor_gyro_range_t;

typedef enum {
    IMU_SENSOR_ACCEL_ODR_OFF    = 0,
    IMU_SENSOR_ACCEL_ODR_12HZ5  = 1,
    IMU_SENSOR_ACCEL_ODR_26HZ   = 2,
    IMU_SENSOR_ACCEL_ODR_52HZ   = 3,
    IMU_SENSOR_ACCEL_ODR_104HZ  = 4,
    IMU_SENSOR_ACCEL_ODR_208HZ  = 5,
    IMU_SENSOR_ACCEL_ODR_416HZ  = 6,
    IMU_SENSOR_ACCEL_ODR_833HZ  = 7,
    IMU_SENSOR_ACCEL_ODR_1660HZ = 8,
    IMU_SENSOR_ACCEL_ODR_3330HZ = 9,
    IMU_SENSOR_ACCEL_ODR_6660HZ = 10,
    IMU_SENSOR_ACCEL_ODR_1HZ6   = 11,
} imu_sensor_accel_odr_t;

typedef enum {
    IMU_SENSOR_GYRO_ODR_OFF    = 0,
    IMU_SENSOR_GYRO_ODR_12HZ5  = 1,
    IMU_SENSOR_GYRO_ODR_26HZ   = 2,
    IMU_SENSOR_GYRO_ODR_52HZ   = 3,
    IMU_SENSOR_GYRO_ODR_104HZ  = 4,
    IMU_SENSOR_GYRO_ODR_208HZ  = 5,
    IMU_SENSOR_GYRO_ODR_416HZ  = 6,
    IMU_SENSOR_GYRO_ODR_833HZ  = 7,
    IMU_SENSOR_GYRO_ODR_1660HZ = 8,
    IMU_SENSOR_GYRO_ODR_3330HZ = 9,
    IMU_SENSOR_GYRO_ODR_6660HZ = 10,
} imu_sensor_gyro_odr_t;

typedef enum {
    IMU_SENSOR_INT_PUSH_PULL = 0,
    IMU_SENSOR_INT_OPEN_DRAIN,
} imu_sensor_int_pin_mode_t;

typedef enum {
    IMU_SENSOR_INT1 = 0,
    IMU_SENSOR_INT2,
} imu_sensor_int_pin_t;

typedef enum {
    IMU_SENSOR_INT_ACTIVE_HIGH = 0,
    IMU_SENSOR_INT_ACTIVE_LOW,
} imu_sensor_int_polarity_t;

typedef enum {
    IMU_SENSOR_INT_PULSED = 0,
    IMU_SENSOR_INT_LATCHED,
} imu_sensor_int_latch_mode_t;

typedef enum {
    IMU_SENSOR_EVENT_NONE       = 0x0000U,
    IMU_SENSOR_EVENT_ACCEL_DRDY = 0x0001U,
    IMU_SENSOR_EVENT_GYRO_DRDY  = 0x0002U,
    IMU_SENSOR_EVENT_TEMP_DRDY  = 0x0004U,
    IMU_SENSOR_EVENT_WAKE_UP    = 0x0008U,
} imu_sensor_int_event_t;

extern volatile imu_sensor_data_t g_imu_sensor_data;
extern volatile int32_t g_imu_sensor_last_status;
extern volatile uint16_t imu_temperature_c_x100;

int32_t imu_sensor_init(void);
int32_t imu_sensor_set_accel_range(imu_sensor_accel_range_t range);
int32_t imu_sensor_set_gyro_range(imu_sensor_gyro_range_t range);
int32_t imu_sensor_set_accel_odr(imu_sensor_accel_odr_t odr);
int32_t imu_sensor_set_gyro_odr(imu_sensor_gyro_odr_t odr);
int32_t imu_sensor_get_accel_range(imu_sensor_accel_range_t *range);
int32_t imu_sensor_get_gyro_range(imu_sensor_gyro_range_t *range);
int32_t imu_sensor_get_accel_odr(imu_sensor_accel_odr_t *odr);
int32_t imu_sensor_get_gyro_odr(imu_sensor_gyro_odr_t *odr);
int32_t imu_sensor_config_interrupt_pin(imu_sensor_int_pin_mode_t mode, imu_sensor_int_polarity_t polarity,
                                        imu_sensor_int_latch_mode_t latch_mode);
int32_t imu_sensor_enable_interrupt(imu_sensor_int_pin_t pin, uint16_t events);
int32_t imu_sensor_disable_interrupt(imu_sensor_int_pin_t pin, uint16_t events);
int32_t imu_sensor_get_interrupt_source(uint16_t *events);
int32_t imu_sensor_read(void);
int32_t imu_sensor_read_raw(void);
int32_t imu_sensor_read_raw_data(imu_sensor_raw_data_t *data);
int32_t imu_sensor_read_unit_data(imu_sensor_unit_data_t *data);
int32_t imu_sensor_read_data(imu_sensor_data_t *data);
int32_t imu_sensor_read_raw_temperature(int16_t *temperature_raw);
int32_t imu_sensor_read_unit_temperature(float *temperature_c);
int32_t imu_sensor_read_temperature(float *temperature_c);
int32_t imu_sensor_read_temperature_data(int16_t *temperature_raw, float *temperature_c);
int32_t imu_data_update(void);

#endif
