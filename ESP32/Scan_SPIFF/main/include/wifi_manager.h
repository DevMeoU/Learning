#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include "nvs.h"

// WiFi event group bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_SCANNING_BIT BIT1

// AP mode constants
#define AP_SSID "ESP32_CONFIG"
#define AP_PASSWORD "12345678"
#define MAX_CONNECTIONS 4

// WiFi scanning functions
esp_err_t start_wifi_scan(void);
const wifi_ap_record_t* get_scanned_aps(uint16_t* count);

// Event group để đồng bộ Wifi
extern EventGroupHandle_t wifi_event_group;
extern nvs_handle_t nvs_handle_storage;

// Các hàm chính
void wifi_init_sta(void);
void wifi_init_ap(void);
bool is_internet_available(void);

// Các hàm scan WiFi mới
esp_err_t start_wifi_scan(void);
const wifi_ap_record_t* get_scanned_aps(uint16_t* count);

// Hàm kết nối WiFi từ NVS (dùng cho web config)
void wifi_manager_connect_from_nvs(void);

#endif // WIFI_MANAGER_H
