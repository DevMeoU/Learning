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

// Cấu hình UART
#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_RX_PIN 4
#define UART_TX_PIN 5
#define UART_BUF_SIZE 1024
#define MAX_PAYLOAD        4

// Cấu hình GPIO
#define BUTTON_GPIO GPIO_NUM_16 // GPIO cho nút nhấn

// Cấu hình WiFi
#define WIFI_SSID "HAPPY HOME floor4"
#define WIFI_PASS "H@ppyhome4"
static char *server_url = NULL;

// Cấu hình AP mode
#define AP_SSID      "ESP32_Config"
#define AP_PASSWORD  "123456789"
#define CONFIG_PORT  80

static const char *TAG = "UART_HTTP";
QueueHandle_t xSensorQueue;

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

nvs_handle_t nvs_handle_storage;

typedef struct {
    float temperature;
    float humidity;
} sensor_data_t;

// Prototypes
void wifi_init_sta(void);
void wifi_init_ap(void);
static esp_err_t post_handler(httpd_req_t *req);
static esp_err_t index_handler(httpd_req_t *req);
void urldecode(char *src, char *dest, size_t max_len);
void print_hex(const uint8_t *data, size_t len);

// Thêm HTML form
static const char* HTML_FORM = 
"<html>"
"<head>"
"    <title>ESP32 Configuration</title>"
"    <meta charset=\"UTF-8\">"
"</head>"
"<body>"
"    <h1>ESP32 Server Configuration</h1>"
"    <form method='post' action='/config' enctype='application/x-www-form-urlencoded'>"
"        <label>Server URL: </label>"
"        <input type='text' name='server_url' required><br><br>"
"        <input type='submit' value='Save'>"
"    </form>"
"</body>"
"</html>";

// Thêm handler cho GET request tới root
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, HTML_FORM, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
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

void fake_data_task(void *pvParameters) {
    sensor_data_t sensor_data;
    srand(time(NULL));

    while (1) {
        sensor_data.temperature = rand() % 100;
        sensor_data.humidity = rand() % 100;

        ESP_LOGI(TAG, "Temperature: %.2f, Humidity: %.2f", 
                sensor_data.temperature, sensor_data.humidity);

        xQueueSend(xSensorQueue, &sensor_data, portMAX_DELAY);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void uart_parser_task(void *pvParameters) {
    uint8_t rx_buf[UART_BUF_SIZE];
    uint8_t state = 0;
    uint8_t payload_len = 0;
    uint8_t payload_cnt = 0;
    uint8_t payload[MAX_PAYLOAD];
    uint8_t checksum = 0;
    ESP_LOGI(TAG, "UART Parser Task started");
    // Bắt đầu đọc dữ liệu từ UART

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, rx_buf, UART_BUF_SIZE, pdMS_TO_TICKS(100));
        // ESP_LOGI(TAG, "Received %d bytes:", len);
        if (len > 0)
        {
            printf("Receive data: ");
            print_hex(rx_buf, len);
        }
        else
        {
            ESP_LOGW(TAG, "Waiting for data...");
            continue; // Không có dữ liệu mới, tiếp tục vòng lặp
        }
        
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
        // Nếu không có dữ liệu mới, đợi một chút
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void http_task(void *pvParameters) {
    sensor_data_t sensor_data;

    ESP_LOGI(TAG, "HTTP Task started");
    // Đọc server_url từ NVS

    while (1) {
        if (xQueueReceive(xSensorQueue, &sensor_data, portMAX_DELAY)) {
            if (server_url == NULL) {
                ESP_LOGE(TAG, "Server URL not configured!");
                continue;
            }

            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "device", "ESP32");
            cJSON_AddNumberToObject(root, "temperature", sensor_data.temperature);
            cJSON_AddNumberToObject(root, "humidity", sensor_data.humidity);

            char *json_str = cJSON_Print(root);
            
            esp_http_client_config_t config = {
                .url = server_url,
                .method = HTTP_METHOD_POST,
                .timeout_ms = 5000
            };

            esp_http_client_handle_t client = esp_http_client_init(&config);
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_post_field(client, json_str, strlen(json_str));

            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "HTTP Status: %d", esp_http_client_get_status_code(client));
            } else {
                ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
            }

            esp_http_client_cleanup(client);
            cJSON_Delete(root);
            free(json_str);
        }
    }
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
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP Mode Started. SSID: %s Password: %s", AP_SSID, AP_PASSWORD);
}

static esp_err_t post_handler(httpd_req_t *req) {
    char buf[256];
    char decoded_buf[256];
    char *server_url_value = NULL;
    
    // Nhận dữ liệu
    int ret = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (ret <= 0) {
        ESP_LOGE(TAG, "Receive failed: %d", ret);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Giải mã URL encoded data
    urldecode(buf, decoded_buf, sizeof(decoded_buf));
    
    // Parse form data (dạng: server_url=value)
    server_url_value = strstr(decoded_buf, "server_url=");
    if (!server_url_value) {
        ESP_LOGE(TAG, "Missing server_url field");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing server_url");
        return ESP_FAIL;
    }
    
    server_url_value += strlen("server_url=");
    char *end = strchr(server_url_value, '&');
    if (end) *end = '\0';

    // Lưu vào NVS
    esp_err_t err = nvs_set_str(nvs_handle_storage, "server_url", server_url_value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS save failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Save failed");
        return ESP_FAIL;
    }
    nvs_commit(nvs_handle_storage);

    // Cập nhật server_url
    if (server_url) free(server_url);
    server_url = strdup(server_url_value);
    
    // Phản hồi
    httpd_resp_send(req, "Configuration saved! Rebooting...", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    return ESP_OK;
}

// Hàm giải mã URL encoding
void urldecode(char *src, char *dest, size_t max_len) {
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

void start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        // Handler cho GET request tới root
        httpd_uri_t index_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_handler
        };
        
        // Handler cho POST request tới /config
        httpd_uri_t config_uri = {
            .uri = "/config",
            .method = HTTP_POST,
            .handler = post_handler
        };

        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &config_uri);
    }
}

void app_main() {
    // Khởi tạo NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Mở NVS storage
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle_storage));

    // Cấu hình GPIO cho nút nhấn
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,    // Sử dụng pull-up nội
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    bool enter_ap_mode = false;
    int timeout = 5; // 5 giây

    ESP_LOGI(TAG, "Press and hold button for %d seconds to enter AP mode...", timeout);

    // Đếm ngược 10 giây chờ nút nhấn
    for (int i = timeout; i > 0; i--) {
        if (gpio_get_level(BUTTON_GPIO) == 0) { // 0 = nút được nhấn (do pull-up)
            enter_ap_mode = true;
            ESP_LOGI(TAG, "Button pressed! Entering AP mode...");
            break;
        }
        ESP_LOGI(TAG, "Timeout: %d seconds remaining", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // Xử lý sau khi hết thời gian
    if (!enter_ap_mode) {
        ESP_LOGI(TAG, "No button press detected");
        // Kiểm tra cấu hình đã lưu
        size_t required_size;
        if (nvs_get_str(nvs_handle_storage, "server_url", NULL, &required_size) == ESP_OK) {
            server_url = malloc(required_size);
            nvs_get_str(nvs_handle_storage, "server_url", server_url, &required_size);
            ESP_LOGI(TAG, "Connecting to saved WiFi...");
            wifi_init_sta();
        } else {
            ESP_LOGI(TAG, "No saved configuration found");
            enter_ap_mode = true;
        }
    }

    // Khởi động AP mode nếu cần
    if (enter_ap_mode) {
        wifi_init_ap();
        start_webserver();
    }

    // Khởi tạo các task chính
    uart_init();
    xSensorQueue = xQueueCreate(5, sizeof(sensor_data_t));
    // xTaskCreate(fake_data_task, "fake_data_task", 4096, NULL, 5, NULL);
    xTaskCreate(uart_parser_task, "uart_parser", 4096, NULL, 5, NULL);
    xTaskCreate(http_task, "http_task", 4096, NULL, 4, NULL);
}

void print_hex(const uint8_t *data, size_t len) {
    for(int i=0; i<len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}