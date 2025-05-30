#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "WIFI_SCAN";

void scan_wifi() {
    // Khởi tạo cấu hình WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Khởi tạo WiFi ở chế độ Station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Cấu hình tham số scan
    wifi_scan_config_t scan_config = {
        .ssid = NULL,          // Scan tất cả SSID
        .bssid = NULL,         // Scan tất cả BSSID
        .channel = 0,          // Scan tất cả kênh
        .show_hidden = true,   // Hiển thị mạng ẩn
        .scan_type = WIFI_SCAN_TYPE_ACTIVE, // Active scanning
        .scan_time = {
            .active = {
                .min = 100,    // Thời gian scan tối thiểu (ms)
                .max = 300     // Thời gian scan tối đa (ms)
            }
        }
    };

    // Bắt đầu quét
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    ESP_LOGI(TAG, "Scanning WiFi networks...");

    // Lấy số lượng AP tìm thấy
    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Found %d access points:", ap_count);

    if(ap_count == 0) {
        ESP_LOGI(TAG, "No AP found");
        return;
    }

    // Cấp phát bộ nhớ cho danh sách AP
    wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));

    // In thông tin các AP
    for(int i = 0; i < ap_count; i++) {
        ESP_LOGI(TAG, "SSID: \t\t%s", ap_list[i].ssid);
        ESP_LOGI(TAG, "RSSI: \t\t%d dBm", ap_list[i].rssi);
        ESP_LOGI(TAG, "Channel: \t%d", ap_list[i].primary);
        ESP_LOGI(TAG, "Auth: \t\t%s", 
                (ap_list[i].authmode == WIFI_AUTH_OPEN) ? "Open" :
                (ap_list[i].authmode == WIFI_AUTH_WEP) ? "WEP" :
                (ap_list[i].authmode == WIFI_AUTH_WPA_PSK) ? "WPA-PSK" :
                (ap_list[i].authmode == WIFI_AUTH_WPA2_PSK) ? "WPA2-PSK" :
                (ap_list[i].authmode == WIFI_AUTH_WPA_WPA2_PSK) ? "WPA/WPA2-PSK" :
                "Unknown");
        ESP_LOGI(TAG, "------------------------");
    }

    // Giải phóng bộ nhớ
    free(ap_list);
}

void app_main() {
    // Khởi tạo NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Khởi tạo network interface
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Chạy hàm scan WiFi
    scan_wifi();
}