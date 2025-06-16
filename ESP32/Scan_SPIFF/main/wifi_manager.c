#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "gpio_control.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define MAX_AP_COUNT 20
#define SCAN_TIMEOUT_MS 10000  // 10 second timeout for scan
#define MODE_SWITCH_DELAY_MS 300
#define CONNECTION_CHECK_TIMEOUT_MS 5000 // 5 second timeout for connection check

static const char *TAG = "WiFi";
EventGroupHandle_t wifi_event_group;
nvs_handle_t nvs_handle_storage;

// Global variables for scan state
static wifi_ap_record_t ap_records[MAX_AP_COUNT];
static uint16_t ap_count = 0;
static bool scan_done = false;
static bool was_ap_mode = false;  // Track if we were in AP mode before scan

static TaskHandle_t ap_led_blink_task_handle = NULL;

static void ap_led_blink_task(void *pvParameter) {
    while (1) {
        LED_NOTIFICATION_ON;
        vTaskDelay(pdMS_TO_TICKS(200));
        LED_NOTIFICATION_OFF;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void start_ap_led_blink(void) {
    if (ap_led_blink_task_handle == NULL) {
        xTaskCreate(ap_led_blink_task, "ap_led_blink", 1024, NULL, 3, &ap_led_blink_task_handle);
    }
}

void stop_ap_led_blink(void) {
    if (ap_led_blink_task_handle != NULL) {
        vTaskDelete(ap_led_blink_task_handle);
        ap_led_blink_task_handle = NULL;
        LED_NOTIFICATION_OFF;
    }
}

static esp_err_t ensure_wifi_ready_for_scan(void) {
    wifi_mode_t current_mode;
    esp_err_t ret = esp_wifi_get_mode(&current_mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }

    if (current_mode == WIFI_MODE_STA) {
        // If in station mode, check if we're trying to connect
        wifi_ap_record_t ap_info;
        ret = esp_wifi_sta_get_ap_info(&ap_info);
        
        if (ret == ESP_ERR_WIFI_CONN) {
            // We're trying to connect - wait for either connection or timeout
            TickType_t start_time = xTaskGetTickCount();
            while ((xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS < CONNECTION_CHECK_TIMEOUT_MS) {
                if (wifi_event_group != NULL) {
                    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                        WIFI_CONNECTED_BIT,
                        pdFALSE,
                        pdTRUE,
                        pdMS_TO_TICKS(100));
                    if (bits & WIFI_CONNECTED_BIT) {
                        // Connected, we can proceed with scan
                        return ESP_OK;
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            // Timeout waiting for connection, disconnect to allow scan
            ESP_LOGW(TAG, "Connection attempt timeout, disconnecting to allow scan");
            esp_wifi_disconnect();
            vTaskDelay(pdMS_TO_TICKS(500)); // Chờ lâu hơn để driver WiFi xử lý xong trạng thái
            // Đảm bảo mode là APSTA trước khi scan
            wifi_mode_t mode;
            esp_wifi_get_mode(&mode);
            if (mode != WIFI_MODE_APSTA) {
                ESP_LOGI(TAG, "Switching to APSTA mode for scan");
                esp_err_t mode_ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
                if (mode_ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to set APSTA mode: %s", esp_err_to_name(mode_ret));
                    return mode_ret;
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                esp_wifi_get_mode(&mode);
                if (mode != WIFI_MODE_APSTA) {
                    ESP_LOGE(TAG, "Mode verification failed after switching to APSTA mode");
                    return ESP_FAIL;
                }
            }
        }
    }
    
    return ESP_OK;
}

// Forward declarations
static void cleanup_scan_resources(void);

static void cleanup_scan_resources(void) {
    scan_done = true;
    ap_count = 0;
    
    if (was_ap_mode) {
        ESP_LOGI(TAG, "Cleaning up: switching back to AP mode");
        esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to switch back to AP mode during cleanup: %s", esp_err_to_name(err));
        }
        was_ap_mode = false;
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                stop_ap_led_blink(); // Tắt nhấp nháy khi chuyển sang STA
                LED_NOTIFICATION_ON; // Sáng liên tục khi đang kết nối STA
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (wifi_event_group != NULL) {
                    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
                }
                LED_NOTIFICATION_OFF; // Tắt đèn khi mất kết nối
                esp_wifi_connect();
                break;
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI(TAG, "Scan completed");
                
                wifi_ap_record_t *ap_info = NULL;
                uint16_t ap_count_temp = 0;
                uint16_t ap_num = 0;
                esp_err_t err = esp_wifi_scan_get_ap_num(&ap_count_temp);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to get AP count: %s", esp_err_to_name(err));
                    goto cleanup;
                }
                
                if (ap_count_temp > 0) {
                    ap_num = MIN(ap_count_temp, MAX_AP_COUNT);
                    ap_info = calloc(ap_num, sizeof(wifi_ap_record_t));
                    if (!ap_info) {
                        ESP_LOGE(TAG, "Failed to allocate memory for AP records");
                        goto cleanup;
                    }
                    err = esp_wifi_scan_get_ap_records(&ap_num, ap_info);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "Failed to get AP records: %s", esp_err_to_name(err));
                        free(ap_info);
                        goto cleanup;
                    }
                    // Copy results to our static buffer
                    memcpy(ap_records, ap_info, ap_num * sizeof(wifi_ap_record_t));
                    ap_count = ap_num;
                    free(ap_info);
                } else {
                    ap_count = 0;
                    ESP_LOGW(TAG, "No APs found");
                    goto cleanup;
                }
                if (ap_num == 0) {
                    ESP_LOGW(TAG, "No APs found");
                    goto cleanup;
                }
                
                // Log found APs
                ESP_LOGI(TAG, "Found %d APs:", ap_num);
                for (int i = 0; i < ap_num; i++) {
                    ESP_LOGI(TAG, "  %d: SSID: %s, RSSI: %d, Channel: %d, Auth: %d",
                            i+1,
                            (char *)ap_records[i].ssid,
                            ap_records[i].rssi,
                            ap_records[i].primary,
                            ap_records[i].authmode);
                }
                
cleanup:
                scan_done = true;
                
                // Switch back to AP mode if we were in it before
                if (was_ap_mode) {
                    ESP_LOGI(TAG, "Switching back to AP mode");
                    err = esp_wifi_set_mode(WIFI_MODE_AP);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "Failed to switch back to AP mode: %s", esp_err_to_name(err));
                    }
                    was_ap_mode = false;
                }
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        if (wifi_event_group != NULL) {
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

void wifi_init_sta(void) {
    wifi_event_group = xEventGroupCreate();    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Tạo và cấu hình network interface cho WiFi STA
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Read WiFi credentials from NVS
    char ssid[33] = {0}, pass[65] = {0};
    size_t ssid_len = sizeof(ssid), pass_len = sizeof(pass);
    nvs_get_str(nvs_handle_storage, "wifi_ssid", ssid, &ssid_len);
    nvs_get_str(nvs_handle_storage, "wifi_pass", pass, &pass_len);

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished");
}

void wifi_init_ap(void) {
    // Khởi tạo event group nếu chưa được tạo
    if (wifi_event_group == NULL) {
        wifi_event_group = xEventGroupCreate();
    }
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASSWORD,
            .max_connection = MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel = 1  // Sử dụng kênh 1 để tránh xung đột
        },
    };

    // Cấu hình chế độ hoạt động là APSTA để có thể scan trong khi vẫn làm AP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started. SSID:%s password:%s", AP_SSID, AP_PASSWORD);
    start_ap_led_blink(); // Bắt đầu nhấp nháy đèn khi vào AP mode
}

bool is_internet_available(void) {
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,    // Try both IPv4/IPv6
        .ai_socktype = SOCK_STREAM,
    };

    bool connected = false;
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    assert(netif != NULL);

    while (esp_netif_is_netif_up(netif) != true)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    struct addrinfo *res;
    int err = getaddrinfo("www.google.com", "80", &hints, &res);
    ESP_LOGI(TAG, "DNS lookup result: %p, %d", res, err);

    if (err != 0 || !res) {
        const char* error_msg = "Unknown DNS error";
        // Handle DNS errors in a compatible way
        if (err == EAI_AGAIN) {
            error_msg = "Temporary failure";
        } else if (err == EAI_FAIL) {
            error_msg = "Non-recoverable failure";
        } else if (err == EAI_NONAME) {
            error_msg = "Host not found";
        } else if (err == EAI_MEMORY) {
            error_msg = "Memory allocation failure";
        } else {
            // For other errors, use strerror
            error_msg = strerror(errno);
        }
        ESP_LOGE(TAG, "DNS lookup failed: %s (code=%d)", error_msg, err);
        return false;
    }

    // Try each returned address
    for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        int sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock < 0) continue;

        // Set timeout (5 seconds)
        struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        // Attempt connection
        if (connect(sock, p->ai_addr, p->ai_addrlen) == 0) {
            connected = true;
            close(sock);
            break;
        }
        close(sock);
    }

    freeaddrinfo(res);
    return connected;
}

esp_err_t start_wifi_scan(void) {
    esp_err_t ret;
    // First ensure WiFi is in a state where we can scan
    ret = ensure_wifi_ready_for_scan();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi not ready for scan: %s", esp_err_to_name(ret));
        return ret;
    }
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active = {
            .min = 150,
            .max = 300
        }
    };
    // Reset scan state
    scan_done = false;
    ap_count = 0;
    was_ap_mode = false;
    // Check if scan already in progress
    uint16_t number = 0;
    ret = esp_wifi_scan_get_ap_num(&number);
    if (ret == ESP_ERR_WIFI_STATE) {
        ESP_LOGW(TAG, "Scan already in progress");
        return ESP_ERR_WIFI_STATE;
    }
    // Stop any ongoing scan
    esp_wifi_scan_stop();
    // Ensure WiFi is ready for scan (e.g., not in the middle of a connection)
    ret = ensure_wifi_ready_for_scan();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi not ready for scan: %s", esp_err_to_name(ret));
        return ret;
    }
    // Check current WiFi mode
    wifi_mode_t current_mode;
    ret = esp_wifi_get_mode(&current_mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }
    // Switch to APSTA mode if needed
    if (current_mode == WIFI_MODE_AP) {
        was_ap_mode = true;
        ESP_LOGI(TAG, "Switching to APSTA mode for scanning...");
        ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to switch to APSTA mode: %s", esp_err_to_name(ret));
            return ret;
        }
        vTaskDelay(pdMS_TO_TICKS(MODE_SWITCH_DELAY_MS));
    }
    // Start scan with retry mechanism and handle STA connecting error
    int retry_count = 0;
    const int max_retries = 3;
    bool scan_started = false;
    TickType_t start_time = xTaskGetTickCount();
    while (!scan_started && retry_count < max_retries) {
        ESP_LOGI(TAG, "Starting WiFi scan... (attempt %d/%d)", retry_count + 1, max_retries);
        ret = esp_wifi_scan_start(&scan_config, false);
        if (ret == ESP_OK) {
            scan_started = true;
        } else if (ret == ESP_ERR_WIFI_STATE) {
            ESP_LOGW(TAG, "Scan start failed: ESP_ERR_WIFI_STATE (STA may be connecting), force disconnect and retry...");
            esp_wifi_disconnect();
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_wifi_set_mode(WIFI_MODE_APSTA);
            vTaskDelay(pdMS_TO_TICKS(200));
        } else {
            ESP_LOGW(TAG, "Scan start failed: %s, retrying...", esp_err_to_name(ret));
        }
        retry_count++;
        if (!scan_started) vTaskDelay(pdMS_TO_TICKS(300));
    }
    if (!scan_started) {
        ESP_LOGE(TAG, "Failed to start scan after %d attempts", max_retries);
        cleanup_scan_resources();
        return ESP_FAIL;
    }
    // Wait for scan completion with timeout
    while (!scan_done) {
        if ((xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS >= SCAN_TIMEOUT_MS) {
            ESP_LOGE(TAG, "Scan timeout after %d ms", SCAN_TIMEOUT_MS);
            esp_wifi_scan_stop();
            cleanup_scan_resources();
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return ESP_OK;
}

const wifi_ap_record_t* get_scanned_aps(uint16_t* count) {
    if (!scan_done) {
        ESP_LOGW(TAG, "Scan not completed yet, returning 0 APs");
        *count = 0;
        return NULL;
    }
    
    // Double-check that we have valid data
    if (ap_count == 0) {
        ESP_LOGW(TAG, "Scan completed but found 0 APs");
        *count = 0;
        return NULL;
    }
    
    ESP_LOGI(TAG, "Returning %d scanned APs", ap_count);
    *count = ap_count;
    return ap_records;
}

void wifi_manager_connect_from_nvs(void) {
    char ssid[33] = {0};
    char password[65] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(password);
    if (nvs_get_str(nvs_handle_storage, "wifi_ssid", ssid, &ssid_len) != ESP_OK) {
        ESP_LOGE(TAG, "No SSID in NVS");
        return;
    }
    nvs_get_str(nvs_handle_storage, "wifi_password", password, &pass_len);
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
}
