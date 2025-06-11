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
    uint8_t led_green;      // Status of green LED
    uint8_t led_red;        // Status of red LED
    uint8_t led_yellow;     // Status of yellow LED
    uint8_t reserved;       // Reserved for alignment
    union sensor_data sensor_data;  // Sensor reading value
    int64_t timestamp;      // Timestamp for HTTP POST
} data_frame_t;

extern QueueHandle_t xSensorQueue;

#ifdef __cplusplus
extern "C" {
#endif

const char *get_current_time(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_TYPES_H */
