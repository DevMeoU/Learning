#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "lwip/netdb.h"
// Thêm các thư viện cần thiết
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/dns.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_spiffs.h"
#include "esp_sntp.h"

// Khai báo external certificate
extern const uint8_t firebase_cert_pem_start[] asm("_binary_firebase_cert_pem_start");
extern const uint8_t firebase_cert_pem_end[]   asm("_binary_firebase_cert_pem_end");

// Cấu hình UART
#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_RX_PIN     4
#define UART_TX_PIN     5
#define UART_BUF_SIZE 1024
#define MAX_PAYLOAD     8

// Cấu hình GPIO
#define BUTTON_GPIO         GPIO_NUM_16 // GPIO cho nút nhấn
#define LED_NOTIFICATION    GPIO_NUM_2 // GPIO cho LED thông báo
#define LED_NOTIFICATION_ON  gpio_set_level(LED_NOTIFICATION, 1)
#define LED_NOTIFICATION_OFF gpio_set_level(LED_NOTIFICATION, 0)

#define LED_GREEN           GPIO_NUM_15 // GPIO cho LED xanh
#define LED_RED             GPIO_NUM_13 // GPIO cho LED đỏ
#define LED_YELLOW          GPIO_NUM_12 // GPIO cho LED vàng

#define LED_GREEN_ON        gpio_set_level(LED_GREEN, 1)
#define LED_GREEN_OFF       gpio_set_level(LED_GREEN, 0)
#define LED_RED_ON          gpio_set_level(LED_RED, 1)
#define LED_RED_OFF         gpio_set_level(LED_RED, 0)
#define LED_YELLOW_ON       gpio_set_level(LED_YELLOW, 1)
#define LED_YELLOW_OFF      gpio_set_level(LED_YELLOW, 0)

#define SET_ALL_LED(__lg__, __lr__, __ly__) { \
    gpio_set_level(LED_GREEN, __lg__); \
    gpio_set_level(LED_RED, __lr__); \
    gpio_set_level(LED_YELLOW, __ly__); \
}

// Cấu hình WiFi
#define WIFI_SSID "HAPPY HOME floor4"
#define WIFI_PASS "H@ppyhome4"
// static char *server_url = NULL; // Removed unused static variable

// Cấu hình AP mode
#define AP_SSID      "ESP32_Config"
#define AP_PASSWORD  "123456789"
#define CONFIG_PORT  80

// Cấu hình múi giờ (ví dụ: Asia/Ha_Noi, UTC+7)
#define TIMEZONE "ICT-7"

static const char *TAG = "UART_HTTP";
QueueHandle_t xSensorQueue;

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

nvs_handle_t nvs_handle_storage;

typedef struct {
    uint8_t led_green;
    uint8_t led_red;
    uint8_t led_yellow;
    uint8_t reserved; // Dùng để căn chỉnh dữ liệu cơ bản
    union sensor_data {
        uint32_t u32_sensor_value;
        float f_sensor_value;
    } sensor_data;
} data_frame_t;

// Prototypes
void wifi_init_sta(void);
void wifi_init_ap(void);
static esp_err_t http_event_handler(esp_http_client_event_t *evt);
static void urldecode(char *src, char *dest, size_t max_len);
void print_hex(const uint8_t *data, size_t len);
bool is_internet_available();
esp_err_t send_file_from_spiffs(httpd_req_t *req, const char *path, const char *content_type);
static esp_err_t handle_get_wifi_scan(httpd_req_t *req);
const char *get_current_time(void); // Prototype for get_current_time function
void time_sync_notification_cb(struct timeval *tv); // Prototype for SNTP callback

// Thêm handler cho GET request tới root
static esp_err_t handle_get_wifi_config_html(httpd_req_t *req) {
    return send_file_from_spiffs(req, "/spiffs/wifi_config.html", "text/html");
}

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
}

void uart_parser_task(void *pvParameters) {
    uint8_t rx_buf[UART_BUF_SIZE];
    uint8_t state = 0;
    uint8_t payload_len = 0;
    uint8_t payload_cnt = 0;
    uint8_t payload[MAX_PAYLOAD];
    uint8_t checksum = 0;
    ESP_LOGI(TAG, "UART Parser Task started");

    while (1) {
        LED_NOTIFICATION_ON;
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

void http_task(void *pvParameters) {
    data_frame_t recv_data;
    char *server_url = NULL;
    size_t required_size;

    ESP_LOGI(TAG, "HTTP Task started");
    
    // Đọc server_url từ NVS
    if (nvs_get_str(nvs_handle_storage, "server_url", NULL, &required_size) == ESP_OK) {
        server_url = malloc(required_size);
        nvs_get_str(nvs_handle_storage, "server_url", server_url, &required_size);
        ESP_LOGI(TAG, "Loaded server URL from NVS: %s", server_url);
    }

    while (1) {
        if (xQueueReceive(xSensorQueue, &recv_data, portMAX_DELAY)) {
            // Trong http_task trước khi gửi request
            if (!is_internet_available()) {
                ESP_LOGW(TAG, "No internet connection, skipping HTTP request");
                continue;
            }
            if (!server_url || strlen(server_url) == 0) {
                ESP_LOGE(TAG, "server_url is NULL or empty, skipping HTTP request");
                continue;
            }
            const char *auth_token = "V2yKXVusT10Fa6BRLP6TgiZOKS0BUI5SqVwwSwUr";
            char full_url[256];
            snprintf(full_url, sizeof(full_url), "%s/data_stream.json?auth=%s", server_url, auth_token);

            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "device", "ESP32-DEESOL");
            cJSON_AddStringToObject(root, "time", get_current_time());
            cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
            cJSON_AddStringToObject(root, "timezone", TIMEZONE); // Giả sử múi giờ là UTC+7
            cJSON_AddNumberToObject(root, "led_green", recv_data.led_green);
            cJSON_AddNumberToObject(root, "led_yellow", recv_data.led_yellow);
            cJSON_AddNumberToObject(root, "led_red", recv_data.led_red);
            cJSON_AddNumberToObject(root, "sensor_data", recv_data.sensor_data.f_sensor_value);

            char *json_str = cJSON_Print(root);
            
            esp_http_client_config_t config = {
                .url = full_url,
                .method = HTTP_METHOD_PUT,
                .timeout_ms = 15000,
                .cert_pem = (const char *)firebase_cert_pem_start,
                .common_name = "deesolcloud-default-rtdb.asia-southeast1.firebasedatabase.app", // Sử dụng common_name
                .transport_type = HTTP_TRANSPORT_OVER_SSL,
                .keep_alive_enable = false,
                .disable_auto_redirect = true,
                .event_handler = http_event_handler,
            };

            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_post_field(client, json_str, strlen(json_str));

            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK) {
                int status_code = esp_http_client_get_status_code(client);
                ESP_LOGI(TAG, "HTTP Status: %d", status_code);
                
                if (status_code == 200 || status_code == 201) {
                    ESP_LOGI(TAG, "Firebase update successful");
                } else {
                    char response[256] = {0};
                    int read_len = esp_http_client_read(client, response, sizeof(response) - 1);
                    ESP_LOGE(TAG, "Firebase error %d: %.*s", 
                            status_code, read_len, response);
                }
            } else {
                ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
                
                if (err == ESP_ERR_HTTP_CONNECT) {
                    ESP_LOGE(TAG, "Connection failed - check network or DNS");
                }
                
                int esp_tls_code = 0;
                int esp_tls_flags = 0;
                esp_err_t tls_err = esp_tls_get_and_clear_last_error(NULL, &esp_tls_code, &esp_tls_flags);
                
                if (tls_err == ESP_OK) {
                    ESP_LOGE(TAG, "TLS error code: 0x%x", esp_tls_code);
                    ESP_LOGE(TAG, "TLS flags: 0x%x", esp_tls_flags);
                } else {
                    ESP_LOGE(TAG, "Failed to get TLS error: %s", esp_err_to_name(tls_err));
                }
            }

            esp_http_client_cleanup(client);
            cJSON_Delete(root);
            free(json_str);

            // LED_NOTIFICATION_OFF; // Tắt LED thông báo sau khi gửi dữ liệu
        }
    }
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT: // Thêm case còn thiếu
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void) {
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Đọc SSID và password từ NVS
    char ssid[33] = {0};
    char pass[65] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(pass);
    nvs_get_str(nvs_handle_storage, "wifi_ssid", ssid, &ssid_len);
    nvs_get_str(nvs_handle_storage, "wifi_pass", pass, &pass_len);

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                         WIFI_CONNECTED_BIT,
                                         pdFALSE,
                                         pdFALSE,
                                         pdMS_TO_TICKS(30000));
    if (bits & WIFI_CONNECTED_BIT) {
        esp_netif_ip_info_t ip_info;
        ESP_ERROR_CHECK(esp_netif_get_ip_info(sta_netif, &ip_info));
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&ip_info.ip));
    } else {
        ESP_LOGE(TAG, "Failed to connect WiFi!");
        esp_restart();
    }
}

void wifi_init_ap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASSWORD,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    
    ESP_ERROR_CHECK(esp_wifi_stop());
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "AP Mode Started. SSID: %s Password: %s", AP_SSID, AP_PASSWORD);
}

// Đọc file HTML từ SPIFFS và trả về client
esp_err_t send_file_from_spiffs(httpd_req_t *req, const char *path, const char *content_type) {
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, content_type);
    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, n);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// API scan WiFi trả về JSON


// Handler POST /config nhận ssid, password, server_url, token
static esp_err_t handle_post_wifi_config(httpd_req_t *req) {
    char buf[512];
    char decoded_buf[512];
    int ret = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (ret <= 0) {
        ESP_LOGE(TAG, "Receive failed: %d", ret);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    // Kiểm tra Content-Type
    char content_type[32];
    httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type));
    char ssid[64] = {0}, password[64] = {0}, server_url[128] = {0}, token[128] = {0};
    if (strstr(content_type, "application/json")) {
        // Parse JSON
        cJSON *root = cJSON_Parse(buf);
        if (!root) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
            return ESP_FAIL;
        }
        cJSON *jssid = cJSON_GetObjectItem(root, "ssid");
        cJSON *jpass = cJSON_GetObjectItem(root, "password");
        cJSON *jurl = cJSON_GetObjectItem(root, "server_url");
        cJSON *jtoken = cJSON_GetObjectItem(root, "token");
        if (!jssid || !jurl || !jtoken) {
            cJSON_Delete(root);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing field");
            return ESP_FAIL;
        }
        strncpy(ssid, jssid->valuestring, sizeof(ssid)-1);
        if (jpass) strncpy(password, jpass->valuestring, sizeof(password)-1);
        strncpy(server_url, jurl->valuestring, sizeof(server_url)-1);
        strncpy(token, jtoken->valuestring, sizeof(token)-1);
        cJSON_Delete(root);
    } else {
        // Parse x-www-form-urlencoded
        urldecode(buf, decoded_buf, sizeof(decoded_buf));
        char *p;
        p = strstr(decoded_buf, "ssid=");
        if (p) { p += 5; char *e = strchr(p, '&'); size_t l = e ? (size_t)(e-p) : strlen(p); strncpy(ssid, p, l); ssid[l] = 0; }
        p = strstr(decoded_buf, "password=");
        if (p) { p += 9; char *e = strchr(p, '&'); size_t l = e ? (size_t)(e-p) : strlen(p); strncpy(password, p, l); password[l] = 0; }
        p = strstr(decoded_buf, "server_url=");
        if (p) { p += 11; char *e = strchr(p, '&'); size_t l = e ? (size_t)(e-p) : strlen(p); strncpy(server_url, p, l); server_url[l] = 0; }
        p = strstr(decoded_buf, "token=");
        if (p) { p += 6; char *e = strchr(p, '&'); size_t l = e ? (size_t)(e-p) : strlen(p); strncpy(token, p, l); token[l] = 0; }
        if (!ssid[0] || !server_url[0] || !token[0]) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing field");
            return ESP_FAIL;
        }
    }
    // Lưu vào NVS
    nvs_set_str(nvs_handle_storage, "wifi_ssid", ssid);
    nvs_set_str(nvs_handle_storage, "wifi_pass", password);
    nvs_set_str(nvs_handle_storage, "server_url", server_url);
    nvs_set_str(nvs_handle_storage, "token", token);
    nvs_commit(nvs_handle_storage);
    // Cập nhật biến runtime
    // ... (tùy logic của bạn)
    // Trả về JSON nếu là JSON, còn lại trả về text
    if (strstr(content_type, "application/json")) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"success\":true}", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send(req, "Configuration saved! Rebooting...", HTTPD_RESP_USE_STRLEN);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

// Sửa start_webserver để đăng ký đúng handler
void start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192; // Tăng stack cho HTTP server để tránh overflow khi scan WiFi
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t index_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = handle_get_wifi_config_html
        };
        httpd_uri_t config_uri = {
            .uri = "/config",
            .method = HTTP_POST,
            .handler = handle_post_wifi_config
        };
        httpd_uri_t scan_uri = {
            .uri = "/scan",
            .method = HTTP_GET,
            .handler = handle_get_wifi_scan
        };
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &config_uri);
        httpd_register_uri_handler(server, &scan_uri);
    }
}

// This function has been moved to main.c
// The original app_main has been removed to avoid conflicts
// All functionality is now called from the main.c file

void print_hex(const uint8_t *data, size_t len) {
    for(int i=0; i<len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

bool is_internet_available() {
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };
    struct addrinfo *res;
    
    int err = getaddrinfo("google.com", "80", &hints, &res);
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed: %d", err);
        return false;
    }
    
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return false;
    }
    
    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        close(sock);
        freeaddrinfo(res);
        return false;
    }
    
    close(sock);
    freeaddrinfo(res);
    return true;
}

static void urldecode(char *src, char *dest, size_t max_len) {
    char a, b;
    size_t i = 0;
    while (*src && i < max_len-1) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dest++ = 16*a + b;
            src += 3;
        } else if (*src == '+') {
            *dest++ = ' ';
            src++;
        } else {
            *dest++ = *src++;
        }
        i++;
    }
    *dest = '\0';
}

static esp_err_t handle_get_wifi_scan(httpd_req_t *req) {
    wifi_ap_record_t ap_records[20];
    uint16_t ap_num = 20; // Số lượng AP tối đa cần lấy
    
    // Lưu lại chế độ WiFi hiện tại
    wifi_mode_t original_mode;
    esp_err_t err = esp_wifi_get_mode(&original_mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi mode: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get WiFi mode");
        return ESP_FAIL;
    }    // Tạm thời chuyển sang APSTA mode nếu cần để duy trì kết nối AP trong khi quét
    bool mode_changed = false;
    if (original_mode == WIFI_MODE_AP) {  // Chỉ thay đổi nếu đang ở chế độ AP
        ESP_LOGI(TAG, "Switching to APSTA mode for scanning while maintaining AP connections");
        err = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(err));
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to set WiFi mode");
            return ESP_FAIL;
        }
        mode_changed = true;
        vTaskDelay(pdMS_TO_TICKS(500)); // Đợi WiFi ổn định, giảm thời gian chờ để tránh timeout
    }

    // Đảm bảo WiFi đã được khởi động
    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_STOPPED) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(err));
        if (mode_changed) esp_wifi_set_mode(original_mode);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to start WiFi");
        return ESP_FAIL;
    }

    // Dừng mọi quá trình quét đang diễn ra
    esp_wifi_scan_stop();
    vTaskDelay(pdMS_TO_TICKS(200));

    // Cấu hình quét WiFi (đơn giản hóa)
    wifi_scan_config_t scan_config = {
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = { .active = { .min = 100, .max = 300 } }
    };

    // Bắt đầu quét
    err = esp_wifi_scan_start(&scan_config, true); // true = chờ quét hoàn tất
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(err));
        if (mode_changed) esp_wifi_set_mode(original_mode);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "WiFi scan failed to start");
        return ESP_FAIL;
    }

    // Lấy kết quả quét
    err = esp_wifi_scan_get_ap_records(&ap_num, ap_records);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Get AP records failed: %s", esp_err_to_name(err));
        if (mode_changed) esp_wifi_set_mode(original_mode);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get scan results");
        return ESP_FAIL;
    }

    // Tạo JSON response
    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < ap_num; i++) {
        cJSON *ap = cJSON_CreateObject();
        cJSON_AddStringToObject(ap, "ssid", (char*)ap_records[i].ssid);
        cJSON_AddNumberToObject(ap, "rssi", ap_records[i].rssi);
        cJSON_AddNumberToObject(ap, "channel", ap_records[i].primary);
        cJSON_AddItemToArray(root, ap);
    }

    char *json_str = cJSON_Print(root);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(root);
        if (mode_changed) esp_wifi_set_mode(original_mode);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON creation failed");
        return ESP_FAIL;
    }

    // Gửi response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    // Dọn dẹp
    free(json_str);
    cJSON_Delete(root);

    // Khôi phục chế độ WiFi ban đầu
    if (mode_changed) {
        esp_wifi_set_mode(original_mode);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    return ESP_OK;
}

// Khởi tạo SNTP để đồng bộ thời gian
void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
    
    // Thiết lập múi giờ
    setenv("TZ", TIMEZONE, 1);
    tzset();
}

// SNTP time sync notification callback
void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Time synced with NTP server");
}

// Hàm lấy thời gian hiện tại dưới dạng chuỗi
const char *get_current_time(void) {
    time_t now;
    struct tm timeinfo;
    static char time_str[64];

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);

    return time_str;
}