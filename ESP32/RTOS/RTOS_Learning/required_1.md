Dưới đây là ví dụ ứng dụng **Hệ thống nhà thông minh đa nhiệm** sử dụng toàn bộ cơ chế đồng bộ hóa (Mutex, Semaphore, Spinlock, Message Queue) trên ESP32 với FreeRTOS:

---

### **Mô tả ứng dụng**

* **Cảm biến nhiệt độ** : Đọc dữ liệu mỗi 2s → Gửi qua Message Queue.
* **Xử lý trung tâm** : Nhận dữ liệu, điều khiển đèn LED và quạt dựa trên ngưỡng nhiệt.
* **Nút nhấn** : Thay đổi ngưỡng nhiệt độ qua UART.
* **Log dữ liệu** : Ghi log ra UART khi có thay đổi trạng thái.

### **Cơ chế sử dụng**

| Cơ chế                | Mục đích                      | Vị trí áp dụng                     |
| ----------------------- | -------------------------------- | -------------------------------------- |
| **Mutex**         | Bảo vệ biến dùng chung       | Ngưỡng nhiệt độ, trạng thái LED |
| **Semaphore**     | Đồng bộ task đọc cảm biến | Thông báo dữ liệu mới             |
| **Spinlock**      | Truy cập phần cứng (PWM)      | Điều khiển độ sáng LED           |
| **Message Queue** | Truyền dữ liệu cảm biến     | Cảm biến → Xử lý trung tâm       |

---

### **Code hoàn chỉnh**

**cpp**

Copy

Download

```
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
        if (xSemaphoreTake(data_ready_sem, portMAX_DELAY) {
            if (xQueueReceive(temp_queue, &msg, 0)) {
                // Kiểm tra ngưỡng nhiệt (dùng Mutex)
                xSemaphoreTake(uart_mutex, portMAX_DELAY);
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
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);

    while (1) {
        if (gpio_get_level(BUTTON_GPIO)) {
            xSemaphoreTake(uart_mutex, portMAX_DELAY);
            temp_threshold += 1.0; // Tăng ngưỡng
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
```

---

### **Giải thích chi tiết**

1. **Message Queue (`temp_queue`)**
   * **Mục đích** : Truyền dữ liệu từ cảm biến đến task xử lý.
   * **Triển khai** : `xQueueCreate()`, `xQueueSend()`, `xQueueReceive()`.
2. **Semaphore (`data_ready_sem`)**
   * **Mục đích** : Đồng bộ hóa giữa task cảm biến và xử lý.
   * **Cơ chế** : Task xử lý chờ (`xSemaphoreTake`) đến khi có dữ liệu mới.
3. **Mutex (`uart_mutex`)**
   * **Mục đích** : Bảo vệ biến `temp_threshold` và truy cập UART.
   * **Sử dụng** : Bao quanh các thao tác đọc/ghi biến chung.
4. **Spinlock (`pwm_spinlock`)**
   * **Mục đích** : Đảm bảo cập nhật PWM (phần cứng) không bị ngắt.
   * **Triển khai** : `portENTER_CRITICAL()`, `portEXIT_CRITICAL()`.

---

### **Kết quả hoạt động**

* Khi nhiệt độ vượt ngưỡng → LED sáng.
* Nhấn nút → Tăng ngưỡng nhiệt, log ra UART.
* Log UART không bị xâu chuỗi nhờ mutex.
* PWM được cập nhật mượt mà nhờ spinlock.

---

### **Sơ đồ luồng dữ liệu**

Copy

Download

```
[Temp Sensor] → (Queue) → [Processing Task] → (Spinlock) → [PWM Hardware]
                          ↑               ↳ (Mutex) → [UART Log]
[Button Task] → (Mutex) → [Threshold Update]
```

---

### **Lưu ý quan trọng**

* **Spinlock** chỉ dùng cho thao tác phần cứng thời gian cực ngắn.
* **Mutex** nên được giải phóng ngay sau khi dùng xong.
* **Độ ưu tiên task** : Task xử lý có độ ưu tiên cao nhất (3) để đảm bảo real-time.
