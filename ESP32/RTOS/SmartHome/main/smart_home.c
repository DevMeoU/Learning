#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

// Define hardware pins
#define TEMP_SENSOR_GPIO 34
#define LED_GPIO 5
#define BUTTON_GPIO 4

// Cấu trúc dữ liệu Message
typedef struct {
    float temperature;
} temp_msg_t;

// Global variables
float temp_threshold = 30.0; // Ngưỡng nhiệt
bool led_state = false;
portMUX_TYPE pwm_spinlock = portMUX_INITIALIZER_UNLOCKED;

// Tạo các đối tượng đồng bộ
QueueHandle_t temp_queue;
SemaphoreHandle_t data_ready_sem;
SemaphoreHandle_t uart_mutex;

// Hàm đọc nhiệt độ (giả lập)
float read_temperature() {
    return (float)(rand() % 40 + 20); // Random 20-60°C
}

// Task cảm biến
void temp_sensor_task(void *pvParam) {
    temp_msg_t msg;
    while (1) {
        msg.temperature = read_temperature();
        xQueueSend(temp_queue, &msg, portMAX_DELAY); // Gửi queue
        xSemaphoreGive(data_ready_sem); // Báo hiệu dữ liệu mới
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

// Task xử lý trung tâm
void processing_task(void *pvParam) {
    temp_msg_t msg;
    while (1) {
        if (xSemaphoreTake(data_ready_sem, portMAX_DELAY)) {
            if (xQueueReceive(temp_queue, &msg, 0)) {
                // Kiểm tra ngưỡng nhiệt (dùng Mutex)
                xSemaphoreTake(uart_mutex, portMAX_DELAY);
                printf("Temperature: %.1fC\n", msg.temperature);
                printf("Threshold: %.1fC\n", temp_threshold);
                bool new_led_state = (msg.temperature > temp_threshold);
                xSemaphoreGive(uart_mutex);

                // Điều khiển LED với Spinlock
                portENTER_CRITICAL(&pwm_spinlock);
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, new_led_state ? 8192 : 0);
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
                portEXIT_CRITICAL(&pwm_spinlock);
            }
        }
    }
}

// Task nút nhấn
void button_task(void *pvParam) {
    gpio_config_t io_conf = {
        .pin_bit_mask   = 1ULL<<BUTTON_GPIO,    // ví dụ BUTTON_GPIO = GPIO_NUM_0
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_ENABLE,    // bật pull-up nội
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE,
    };
    // Cấu hình GPIO cho nút nhấn
    gpio_config(&io_conf);

    while (1) {
        if (gpio_get_level(BUTTON_GPIO)) {
            printf("Button pressed\n");
            xSemaphoreTake(uart_mutex, portMAX_DELAY);
            temp_threshold += 1.0; // Tăng ngưỡng
            if (temp_threshold > 35.0) {
                temp_threshold = 35.0; // Giới hạn ngưỡng tối đa
            }
            printf("New threshold: %.1fC\n", temp_threshold);
            xSemaphoreGive(uart_mutex);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        else
        {
            printf("Button released\n");
            xSemaphoreTake(uart_mutex, portMAX_DELAY);
            temp_threshold -= 1.0; // Giảm ngưỡng
            if (temp_threshold < 25.0) {
                temp_threshold = 25.0; // Giới hạn ngưỡng tối thiểu
            }
            printf("New threshold: %.1fC\n", temp_threshold);
            xSemaphoreGive(uart_mutex);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Khởi tạo PWM cho LED
void init_led_pwm() {
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ch_conf = {
        .gpio_num = LED_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0
    };
    ledc_channel_config(&ch_conf);
}

void app_main() {
    // Khởi tạo phần cứng
    init_led_pwm();

    // Tạo các cơ chế đồng bộ
    temp_queue = xQueueCreate(5, sizeof(temp_msg_t));
    data_ready_sem = xSemaphoreCreateBinary();
    uart_mutex = xSemaphoreCreateMutex();

    // Tạo tasks
    xTaskCreatePinnedToCore(temp_sensor_task, "Temp", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(processing_task, "Process", 4096, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(button_task, "Button", 2048, NULL, 1, NULL, 1);
}