#ifndef SPA06_SENSOR_H
#define SPA06_SENSOR_H

#include "main.h"
#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPA06_ADDR_DEFAULT                  0x77U
#define SPA06_ADDR_ALT                      0x76U
#define SPA06_EXPECTED_ID                   0x11U
#define SPA06_I2C_TIMEOUT_MS                100U
#define SPA06_DEFAULT_SEA_LEVEL_PRESSURE_PA 101330.0

#define SPA06_REG_PRS_B2   0x00U
#define SPA06_REG_TMP_B2   0x03U
#define SPA06_REG_PRS_CFG  0x06U
#define SPA06_REG_TMP_CFG  0x07U
#define SPA06_REG_MEAS_CFG 0x08U
#define SPA06_REG_CFG_REG  0x09U
#define SPA06_REG_INT_STS  0x0AU
#define SPA06_REG_FIFO_STS 0x0BU
#define SPA06_REG_RESET    0x0CU
#define SPA06_REG_ID       0x0DU
#define SPA06_REG_COEF     0x10U

typedef enum {
    SPA06_1HZ = 0,
    SPA06_2HZ,
    SPA06_4HZ,
    SPA06_8HZ,
    SPA06_16HZ,
    SPA06_32HZ,
    SPA06_64HZ,
    SPA06_128HZ,
    SPA06_25_16HZ,
    SPA06_25_8HZ,
    SPA06_25_4HZ,
    SPA06_25_2HZ,
    SPA06_25HZ,
    SPA06_50HZ,
    SPA06_100HZ,
    SPA06_200HZ,
} spa06_measure_rate_t;

typedef enum {
    SPA06_1SAMPLE = 0,
    SPA06_2SAMPLES,
    SPA06_4SAMPLES,
    SPA06_8SAMPLES,
    SPA06_16SAMPLES,
    SPA06_32SAMPLES,
    SPA06_64SAMPLES,
    SPA06_128SAMPLES,
} spa06_oversample_rate_t;

typedef enum {
    SPA06_IDLE             = 0x00,
    SPA06_ONE_PRESSURE     = 0x01,
    SPA06_ONE_TEMPERATURE  = 0x02,
    SPA06_CONT_PRESSURE    = 0x05,
    SPA06_CONT_TEMPERATURE = 0x06,
    SPA06_CONT_PRES_TEMP   = 0x07,
} spa06_mode_t;

typedef enum {
    SPA06_INT_OFF = 0,
    SPA06_INT_PRES,
    SPA06_INT_TEMP,
    SPA06_INT_PRES_TEMP,
    SPA06_INT_FIFO,
    SPA06_INT_FIFO_PRES,
    SPA06_INT_FIFO_TEMP,
    SPA06_INT_ALL,
} spa06_interrupt_option_t;

typedef enum {
    SPA06_TSRC_ASIC = 0,
    SPA06_TSRC_MEMS,
} spa06_temperature_source_t;

typedef struct {
    I2C_HandleTypeDef *i2c;
    uint8_t address;
    uint8_t inited;

    int16_t c0;
    int16_t c1;
    int32_t c00;
    int32_t c10;
    int16_t c01;
    int16_t c11;
    int16_t c20;
    int16_t c21;
    int16_t c30;
    int16_t c31;
    int16_t c40;

    double pressure_offset_pa;
    double sea_level_pressure_pa;
    double temperature_offset_c;
    spa06_measure_rate_t pressure_rate;
    spa06_measure_rate_t temperature_rate;
    spa06_oversample_rate_t pressure_oversample;
    spa06_oversample_rate_t temperature_oversample;
    spa06_mode_t mode;
    spa06_temperature_source_t temperature_source;
    uint8_t pressure_shift_enabled;
    uint8_t temperature_shift_enabled;
    uint8_t meas_cfg;
    uint8_t cfg_reg;
} spa06_sensor_t;

extern spa06_sensor_t spa06_sensor;
extern volatile uint32_t spa06_pressure_pa;
extern volatile int32_t spa06_temperature_c_x100;
extern volatile int32_t spa06_altitude_cm;
extern volatile uint8_t spa06_sensor_last_status;

uint8_t spa06_sensor_init(void);
uint8_t spa06_sensor_init_with_addr(uint8_t address);
uint8_t spa06_sensor_deinit(void);
uint8_t spa06_sensor_reset(void);
uint8_t spa06_sensor_apply_config_from_registers(void);

uint8_t spa06_sensor_set_mode(spa06_mode_t mode);
uint8_t spa06_sensor_set_pressure_config(spa06_measure_rate_t rate, spa06_oversample_rate_t oversample);
uint8_t spa06_sensor_set_temperature_config(spa06_measure_rate_t rate, spa06_oversample_rate_t oversample);
uint8_t spa06_sensor_set_temperature_source(spa06_temperature_source_t source);
uint8_t spa06_sensor_set_interrupt_active_high(uint8_t active_high);
uint8_t spa06_sensor_configure_interrupt(spa06_interrupt_option_t option);
uint8_t spa06_sensor_get_interrupt_status(uint8_t *status);

uint8_t spa06_sensor_pressure_available(uint8_t *available);
uint8_t spa06_sensor_temperature_available(uint8_t *available);
void spa06_sensor_set_pressure_offset(double offset_pa);
void spa06_sensor_set_sea_level_pressure(double pressure_pa);
void spa06_sensor_set_temperature_offset(double offset_c);

uint8_t spa06_sensor_read_pressure(uint32_t *pressure_pa);
uint8_t spa06_sensor_read_temperature(double *temperature_c);
uint8_t spa06_sensor_read_altitude(int32_t *altitude_cm);
uint8_t spa06_sensor_read_all(uint32_t *pressure_pa, double *temperature_c, int32_t *altitude_cm);
uint8_t spa06_data_update(void);

#ifdef __cplusplus
}
#endif

#endif
