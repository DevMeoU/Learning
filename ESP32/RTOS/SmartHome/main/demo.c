#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "esp_http_client.h"
#include "cJSON.h"

// UART config
#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     115200
#define UART_RX_PIN        4
#define UART_TX_PIN        5
#define UART_BUF_SIZE      1024
#define MAX_PAYLOAD        4

// WiFi + Server config
#define WIFI_SSID         "HAPPY HOME floor4"
#define WIFI_PASS         "H@ppyhome4"
#define SERVER_URL        "http://192.168.1.139:8080/data"

static const char *TAG = "UART_HTTP";
QueueHandle_t xSensorQueue;

typedef struct {
    float temperature;
    float humidity;
} sensor_data_t;

void print_hex(const uint8_t *data, size_t len);

// UART init
void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);
}

// UART parser task
void uart_parser_task(void *pvParameters) {
    uint8_t rx_buf[UART_BUF_SIZE];
    uint8_t state = 0;
    uint8_t payload_len = 0;
    uint8_t payload_cnt = 0;
    uint8_t payload[MAX_PAYLOAD];
    uint8_t checksum = 0;

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, rx_buf, UART_BUF_SIZE, pdMS_TO_TICKS(100));
        for (int i = 0; i < len; i++) {
            uint8_t b = rx_buf[i];
            switch (state) {
                case 0:
                    if (b == 0xAA) state = 1;
                    break;
                case 1: // đọc độ dài payload
                    payload_len = b;
                    if (payload_len > MAX_PAYLOAD) {
                        state = 0;
                    } else {
                        payload_cnt = 0;
                        checksum = 0;         // reset checksum
                        checksum += b;        // bắt đầu tính checksum từ LENGTH
                        state = 2;
                    }
                    break;
                case 2:
                    payload[payload_cnt++] = b;
                    checksum += b;
                    if (payload_cnt >= payload_len) state = 3;
                    break;
                case 3:
                    checksum &= 0xFF; // checksum 8 bit
                    if (checksum == b) {
                        uint16_t raw_t = (payload[0] << 8) | payload[1];
                        uint16_t raw_h = (payload[2] << 8) | payload[3];
                        float T = raw_t / 100.0f;
                        float H = raw_h / 100.0f;
                        printf("Parsed: T=%.2f°C, H=%.2f%%\n", T, H);

                        sensor_data_t data = { .temperature = T, .humidity = H };
                        xQueueSend(xSensorQueue, &data, 0);
                    } else {
                        printf("Checksum lỗi: 0x%02X (đúng: 0x%02X)\n", b, checksum);
                        print_hex(payload, payload_len);
                    }
                    state = 0;
                    break;
            }
        }
    }
}

// HTTP task
void http_task(void *pvParameters) {
    sensor_data_t sensor_data;

    while (1) {
        if (xQueueReceive(xSensorQueue, &sensor_data, portMAX_DELAY)) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "device", "ESP32");
            cJSON_AddNumberToObject(root, "temperature", sensor_data.temperature);
            cJSON_AddNumberToObject(root, "humidity", sensor_data.humidity);

            char *json_str = cJSON_Print(root);

            esp_http_client_config_t config = {
                .url = SERVER_URL,
                .method = HTTP_METHOD_POST,
                .transport_type = HTTP_TRANSPORT_OVER_TCP,
            };

            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_post_field(client, json_str, strlen(json_str));

            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "HTTP POST Status = %d", esp_http_client_get_status_code(client));
            } else {
                ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
            }

            esp_http_client_cleanup(client);
            cJSON_Delete(root);
            free(json_str);
        }
    }
}

// WiFi init
void wifi_init() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

// Fake UART sender
void fake_uart_task(void *pvParameters) {
    const uint8_t frames[][8] = {
        {0xAA, 0x04, 0x08, 0xF7, 0x14, 0xA8, 0xBF},
        {0xAA, 0x04, 0x0A, 0x4F, 0x12, 0xEB, 0x5A},
        {0xAA, 0x04, 0x09, 0x5C, 0x15, 0x1B, 0x99},
    };

    while (1) {
        for (int i = 0; i < sizeof(frames) / sizeof(frames[0]); i++) {
            uart_write_bytes(UART_PORT_NUM, (const char *)frames[i], 7);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

void app_main() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();
    vTaskDelay(pdMS_TO_TICKS(5000)); // đợi WiFi

    uart_init();

    xSensorQueue = xQueueCreate(5, sizeof(sensor_data_t));

    xTaskCreate(uart_parser_task, "uart_parser", 4096, NULL, 5, NULL);
    xTaskCreate(http_task, "http_task", 4096, NULL, 5, NULL);
    xTaskCreate(fake_uart_task, "fake_uart", 4096, NULL, 4, NULL);  // dùng để test
}

// In hex dump để kiểm tra dữ liệu
void print_hex(const uint8_t *data, size_t len) {
    for(int i=0; i<len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}