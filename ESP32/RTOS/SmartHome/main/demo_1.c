#include <stdio.h>
#include <string.h>
#include <string.h>
#include <time.h>
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

// Cấu hình UART
#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_RX_PIN 4
#define UART_TX_PIN 5
#define UART_BUF_SIZE 1024

// Cấu hình WiFi
#define WIFI_SSID "HAPPY HOME floor4"
#define WIFI_PASS "H@ppyhome4"
#define SERVER_URL "http://192.168.1.139:8080/data"

static const char *TAG = "UART_HTTP";
QueueHandle_t xSensorQueue;

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

// Cấu trúc dữ liệu cảm biến
typedef struct
{
    float temperature;
    float humidity;
} sensor_data_t;

// Khởi tạo UART
void uart_init()
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);
}

// Xử lý dữ liệu UART
void fake_data_task(void *pvParameters)
{
    sensor_data_t sensor_data;

    while (1)
    {
        sensor_data.temperature = rand() % 100;
        sensor_data.humidity = rand() % 100;

        ESP_LOGI(TAG, "Temperature: %.2f, Humidity: %.2f", sensor_data.temperature, sensor_data.humidity);

        xQueueSend(xSensorQueue, &sensor_data, portMAX_DELAY);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Hàm gửi JSON qua HTTP
void http_task(void *pvParameters)
{
    sensor_data_t sensor_data;

    while (1)
    {
        if (xQueueReceive(xSensorQueue, &sensor_data, portMAX_DELAY))
        {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "device", "ESP32");
            cJSON_AddNumberToObject(root, "temperature", sensor_data.temperature);
            cJSON_AddNumberToObject(root, "humidity", sensor_data.humidity);

            char *json_str = cJSON_Print(root);

            esp_http_client_config_t config = {
                .url = SERVER_URL,
                .method = HTTP_METHOD_POST,
                .transport_type = HTTP_TRANSPORT_OVER_TCP,
                .timeout_ms = 5000};

            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_post_field(client, json_str, strlen(json_str));

            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK)
            {
                int status_code = esp_http_client_get_status_code(client);
                if (status_code == 200)
                {
                    ESP_LOGI(TAG, "HTTP POST Status = %d", status_code);
                }
                else
                {
                    ESP_LOGE(TAG, "HTTP POST failed: %d", status_code);
                }
            }
            else
            {
                ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
            }

            esp_http_client_cleanup(client);
            cJSON_Delete(root);
            free(json_str);
        }
    }
}

// Hàm khởi tạo WiFi
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init()
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Đăng ký sự kiện WiFi
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Chờ kết nối WiFi (tối đa 30 giây)
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(30000));
    if (bits & WIFI_CONNECTED_BIT)
    {
        esp_netif_ip_info_t ip_info;
        ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info));
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&ip_info.ip));
    }
    else
    {
        ESP_LOGE(TAG, "Failed to connect WiFi!");
        esp_restart();
    }
}

void app_main()
{
    // Khởi tạo NVS và WiFi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();

    // Đợi kết nối WiFi
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    uint8_t mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    ESP_LOGI(TAG, "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Khởi tạo UART
    uart_init();

    // Tạo queue
    xSensorQueue = xQueueCreate(5, sizeof(sensor_data_t));

    // Tạo các task
    xTaskCreate(fake_data_task, "fake_data_task", 4096, NULL, 5, NULL);
    xTaskCreate(http_task, "http_task", 4096, NULL, 5, NULL);
}