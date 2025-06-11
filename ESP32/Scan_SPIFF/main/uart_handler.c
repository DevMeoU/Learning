#include <string.h>
#include "uart_handler.h"
#include "esp_log.h"
#include "gpio_control.h"
#include "common_types.h"

static const char *TAG = "UART";
QueueHandle_t xSensorQueue = NULL;

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
                        SET_ALL_LED(recv_data.led_green, recv_data.led_red, recv_data.led_yellow);
                        // In ra thông tin nhận được
                        ESP_LOGI(TAG, "Time: %s, LED Green: %d, LED Red: %d, LED Yellow: %d, Sensor Value: %.2f",
                                 get_current_time(),
                                 recv_data.led_green,
                                 recv_data.led_red,
                                 recv_data.led_yellow,
                                 recv_data.sensor_data.f_sensor_value);
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
