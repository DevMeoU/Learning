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

static esp_err_t handle_post_config(httpd_req_t *req) {
    char buf[1024];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = 0;

    // Parse all configuration parameters
    char ssid[33] = {0};
    char password[65] = {0};
    char firebase_url[129] = {0};
    char token[129] = {0};
    char aws_url[256] = {0};
    char phone_numbers[256] = {0};
    char sms_messages[1024] = {0};

    httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid));
    httpd_query_key_value(buf, "password", password, sizeof(password));
    httpd_query_key_value(buf, "firebase_url", firebase_url, sizeof(firebase_url));
    httpd_query_key_value(buf, "token", token, sizeof(token));
    httpd_query_key_value(buf, "aws_url", aws_url, sizeof(aws_url));
    httpd_query_key_value(buf, "phone_numbers", phone_numbers, sizeof(phone_numbers));
    httpd_query_key_value(buf, "sms_messages", sms_messages, sizeof(sms_messages));

    char decoded_ssid[33] = {0};
    char decoded_password[65] = {0};
    char decoded_firebase_url[129] = {0};
    char decoded_token[129] = {0};
    char decoded_aws_url[256] = {0};
    char decoded_phone_numbers[256] = {0};
    char decoded_sms_messages[1024] = {0};
    
    urldecode(ssid, decoded_ssid, sizeof(decoded_ssid));
    urldecode(password, decoded_password, sizeof(decoded_password));
    urldecode(firebase_url, decoded_firebase_url, sizeof(decoded_firebase_url));
    urldecode(token, decoded_token, sizeof(decoded_token));
    urldecode(aws_url, decoded_aws_url, sizeof(decoded_aws_url));
    urldecode(phone_numbers, decoded_phone_numbers, sizeof(decoded_phone_numbers));
    urldecode(sms_messages, decoded_sms_messages, sizeof(decoded_sms_messages));

    if (strstr(decoded_firebase_url, "://") == NULL) {
        ESP_LOGE(TAG, "Invalid Firebase URL format: %s", firebase_url);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid URL format");
        return ESP_FAIL;
    }


    ESP_LOGI(TAG, "Saving WiFi SSID: %s", decoded_ssid);
    ESP_LOGI(TAG, "Saving WiFi password: %s", decoded_password);
    ESP_LOGI(TAG, "Saving Firebase URL: %s", decoded_firebase_url);
    ESP_LOGI(TAG, "Saving token: %s", decoded_token);
    ESP_LOGI(TAG, "Saving AWS URL: %s", decoded_aws_url);
    ESP_LOGI(TAG, "Saving phone numbers: %s", decoded_phone_numbers);
    ESP_LOGI(TAG, "Saving SMS messages: %s", decoded_sms_messages);
    ESP_LOGI(TAG, "Saving all configurations");
    
    // Save to NVS
    nvs_handle_t nvs;
    if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_str(nvs, "wifi_ssid", decoded_ssid);
        nvs_set_str(nvs, "wifi_password", decoded_password);
        nvs_set_str(nvs, "firebase_url", decoded_firebase_url);
        nvs_set_str(nvs, "token", decoded_token);
        nvs_set_str(nvs, "aws_url", decoded_aws_url);
        nvs_set_str(nvs, "phone_numbers", decoded_phone_numbers);
        nvs_set_str(nvs, "sms_messages", decoded_sms_messages);
        nvs_commit(nvs);
        nvs_close(nvs);
        httpd_resp_sendstr(req, "All configurations saved successfully");
        
        // Reboot after successful save
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "NVS error");
    }
    return ESP_OK;
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
    char ssid[33] = {0};
    char firebase_url[128] = {0};
    char token[128] = {0};
    size_t len = sizeof(ssid);
    
    nvs_get_str(nvs_handle_storage, "wifi_ssid", ssid, &len);
    len = sizeof(firebase_url);
    nvs_get_str(nvs_handle_storage, "firebase_url", firebase_url, &len);
    len = sizeof(token);
    nvs_get_str(nvs_handle_storage, "token", token, &len);

    char fixed_url[150] = {0};
    if (strstr(firebase_url, "://") == NULL && firebase_url[0] != '\0') {
        snprintf(fixed_url, sizeof(fixed_url), "https://%s", firebase_url);
    } else {
        strncpy(fixed_url, firebase_url, sizeof(fixed_url));
    }
    
    char json[384];
    snprintf(json, sizeof(json), "{\"ssid\":\"%s\",\"firebase_url\":\"%s\",\"token\":\"%s\"}", 
             ssid, fixed_url, token);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
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
