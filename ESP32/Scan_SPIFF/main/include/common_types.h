#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>

// Union for sensor data values
union sensor_data {
    uint32_t u32_sensor_value;
    float f_sensor_value;
};

// Data structure for sensor readings
typedef struct {
    uint8_t led_status; /** Trạng thái led */
    uint8_t reserved[3];
    union sensor_data temperature;
    union sensor_data temperature_fake;
} data_frame_t;

extern QueueHandle_t xSensorQueue;

#ifdef __cplusplus
extern "C" {
#endif

const char *get_current_time(void);
int32_t get_current_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_TYPES_H */
