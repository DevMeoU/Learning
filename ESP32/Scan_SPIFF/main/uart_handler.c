#include <string.h>
#include "uart_handler.h"
#include "esp_log.h"
#include "gpio_control.h"
#include "common_types.h"
#include "cJSON.h"
#include "http_server.h"
#include "nvs.h"

static const char *TAG = "UART";
QueueHandle_t xSensorQueue = NULL;
extern nvs_handle_t nvs_handle_storage;

void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);
    
    ESP_LOGI(TAG, "UART initialized");
}

// Prototype
bool is_internet_available(void);

// Task gửi dữ liệu lên Firebase
void firebase_sender_task(void *pvParameters) {
    data_frame_t recv_data;
    char firebase_url[128] = {0};
    char auth_token[128] = {0}; // Thêm biến lưu trữ auth token
    size_t url_len = sizeof(firebase_url);
    size_t token_len = sizeof(auth_token);
    
    // Lấy token từ NVS
    nvs_get_str(nvs_handle_storage, "token", auth_token, &token_len);
    
    while (1) {
        if (xQueueReceive(xSensorQueue, &recv_data, portMAX_DELAY) == pdTRUE) {
            // Lấy URL từ NVS mỗi lần gửi để cập nhật thay đổi
            url_len = sizeof(firebase_url);
            nvs_get_str(nvs_handle_storage, "firebase_url", firebase_url, &url_len);
            
            if (!is_internet_available()) {
                ESP_LOGW(TAG, "No internet connection, skipping HTTP request");
                continue;
            }
            
            if (strlen(firebase_url) == 0) {
                ESP_LOGE(TAG, "firebase_url is empty, skipping HTTP request");
                continue;
            }
            
            // Tạo JSON dữ liệu
            cJSON *root = cJSON_CreateObject();
            cJSON_AddNumberToObject(root, "device_id", 1);
            cJSON_AddStringToObject(root, "time", get_current_time());
            cJSON_AddNumberToObject(root, "timestamp", get_current_timestamp());
            cJSON_AddStringToObject(root, "timezone", TIMEZONE);
            cJSON_AddNumberToObject(root, "led_warning", recv_data.led_status);
            cJSON_AddNumberToObject(root, "temperature_sensor_real", recv_data.temperature.f_sensor_value);
            cJSON_AddNumberToObject(root, "temperature_sensor_fake", recv_data.temperature_fake.f_sensor_value);

            char *json_buf = cJSON_PrintUnformatted(root);
            cJSON_Delete(root);
            
            // Xây dựng URL với phương thức PUT và auth token
            char full_url[512];
            if (strlen(auth_token) > 0) {
                snprintf(full_url, sizeof(full_url), 
                         "%s/data_stream.json?auth=%s", 
                         firebase_url, 
                         auth_token);
            } else {
                snprintf(full_url, sizeof(full_url), 
                         "%s/data_stream.json", 
                         firebase_url);
            }
            
            send_to_firebase(full_url, json_buf, HTTP_METHOD_PUT);
            
            free(json_buf);
            
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void uart_parser_task(void *pvParameters) {
    uint8_t rx_buf[UART_BUF_SIZE] = {0};
    uint8_t state = 0;
    uint8_t payload_len = 0;
    uint8_t payload_cnt = 0;
    uint8_t payload[MAX_PAYLOAD];
    uint8_t checksum = 0;
    ESP_LOGI(TAG, "UART Parser Task started");

    while (1) {
        LED_NOTIFICATION_ON;
        memset(rx_buf, 0, UART_BUF_SIZE); // Clear buffer before reading
        int len = uart_read_bytes(UART_PORT_NUM, rx_buf, UART_BUF_SIZE, pdMS_TO_TICKS(100));
        if (len > 0) {
            printf("Receive data: ");
            print_hex(rx_buf, len);
        } else {
            ESP_LOGW(TAG, "Waiting for data...");
            continue;
        }

        for (int i = 0; i < len; i++) {
            uint8_t b = rx_buf[i];
            switch (state) {
                case 0: // Chờ byte start
                    if (b == 0xAA) state = 1;
                    break;
                case 1: // Đọc độ dài payload
                    payload_len = b;
                    // ESP_LOGI(TAG, "Payload length: %d, Max payload length: %d, sizeof(data_frame_t): %d", payload_len, MAX_PAYLOAD, sizeof(data_frame_t));
                    if (payload_len > MAX_PAYLOAD || payload_len != sizeof(data_frame_t)) {
                        ESP_LOGE(TAG, "Invalid payload length: %d", payload_len);
                        state = 0;
                    } else {
                        payload_cnt = 0;
                        checksum = 0;
                        checksum += b; // Bắt đầu tính checksum từ LENGTH
                        state = 2;
                    }
                    break;
                case 2: // Đọc toàn bộ payload
                    payload[payload_cnt++] = b;
                    checksum += b;
                    if (payload_cnt >= payload_len) state = 3;
                    break;
                case 3: // Đọc và kiểm tra checksum
                    checksum &= 0xFF;
                    if (checksum == b) {
                        data_frame_t recv_data;
                        memcpy(&recv_data, payload, sizeof(data_frame_t));
                        SET_ALL_LED(recv_data.led_status, 0, 0); // Cập nhật trạng thái LED
                        ESP_LOGI(TAG, "Time: %s, Led Status: %d, Temperature: %.2f, Fake Temp: %.2f",
                                 get_current_time(),
                                 recv_data.led_status,
                                 recv_data.temperature.f_sensor_value,
                                 recv_data.temperature_fake.f_sensor_value);
                        xQueueSend(xSensorQueue, &recv_data, 0);
                    } else {
                        ESP_LOGE(TAG, "Checksum error: expected 0x%02X, got 0x%02X", checksum, b);
                    }
                    state = 0;
                    break;
                default:
                    state = 0;
                    break;
            }
        }
        LED_NOTIFICATION_OFF; // Tắt LED thông báo sau khi xử lý xong
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void print_hex(const uint8_t *data, size_t len) {
    for(int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}
