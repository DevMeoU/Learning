#include "include/http_server.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_crt_bundle.h"
#include "mbedtls/pem.h"
#include "spiffs_handler.h"
#include "cJSON.h"
#include "esp_wifi.h"
#include "wifi_manager.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "common_types.h"
#include "time_utils.h"
#include "uart_handler.h"

#define POST_BUF_LEN 4096


static const char *TAG = "HTTP_SERVER";
static httpd_handle_t server = NULL;
extern nvs_handle_t nvs_handle_storage;
extern QueueHandle_t xSensorQueue;

// Global certificate storage
extern const uint8_t firebase_cert_pem_start[] asm("_binary_firebase_cert_pem_start");
extern const uint8_t firebase_cert_pem_end[] asm("_binary_firebase_cert_pem_end");

// Function prototypes
static void urldecode(char *src, char *dest, size_t max_len);
static esp_err_t root_get_handler(httpd_req_t *req);
static esp_err_t handle_get_wifi_scan(httpd_req_t *req);
static esp_err_t handle_get_config(httpd_req_t *req);

// Simplified send_to_firebase: caller provides firebase host and token separately
// full URL is built internally to include /data_stream.json?auth=<token>

// send_to_firebase: build URL with no double slashes and trim trailing slash
esp_err_t send_to_firebase(const char* firebase_host, const char* token, const char* post_data, esp_http_client_method_t method) {
    if (!firebase_host || !post_data) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // Trim trailing slash from host
    char host_trim[256];
    size_t hlen = strlen(firebase_host);
    while (hlen > 0 && firebase_host[hlen - 1] == '/') {
        hlen--;
    }
    if (hlen >= sizeof(host_trim)) {
        ESP_LOGE(TAG, "Host too long");
        return ESP_ERR_NO_MEM;
    }
    strncpy(host_trim, firebase_host, hlen);
    host_trim[hlen] = '\0';

    // Build final URL: include scheme if missing
    char full_url[256];
    int len = 0;
    if (strncmp(host_trim, "http", 4) == 0) {
        len = snprintf(full_url, sizeof(full_url), "%s/data_stream.json", host_trim);
    } else {
        len = snprintf(full_url, sizeof(full_url), "https://%s/data_stream.json", host_trim);
    }
    if (token && token[0]) {
        len += snprintf(full_url + len, sizeof(full_url) - len, "?auth=%s", token);
    }
    ESP_LOGI(TAG, "Final Firebase URL: %s", full_url);

    // Configure HTTP client with global CA store for TLS verification
    esp_http_client_config_t config = {
        .url = full_url,
        .timeout_ms = 15000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,  // enable server certificate verification
        .skip_cert_common_name_check = false,        // Kiểm tra CN/SAN
        .keep_alive_enable = true,                   // Giữ kết nối
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    // Optionally enforce common name match
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_method(client, method);

    esp_err_t err = ESP_FAIL;
    for (int i = 0; i < 3; ++i) {
        err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            int status = esp_http_client_get_status_code(client);
            ESP_LOGI(TAG, "HTTP status = %d", status);
            if (status >= 200 && status < 300) {
                break;
            }
            ESP_LOGW(TAG, "Server returned HTTP %d", status);
        } else {
            ESP_LOGW(TAG, "Request failed: %s", esp_err_to_name(err));
            int tls_code, tls_flags;
            esp_tls_get_and_clear_last_error(NULL, &tls_code, &tls_flags);
            ESP_LOGW(TAG, "TLS error: code=%d, flags=0x%x", tls_code, tls_flags);
        }
        vTaskDelay(pdMS_TO_TICKS(2000 << i));  // backoff
    }

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t send_sms_via_aws(const char* aws_url, const char* phone, const char* message) {
    if (!aws_url || !phone || !message) {
        ESP_LOGE(TAG, "Missing AWS parameters");
        return ESP_ERR_INVALID_ARG;
    }
    
    char post_data[256];
    snprintf(post_data, sizeof(post_data), "{\"phone\":\"%s\",\"message\":\"%s\"}", phone, message);
    
    esp_http_client_config_t config = {
        .url = aws_url,
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10000,
        .keep_alive_enable = false,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for AWS");
        return ESP_FAIL;
    }
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status >= 200 && status < 300) {
            ESP_LOGI(TAG, "SMS sent, status: %d", status);
        } else {
            ESP_LOGE(TAG, "SMS error, status: %d", status);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "SMS failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return err;
}

// Handler for / (root) to serve wifi_config.html from SPIFFS
static esp_err_t root_get_handler(httpd_req_t *req) {
    if (spiffs_file_exists("/wifi_config.html")) {
        return send_file_from_spiffs(req, "/wifi_config.html", "text/html");
    } else {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, "<html><body><h1>Configuration Page</h1><p>wifi_config.html not found!</p></body></html>", -1);
        return ESP_FAIL;
    }
}

static esp_err_t handle_get_wifi_scan(httpd_req_t *req) {
    ESP_LOGI(TAG, "Starting WiFi scan");
    
    esp_err_t ret = start_wifi_scan();
    if (ret != ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        if (ret == ESP_ERR_WIFI_STATE) {
            httpd_resp_set_status(req, "409 Conflict");
            httpd_resp_sendstr(req, "{\"error\":\"Scan in progress\"}");
        } else {
            httpd_resp_set_status(req, "500 Internal Server Error");
            char error_msg[100];
            snprintf(error_msg, sizeof(error_msg), "{\"error\":\"%s\"}", esp_err_to_name(ret));
            httpd_resp_sendstr(req, error_msg);
        }
        return ESP_OK;
    }
    
    // Wait for scan results
    int timeout_ms = 5000;
    int elapsed_ms = 0;
    
    while (elapsed_ms < timeout_ms) {
        uint16_t ap_count = 0;
        const wifi_ap_record_t* ap_records = get_scanned_aps(&ap_count);
        
        if (ap_records != NULL) {
            cJSON *root = cJSON_CreateObject();
            cJSON *networks = cJSON_CreateArray();
            cJSON_AddItemToObject(root, "networks", networks);

            for (int i = 0; i < ap_count; i++) {
                if (strlen((char *)ap_records[i].ssid) == 0) continue;
                
                cJSON *ap = cJSON_CreateObject();
                cJSON_AddStringToObject(ap, "ssid", (char *)ap_records[i].ssid);
                cJSON_AddNumberToObject(ap, "rssi", ap_records[i].rssi);
                cJSON_AddNumberToObject(ap, "channel", ap_records[i].primary);
                
                const char *auth_mode;
                switch (ap_records[i].authmode) {
                    case WIFI_AUTH_OPEN: auth_mode = "open"; break;
                    case WIFI_AUTH_WEP: auth_mode = "wep"; break;
                    case WIFI_AUTH_WPA_PSK: auth_mode = "wpa"; break;
                    case WIFI_AUTH_WPA2_PSK: auth_mode = "wpa2"; break;
                    case WIFI_AUTH_WPA_WPA2_PSK: auth_mode = "wpa/wpa2"; break;
                    case WIFI_AUTH_WPA3_PSK: auth_mode = "wpa3"; break;
                    default: auth_mode = "unknown";
                }
                cJSON_AddStringToObject(ap, "auth", auth_mode);
                cJSON_AddItemToArray(networks, ap);
            }

            char *json_str = cJSON_PrintUnformatted(root);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_sendstr(req, json_str);
            free(json_str);
            cJSON_Delete(root);
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed_ms += 100;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, "504 Gateway Timeout");
    httpd_resp_sendstr(req, "{\"error\":\"Scan timeout\"}");
    return ESP_OK;
}

static esp_err_t handle_get_config(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON root object");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Helper macro for safe NVS reads and JSON population
    #define ADD_NVS_STRING_TO_JSON(json_key, nvs_key, buf_size) do { \
        char buffer[buf_size] = {0}; \
        size_t len = sizeof(buffer); \
        esp_err_t ret = nvs_get_str(nvs_handle_storage, nvs_key, buffer, &len); \
        if (ret != ESP_OK) { \
            ESP_LOGW(TAG, "Failed to read %s from NVS: %s", nvs_key, esp_err_to_name(ret)); \
            buffer[0] = '\0'; \
        } \
        cJSON_AddStringToObject(root, json_key, buffer); \
    } while(0)

    // Add all configuration parameters to JSON
    ADD_NVS_STRING_TO_JSON("ssid", "wifi_ssid", 33);
    ADD_NVS_STRING_TO_JSON("token", "token", 128);
    ADD_NVS_STRING_TO_JSON("aws_url", "aws_url", 256);
    ADD_NVS_STRING_TO_JSON("phone_numbers", "phone_numbers", 256);
    ADD_NVS_STRING_TO_JSON("sms_messages", "sms_messages", 1024);
    ADD_NVS_STRING_TO_JSON("twilio_acc_sid", "twilio_acc_sid", 64);
    ADD_NVS_STRING_TO_JSON("twilio_token", "twilio_token", 64);
    ADD_NVS_STRING_TO_JSON("twilio_from_n", "twilio_from_n", 32);
    ADD_NVS_STRING_TO_JSON("twilio_url", "twilio_url", 256);
    ADD_NVS_STRING_TO_JSON("enable_twilio", "enable_twilio", 8);
    ADD_NVS_STRING_TO_JSON("enable_aws", "enable_aws", 8);

    // Special handling for Firebase URL with auto-https
    char firebase_url[128] = {0};
    size_t len = sizeof(firebase_url);
    esp_err_t ret = nvs_get_str(nvs_handle_storage, "firebase_url", firebase_url, &len);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read firebase_url from NVS: %s", esp_err_to_name(ret));
        firebase_url[0] = '\0';
    }
    
    // Auto-add https:// prefix if missing
    char fixed_url[150] = {0};
    if (firebase_url[0] != '\0' && strstr(firebase_url, "://") == NULL) {
        snprintf(fixed_url, sizeof(fixed_url), "https://%s", firebase_url);
    } else {
        strncpy(fixed_url, firebase_url, sizeof(fixed_url));
    }
    cJSON_AddStringToObject(root, "firebase_url", fixed_url);

    #undef ADD_NVS_STRING_TO_JSON

    // Generate JSON string
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);  // Delete cJSON object immediately after printing
    
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to print JSON");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Send response
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, json_str);
    
    // Free allocated JSON string
    free(json_str);
    
    return err;
}

static esp_err_t handle_post_config(httpd_req_t *req) {
    // 1) Đọc POST body
    char *buf = malloc(POST_BUF_LEN);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }
    int ret = httpd_req_recv(req, buf, POST_BUF_LEN - 1);
    if (ret <= 0) {
        free(buf);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // 2) Định nghĩa độ dài tối đa cho mỗi tham số
    enum {
        SSID_LEN               =  32 + 1,
        PASSWORD_LEN           =  64 + 1,
        FIREBASE_URL_LEN       = 256 + 1,
        TOKEN_LEN              = 128 + 1,
        AWS_URL_LEN            = 256 + 1,
        PHONE_NUMBERS_LEN      = 256 + 1,
        SMS_MESSAGES_LEN       = 512 + 1,
        TWILIO_SID_LEN         =  64 + 1,
        TWILIO_TOKEN_LEN       =  64 + 1,
        TWILIO_FROM_NUMBER_LEN =  32 + 1,
        TWILIO_URL_LEN         = 256 + 1,
        ENABLE_FLAG_LEN        =   8 + 1,
    };

    // 3) Trích xuất thô từ query key/value, luôn NUL‑terminate
    char raw_ssid[SSID_LEN]                         = {0};
    char raw_password[PASSWORD_LEN]                 = {0};
    char raw_firebase[FIREBASE_URL_LEN]             = {0};
    char raw_token[TOKEN_LEN]                       = {0};
    char raw_aws[AWS_URL_LEN]                       = {0};
    char raw_phones[PHONE_NUMBERS_LEN]              = {0};
    char raw_sms[SMS_MESSAGES_LEN]                  = {0};
    char raw_twilio_sid[TWILIO_SID_LEN]             = {0};
    char raw_twilio_token[TWILIO_TOKEN_LEN]         = {0};
    char raw_twilio_from_n[TWILIO_FROM_NUMBER_LEN]  = {0};
    char raw_twilio_url[TWILIO_URL_LEN]             = {0};
    char raw_enable_twilio[ENABLE_FLAG_LEN]         = {0};
    char raw_enable_aws[ENABLE_FLAG_LEN]            = {0};

    httpd_query_key_value(buf, "ssid",              raw_ssid,               SSID_LEN);
    httpd_query_key_value(buf, "password",          raw_password,           PASSWORD_LEN);
    httpd_query_key_value(buf, "firebase_url",      raw_firebase,           FIREBASE_URL_LEN);
    httpd_query_key_value(buf, "token",             raw_token,              TOKEN_LEN);
    httpd_query_key_value(buf, "aws_url",           raw_aws,                AWS_URL_LEN);
    httpd_query_key_value(buf, "phone_numbers",     raw_phones,             PHONE_NUMBERS_LEN);
    httpd_query_key_value(buf, "sms_messages",      raw_sms,                SMS_MESSAGES_LEN);
    httpd_query_key_value(buf, "twilio_acc_sid",    raw_twilio_sid,         TWILIO_SID_LEN);
    httpd_query_key_value(buf, "twilio_token",      raw_twilio_token,       TWILIO_TOKEN_LEN);
    httpd_query_key_value(buf, "twilio_from_n",     raw_twilio_from_n,      TWILIO_FROM_NUMBER_LEN);
    httpd_query_key_value(buf, "twilio_url",        raw_twilio_url,         TWILIO_URL_LEN);
    httpd_query_key_value(buf, "enable_twilio",     raw_enable_twilio,      ENABLE_FLAG_LEN);
    httpd_query_key_value(buf, "enable_aws",        raw_enable_aws,         ENABLE_FLAG_LEN);

    // 4) URL‑decode vào buffer riêng, cũng trên stack
    char ssid[SSID_LEN]                             = {0};
    char password[PASSWORD_LEN]                     = {0};
    char firebase_url[FIREBASE_URL_LEN]             = {0};
    char token[TOKEN_LEN]                           = {0};
    char aws_url[AWS_URL_LEN]                       = {0};
    char phone_numbers[PHONE_NUMBERS_LEN]           = {0};
    char sms_messages[SMS_MESSAGES_LEN]             = {0};
    char twilio_sid[TWILIO_SID_LEN]                 = {0};
    char twilio_token[TWILIO_TOKEN_LEN]             = {0};
    char twilio_from_n[TWILIO_FROM_NUMBER_LEN]      = {0};
    char twilio_url[TWILIO_URL_LEN]                 = {0};
    char enable_twilio[ENABLE_FLAG_LEN]             = {0};
    char enable_aws[ENABLE_FLAG_LEN]                = {0};

    urldecode(raw_ssid,           ssid,            sizeof(ssid));
    urldecode(raw_password,       password,        sizeof(password));
    urldecode(raw_firebase,       firebase_url,    sizeof(firebase_url));
    urldecode(raw_token,          token,           sizeof(token));
    urldecode(raw_aws,            aws_url,         sizeof(aws_url));
    urldecode(raw_phones,         phone_numbers,   sizeof(phone_numbers));
    urldecode(raw_sms,            sms_messages,    sizeof(sms_messages));
    urldecode(raw_twilio_sid,     twilio_sid,      sizeof(twilio_sid));
    urldecode(raw_twilio_token,   twilio_token,    sizeof(twilio_token));
    urldecode(raw_twilio_from_n,  twilio_from_n,   sizeof(twilio_from_n));
    urldecode(raw_twilio_url,     twilio_url,      sizeof(twilio_url));
    urldecode(raw_enable_twilio,  enable_twilio,   sizeof(enable_twilio));
    urldecode(raw_enable_aws,     enable_aws,      sizeof(enable_aws));

    // 5) Kiểm tra bắt buộc (ví dụ SSID, password, firebase_url, token phải không rỗng)
    if (!ssid[0] || !password[0] || !firebase_url[0] || !token[0]) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing required fields");
        free(buf);
        return ESP_FAIL;
    }
    // Kiểm tra format URL
    if (!strstr(firebase_url, "://")) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Firebase URL");
        free(buf);
        return ESP_FAIL;
    }

    // 6) Ghi vào NVS
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "NVS open failed");
        free(buf);
        return ESP_FAIL;
    }

    #define SET_AND_CHECK(key, val)                                    \
        do {                                                           \
            esp_err_t __e = nvs_set_str(nvs, key, val);                \
            if (__e != ESP_OK) {                                       \
                ESP_LOGE(TAG, "nvs_set_str(%s) failed: %s", key, esp_err_to_name(__e)); \
            }                                                          \
        } while (0)

    SET_AND_CHECK("wifi_ssid",        ssid);
    SET_AND_CHECK("wifi_password",    password);
    SET_AND_CHECK("firebase_url",     firebase_url);
    SET_AND_CHECK("token",            token);
    SET_AND_CHECK("aws_url",          aws_url);
    SET_AND_CHECK("phone_numbers",    phone_numbers);
    SET_AND_CHECK("sms_messages",     sms_messages);
    SET_AND_CHECK("twilio_acc_sid",   twilio_sid);
    SET_AND_CHECK("twilio_token",     twilio_token);
    SET_AND_CHECK("twilio_from_n",    twilio_from_n);
    SET_AND_CHECK("twilio_url",       twilio_url);
    SET_AND_CHECK("enable_twilio",    enable_twilio);
    SET_AND_CHECK("enable_aws",       enable_aws);

    err = nvs_commit(nvs);
    nvs_close(nvs);
    #undef SET_AND_CHECK

    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "NVS commit failed");
        free(buf);
        return ESP_FAIL;
    }

    // 7) Phản hồi và reboot
    httpd_resp_sendstr(req, "All configurations saved");
    free(buf);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

static void urldecode(char *src, char *dest, size_t max_len) {
    size_t src_len = strlen(src);
    size_t i, j = 0;
    char a, b;

    for (i = 0; i < src_len && j < max_len - 1; i++) {
        if (src[i] == '%' && i + 2 < src_len) {
            a = src[i + 1];
            b = src[i + 2];
            if (isxdigit(a) && isxdigit(b)) {
                dest[j++] = (a >= 'A' ? (a - 'A' + 10) : (a - '0')) * 16 +
                           (b >= 'A' ? (b - 'A' + 10) : (b - '0'));
                i += 2;
                continue;
            }
        } else if (src[i] == '+') {
            dest[j++] = ' ';
            continue;
        }
        dest[j++] = src[i];
    }
    dest[j] = '\0';
}

esp_err_t start_webserver(void) {
    if (server != NULL) {
        ESP_LOGI(TAG, "Web server already running");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting web server on port: %d", config.server_port);
    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register URI handlers
    // Cập nhật handlers array
    httpd_uri_t handlers[] = {
    { .uri = "/", .method = HTTP_GET, .handler = root_get_handler },
    { .uri = "/scan", .method = HTTP_GET, .handler = handle_get_wifi_scan },
    { .uri = "/config", .method = HTTP_GET, .handler = handle_get_config },
    { .uri = "/config", .method = HTTP_POST, .handler = handle_post_config }
    };

    for (int i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++) {
        httpd_register_uri_handler(server, &handlers[i]);
    }

    ESP_LOGI(TAG, "Web server started successfully");
    return ESP_OK;
}

void stop_webserver(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    }
}

void http_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting HTTP task");
    esp_err_t ret = start_webserver();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Web server init failed");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        data_frame_t recv_data;
        if (xQueueReceive(xSensorQueue, &recv_data, portMAX_DELAY) == pdTRUE) {
            cJSON *root = cJSON_CreateObject();
            if (!root) continue;

            cJSON_AddNumberToObject(root, "device_id", 1);
            cJSON_AddStringToObject(root, "time", get_current_time());
            cJSON_AddNumberToObject(root, "timestamp", get_current_timestamp());
            cJSON_AddStringToObject(root, "timezone", TIMEZONE);
            cJSON_AddNumberToObject(root, "led_warning", recv_data.led_status);
            cJSON_AddNumberToObject(root, "temperature_sensor_real", recv_data.temperature.f_sensor_value);
            cJSON_AddNumberToObject(root, "temperature_sensor_fake", recv_data.temperature_fake.f_sensor_value);

            
            char *post_data = cJSON_PrintUnformatted(root);
            if (!post_data) {
                cJSON_Delete(root);
                continue;
            }
            
            // Get server config
            char *firebase_url = NULL;
            char *token = NULL;
            size_t len = 0;
            
            if (nvs_get_str(nvs_handle_storage, "firebase_url", NULL, &len) == ESP_OK) {
                firebase_url = malloc(len);
                if (firebase_url) nvs_get_str(nvs_handle_storage, "firebase_url", firebase_url, &len);
            }
            
            if (nvs_get_str(nvs_handle_storage, "token", NULL, &len) == ESP_OK) {
                token = malloc(len);
                if (token) nvs_get_str(nvs_handle_storage, "token", token, &len);
            }
            
            if (firebase_url && token && post_data) 
            {
                if (strstr(firebase_url, "://") == NULL) {
                    ESP_LOGE(TAG, "Invalid Firebase URL: %s", firebase_url);
                } else {
                    if (send_to_firebase(firebase_url, token, post_data, HTTP_METHOD_PUT) == ESP_OK)
                    {
                        ESP_LOGI(TAG, "Firebase post successful");
                    }       
                    else
                    {
                        ESP_LOGE(TAG, "Firebase post failed");
                    }
                }
            }
            
            // Cleanup
            free(firebase_url);
            free(token);
            free(post_data);
            cJSON_Delete(root);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
