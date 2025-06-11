/* 
 * ESP32 SPIFFS Project with WiFi Configuration and Firebase Integration
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_tls.h"

static const char *TAG = "main";

// Event group để đồng bộ giữa init_task và app_main
static EventGroupHandle_t init_event_group;
#define INIT_DONE_BIT BIT0

// Function declarations from apmode_cfg.c
extern void wifi_init_sta(void);
extern void wifi_init_ap(void);
extern void start_webserver(void);
extern void uart_init(void);
extern void uart_parser_task(void *pvParameters);
extern void http_task(void *pvParameters);
extern void initialize_sntp(void);
extern QueueHandle_t xSensorQueue;
extern nvs_handle_t nvs_handle_storage;

// Định nghĩa struct data_frame_t từ apmode_cfg.c để dùng cho xQueueCreate
typedef struct {
    uint8_t led_green;
    uint8_t led_red;
    uint8_t led_yellow;
    uint8_t reserved;
    union sensor_data {
        uint32_t u32_sensor_value;
        float f_sensor_value;
    } sensor_data;
} data_frame_t;

// GPIO definitions from apmode_cfg.c
#define BUTTON_GPIO         GPIO_NUM_16 // GPIO for button
#define LED_NOTIFICATION    GPIO_NUM_2  // GPIO for notification LED
#define LED_GREEN           GPIO_NUM_15 // GPIO for green LED
#define LED_RED             GPIO_NUM_13 // GPIO for red LED
#define LED_YELLOW          GPIO_NUM_12 // GPIO for yellow LED

#define LED_NOTIFICATION_ON  gpio_set_level(LED_NOTIFICATION, 1)
#define LED_NOTIFICATION_OFF gpio_set_level(LED_NOTIFICATION, 0)
#define LED_GREEN_ON        gpio_set_level(LED_GREEN, 1)
#define LED_GREEN_OFF       gpio_set_level(LED_GREEN, 0)
#define LED_RED_ON          gpio_set_level(LED_RED, 1)
#define LED_RED_OFF         gpio_set_level(LED_RED, 0)
#define LED_YELLOW_ON       gpio_set_level(LED_YELLOW, 1)
#define LED_YELLOW_OFF      gpio_set_level(LED_YELLOW, 0)

// Function to initialize and mount SPIFFS with optimized settings
esp_err_t initialize_spiffs(void) {
    ESP_LOGI(TAG, "Initializing SPIFFS with optimized settings");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,  // Increased from 5 to allow more concurrent file operations
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return ret;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partition size info
    if (used > total) {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return ret;
        } else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }

    return ESP_OK;
}

// Function to configure GPIO pins
void configure_gpio(void) {
    // Configure GPIO for button
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Configure GPIO for LEDs
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_NOTIFICATION) | (1ULL << LED_GREEN) | 
                        (1ULL << LED_RED) | (1ULL << LED_YELLOW),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
}

// Task initialization function that runs on core 0
void init_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting initialization on core %d", xPortGetCoreID());
    
    // Bật log chi tiết
    esp_log_level_set("wifi", ESP_LOG_DEBUG);
    esp_log_level_set("UART_HTTP", ESP_LOG_DEBUG);

    // Kiểm tra bộ nhớ
    ESP_LOGI(TAG, "Free heap: %" PRIu32, esp_get_free_heap_size());

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Open NVS storage
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle_storage));

    // Initialize SPIFFS with optimized settings
    if (initialize_spiffs() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS, halting");
        vTaskDelete(NULL);
        return;
    }

    // Configure GPIO pins
    configure_gpio();

    // Initialize TLS
    esp_tls_init_global_ca_store();    // Đánh dấu init task đã hoàn thành
    xEventGroupSetBits(init_event_group, INIT_DONE_BIT);
    ESP_LOGI(TAG, "Initialization completed");
    
    // Xóa task
    vTaskDelete(NULL);
}

void app_main(void) {
    // Tạo event group để đồng bộ
    init_event_group = xEventGroupCreate();
    
    // Tạo init task trên core 0
    xTaskCreatePinnedToCore(init_task, "init_task", 4096, NULL, 5, NULL, 0);
    
    // Chờ init task hoàn thành
    xEventGroupWaitBits(init_event_group, INIT_DONE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    
    // Giải phóng event group
    vEventGroupDelete(init_event_group);

    bool enter_ap_mode = false;
    int timeout = 5; // 5 seconds

    ESP_LOGI(TAG, "Press and hold button for %d seconds to enter AP mode...", timeout);

    LED_NOTIFICATION_ON; // Turn on notification LED
    // 5-second countdown waiting for button press
    for (int i = timeout; i > 0; i--) {
        if (gpio_get_level(BUTTON_GPIO) == 0) { // 0 = button pressed (due to pull-up)
            enter_ap_mode = true;
            ESP_LOGI(TAG, "Button pressed! Entering AP mode...");
            break;
        }
        ESP_LOGI(TAG, "Timeout: %d seconds remaining", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    LED_NOTIFICATION_OFF; // Turn off notification LED

    // Kiểm tra và vào chế độ AP nếu được yêu cầu
    if (enter_ap_mode) {
        ESP_LOGI(TAG, "Starting AP mode for configuration...");
        wifi_init_ap();
        start_webserver();
        esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
        esp_log_level_set("http_client", ESP_LOG_VERBOSE);

        // Chờ cấu hình xong và khởi động lại
        while(1) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            uint8_t wifi_configured = 0;
            if (nvs_get_u8(nvs_handle_storage, "wifi_configured", &wifi_configured) == ESP_OK && wifi_configured == 1) {
                ESP_LOGI(TAG, "WiFi configured, restarting system...");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                esp_restart();
            }
        }
    } 
    // Không có nút nhấn, kiểm tra cấu hình và vào chế độ Station
    else {
        ESP_LOGI(TAG, "Checking saved configuration...");
        size_t required_size;
        char *saved_ssid = NULL;
        char *saved_pass = NULL;
        char *saved_server_url = NULL;
        char *saved_token = NULL;
        bool config_valid = true;

        // Kiểm tra cấu hình đã lưu
        if (nvs_get_str(nvs_handle_storage, "wifi_ssid", NULL, &required_size) == ESP_OK) {
            saved_ssid = malloc(required_size);
            nvs_get_str(nvs_handle_storage, "wifi_ssid", saved_ssid, &required_size);
            ESP_LOGI(TAG, "Found saved SSID: %s", saved_ssid);
        } else {
            ESP_LOGW(TAG, "No saved SSID found");
            config_valid = false;
        }

        if (nvs_get_str(nvs_handle_storage, "wifi_pass", NULL, &required_size) == ESP_OK) {
            saved_pass = malloc(required_size);
            nvs_get_str(nvs_handle_storage, "wifi_pass", saved_pass, &required_size);
            ESP_LOGI(TAG, "Found saved password");
        }

        if (nvs_get_str(nvs_handle_storage, "server_url", NULL, &required_size) == ESP_OK) {
            saved_server_url = malloc(required_size);
            nvs_get_str(nvs_handle_storage, "server_url", saved_server_url, &required_size);
            ESP_LOGI(TAG, "Found saved server URL: %s", saved_server_url);
        } else {
            ESP_LOGW(TAG, "No saved server URL found");
            config_valid = false;
        }

        if (nvs_get_str(nvs_handle_storage, "token", NULL, &required_size) == ESP_OK) {
            saved_token = malloc(required_size);
            nvs_get_str(nvs_handle_storage, "token", saved_token, &required_size);
            ESP_LOGI(TAG, "Found saved token");
        } else {
            ESP_LOGW(TAG, "No saved token found");
            config_valid = false;
        }

        // Giải phóng bộ nhớ        if (saved_ssid) free(saved_ssid);
        if (saved_pass) free(saved_pass);
        if (saved_server_url) free(saved_server_url);
        if (saved_token) free(saved_token);
        
        if (config_valid) {
            ESP_LOGI(TAG, "Valid configuration found, starting in Station mode...");
            
            // Khởi tạo WiFi Station mode trên core 0 (protocol core)
            wifi_init_sta();
            ESP_LOGI(TAG, "WiFi Station mode initialized");

            // Khởi tạo SNTP
            initialize_sntp();
            ESP_LOGI(TAG, "SNTP initialized");

            // Khởi tạo UART và các task xử lý dữ liệu
            uart_init();
            ESP_LOGI(TAG, "UART initialized");

            xSensorQueue = xQueueCreate(5, sizeof(data_frame_t));
            if (xSensorQueue == NULL) {
                ESP_LOGE(TAG, "Failed to create sensor queue");
                return;
            }
            ESP_LOGI(TAG, "Sensor queue created");

            // Tạo các task xử lý dữ liệu trên các core khác nhau
            ESP_LOGI(TAG, "Creating UART parser and HTTP tasks on specific cores");

            // UART parser task chạy trên core 1 (application core) với priority cao
            // để đảm bảo xử lý dữ liệu UART kịp thời
            xTaskCreatePinnedToCore(
                uart_parser_task,    // Task function
                "uart_parser",       // Task name
                1024 * 4,           // Stack size
                NULL,               // Parameters
                5,                  // Priority (higher)
                NULL,               // Task handle
                1                   // Core ID (1 = application core)
            );

            // HTTP task chạy trên core 0 (protocol core) với priority thấp hơn
            // vì đây là core xử lý WiFi và network
            xTaskCreatePinnedToCore(
                http_task,          // Task function
                "http_task",        // Task name
                1024 * 8,          // Stack size
                NULL,              // Parameters
                4,                 // Priority (lower)
                NULL,              // Task handle
                0                  // Core ID (0 = protocol core)
            );
        } else {
            ESP_LOGW(TAG, "No valid configuration found, restarting in AP mode...");
            nvs_set_u8(nvs_handle_storage, "force_ap", 1);
            nvs_commit(nvs_handle_storage);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        }
    }
}

// Moved urldecode function to apmode_cfg.c to avoid duplication