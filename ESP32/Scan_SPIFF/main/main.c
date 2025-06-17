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
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "include/gpio_control.h"
#include "include/uart_handler.h"
#include "include/wifi_manager.h"
#include "include/spiffs_handler.h"
#include "include/http_server.h"
#include "include/sntp_handler.h"
#include "include/common_types.h"
#include "sms_task.h"

static const char *TAG = "main";

// Function prototypes
void initialize_sntp(void);
void gpio0_sms_task(void *pvParameters);

// Event group để đồng bộ giữa init_task và app_main
static EventGroupHandle_t init_event_group;
#define INIT_DONE_BIT BIT0

extern nvs_handle_t nvs_handle_storage;

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

    // Initialize GPIO
    ESP_ERROR_CHECK(init_gpio());

    // Initialize TLS
    esp_tls_init_global_ca_store();// Đánh dấu init task đã hoàn thành
    xEventGroupSetBits(init_event_group, INIT_DONE_BIT);
    ESP_LOGI(TAG, "Initialization completed");
    
    // Xóa task
    vTaskDelete(NULL);
}

static char* nvs_get_alloc_str(nvs_handle_t h, const char *key) {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(h, key, NULL, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No \"%s\" in NVS (%s)", key, esp_err_to_name(err));
        return NULL;
    }
    char *buf = malloc(required_size);
    if (!buf) {
        ESP_LOGE(TAG, "Out of memory allocating for \"%s\"", key);
        return NULL;
    }
    err = nvs_get_str(h, key, buf, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read \"%s\" failed: %s", key, esp_err_to_name(err));
        free(buf);
        return NULL;
    }
    return buf;
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
        if (gpio_get_level(BUTTON_PAIR_CFG) == 0) { // 0 = button pressed (due to pull-up)
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
        bool config_valid = true;

        // Kiểm tra cấu hình đã lưu
        char *saved_ssid           = nvs_get_alloc_str(nvs_handle_storage, "wifi_ssid");
        char *saved_pass           = nvs_get_alloc_str(nvs_handle_storage, "wifi_password");
        char *saved_firebase_url   = nvs_get_alloc_str(nvs_handle_storage, "firebase_url");
        char *saved_token          = nvs_get_alloc_str(nvs_handle_storage, "token");
        char *saved_aws_url        = nvs_get_alloc_str(nvs_handle_storage, "aws_url");
        char *saved_phone_numbers  = nvs_get_alloc_str(nvs_handle_storage, "phone_numbers");
        char *saved_sms_messages   = nvs_get_alloc_str(nvs_handle_storage, "sms_messages");
        char *saved_twilio_acc_sid = nvs_get_alloc_str(nvs_handle_storage, "twilio_acc_sid");
        char *saved_twilio_token = nvs_get_alloc_str(nvs_handle_storage, "twilio_token");
        char *saved_twilio_from_n = nvs_get_alloc_str(nvs_handle_storage, "twilio_from_n");
        char *saved_twilio_url = nvs_get_alloc_str(nvs_handle_storage, "twilio_url");
        char *saved_enable_twilio = nvs_get_alloc_str(nvs_handle_storage, "enable_twilio");
        char *saved_enable_aws = nvs_get_alloc_str(nvs_handle_storage, "enable_aws");

        ESP_LOGI(TAG, "SSID: %s", saved_ssid ? saved_ssid : "(not set)");
        ESP_LOGI(TAG, "Password: %s", saved_pass ? saved_pass : "(not set)");
        ESP_LOGI(TAG, "Firebase URL: %s", saved_firebase_url ? saved_firebase_url : "(not set)");
        ESP_LOGI(TAG, "Token: %s", saved_token ? saved_token : "(not set)");
        ESP_LOGI(TAG, "AWS URL: %s", saved_aws_url ? saved_aws_url : "(not set)");
        ESP_LOGI(TAG, "Phone numbers: %s", saved_phone_numbers ? saved_phone_numbers : "(not set)");
        ESP_LOGI(TAG, "SMS messages: %s", saved_sms_messages ? saved_sms_messages : "(not set)");
        ESP_LOGI(TAG, "Twilio account SID: %s", saved_twilio_acc_sid ? saved_twilio_acc_sid : "(not set)");
        ESP_LOGI(TAG, "Twilio auth token: %s", saved_twilio_token ? saved_twilio_token : "(not set)");
        ESP_LOGI(TAG, "Twilio from number: %s", saved_twilio_from_n ? saved_twilio_from_n : "(not set)");
        ESP_LOGI(TAG, "Twilio URL: %s", saved_twilio_url ? saved_twilio_url : "(not set)");
        ESP_LOGI(TAG, "Enable Twilio: %s", saved_enable_twilio ? saved_enable_twilio : "(not set)");
        ESP_LOGI(TAG, "Enable AWS: %s", saved_enable_aws ? saved_enable_aws : "(not set)");

        if (saved_ssid) free(saved_ssid);
        if (saved_pass) free(saved_pass);
        if (saved_firebase_url) free(saved_firebase_url);
        if (saved_token) free(saved_token);
        if (saved_aws_url) free(saved_aws_url);
        if (saved_phone_numbers) free(saved_phone_numbers);
        if (saved_sms_messages) free(saved_sms_messages);
        if (saved_twilio_acc_sid) free(saved_twilio_acc_sid);
        if (saved_twilio_token) free(saved_twilio_token);
        if (saved_twilio_from_n) free(saved_twilio_from_n);
        if (saved_twilio_url) free(saved_twilio_url);
        if (saved_enable_twilio) free(saved_enable_twilio);
        if (saved_enable_aws) free(saved_enable_aws);
        
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

            init_network();
            ESP_LOGI(TAG, "Network initialized");
            
            // UART parser task chạy trên core 1 (application core) với priority cao
            // để đảm bảo xử lý dữ liệu UART kịp thời
            xTaskCreatePinnedToCore(
                uart_parser_task,    // Task function
                "uart_parser",       // Task name
                1024 * 8,           // Stack size
                NULL,               // Parameters
                4,                  // Priority (higher)
                NULL,               // Task handle
                1                   // Core ID (1 = application core)
            );

            // HTTP task chạy trên core 0 (protocol core) với priority thấp hơn
            // vì đây là core xử lý WiFi và network
            xTaskCreatePinnedToCore(
                http_task,          // Task function
                "http_task",        // Task name
                1024 * 32,          // Stack size
                NULL,              // Parameters
                3,                 // Priority (lower)
                NULL,              // Task handle
                0                  // Core ID (0 = protocol core)
            );

            // Tạo task gửi dữ liệu lên Firebase trên core 1
            xTaskCreatePinnedToCore(
                firebase_sender_task,   // Task function
                "firebase_sender",     // Task name
                1024 * 8,                   // Stack size
                NULL,                   // Parameters
                5,                      // Priority (thấp hơn parser)
                NULL,                   // Task handle
                1                       // Core ID (1 = application core)
            );

            // After creating other tasks, create the GPIO0 SMS task
            xTaskCreatePinnedToCore(
                sms_task_wrapper,
                "sms_task",
                1024 * 32, // Stack size
                NULL,
                5, // Lower priority
                NULL,
                0 // Application core
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

