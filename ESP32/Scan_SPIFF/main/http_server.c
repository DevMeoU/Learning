#include "http_server.h"
#include "esp_log.h"
#include "spiffs_handler.h"
#include "cJSON.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "esp_system.h"
#include "esp_err.h"
#include <string.h>
#include "sdkconfig.h"
#include <ctype.h>
#include "wifi_manager.h"
#include "esp_wifi_types.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_client.h"
#include "common_types.h"

extern QueueHandle_t xSensorQueue;
extern nvs_handle_t nvs_handle_storage;

#ifndef CONFIG_LOG_MAXIMUM_LEVEL 
#define CONFIG_LOG_MAXIMUM_LEVEL 3    // Define default log level as INFO if not set
#endif

#ifndef MALLOC_CAP_INTERNAL
#define MALLOC_CAP_INTERNAL (1<<9)    // Define internal memory capability if not set
#endif

static const char *TAG = "HTTP_Server";
static httpd_handle_t server = NULL;
extern nvs_handle_t nvs_handle_storage;

// Function declarations
static void urldecode(char *src, char *dest, size_t max_len);

esp_err_t handle_get_wifi_config_html(httpd_req_t *req) {
    return send_file_from_spiffs(req, "/spiffs/wifi_config.html", "text/html");
}

esp_err_t handle_get_wifi_scan(httpd_req_t *req) {
    ESP_LOGI(TAG, "Handling WiFi scan request");
    
    // Use the WiFi manager's scan function instead of implementing scan here
    esp_err_t ret = start_wifi_scan();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi scan: %s", esp_err_to_name(ret));
        
        // Return appropriate error response based on error type
        httpd_resp_set_type(req, "application/json");
        
        if (ret == ESP_ERR_WIFI_STATE) {
            httpd_resp_set_status(req, "409 Conflict");
            httpd_resp_sendstr(req, "{\"error\":\"Scan already in progress\"}");
        } else {
            httpd_resp_set_status(req, "500 Internal Server Error");
            char error_msg[100];
            snprintf(error_msg, sizeof(error_msg), "{\"error\":\"Failed to start WiFi scan: %s\"}", esp_err_to_name(ret));
            httpd_resp_sendstr(req, error_msg);
        }
        return ESP_OK;
    }
    
    // Wait for scan to complete with timeout
    int timeout_ms = 5000; // 5 seconds timeout
    int wait_interval_ms = 100;
    int elapsed_ms = 0;
    
    ESP_LOGI(TAG, "Waiting for scan to complete (timeout: %d ms)", timeout_ms);
    while (elapsed_ms < timeout_ms) {
        uint16_t ap_count = 0;
        const wifi_ap_record_t* ap_records = get_scanned_aps(&ap_count);
        
        if (ap_records != NULL) {
            // Scan completed successfully
            ESP_LOGI(TAG, "Scan completed, found %d networks", ap_count);
            
            // Create JSON response
            cJSON *root = cJSON_CreateObject();
            cJSON *networks = cJSON_CreateArray();
            cJSON_AddItemToObject(root, "networks", networks);

            // Add scan results to JSON
            for (int i = 0; i < ap_count; i++) {
                if (strlen((char *)ap_records[i].ssid) == 0) continue; // Skip empty SSIDs
                
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
        
        // Wait a bit before checking again
        vTaskDelay(pdMS_TO_TICKS(wait_interval_ms));
        elapsed_ms += wait_interval_ms;
    }
    
    // Timeout occurred
    ESP_LOGW(TAG, "Scan timeout after %d ms", timeout_ms);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, "504 Gateway Timeout");
    httpd_resp_sendstr(req, "{\"error\":\"Scan timeout\"}");
    return ESP_OK;
}

esp_err_t handle_post_wifi_config(httpd_req_t *req) {
    char *buf = NULL;
    char *decoded_buf = NULL;
    esp_err_t ret = ESP_OK;
    bool response_sent = false;

    // Validate content length
    if (req->content_len <= 0 || req->content_len > 1024) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "{\"error\":\"Invalid content length\"}");
        return ESP_FAIL;
    }

    // Allocate buffers
    buf = calloc(req->content_len + 1, sizeof(char));
    decoded_buf = calloc(req->content_len + 1, sizeof(char));
    if (!buf || !decoded_buf) {
        ESP_LOGE(TAG, "Failed to allocate memory for request buffers");
        if (!response_sent) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "{\"error\":\"Server memory error\"}");
        }
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    // Read request body
    int total_len = req->content_len;
    int cur_len = 0;
    while (cur_len < total_len) {
        int received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT && !response_sent) {
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "{\"error\":\"Request timeout\"}");
                response_sent = true;
            }
            ret = ESP_FAIL;
            goto cleanup;
        }
        cur_len += received;
    }
    buf[cur_len] = '\0';

    // URL decode the data
    urldecode(buf, decoded_buf, req->content_len + 1);

    // Initialize config variables with proper sizes
    char ssid[33] = {0};        // 32 chars + null
    char password[65] = {0};     // 64 chars + null
    char server_url[129] = {0};  // 128 chars + null
    char token[129] = {0};       // 128 chars + null
    bool has_ssid = false;    // Parse form data and add debug logging
    char *saveptr;
    char *ptr = strtok_r(decoded_buf, "&", &saveptr);
    ESP_LOGI(TAG, "Parsing form data: %s", decoded_buf);
    
    while (ptr != NULL) {
        ESP_LOGI(TAG, "Processing field: %s", ptr);
        if (strncmp(ptr, "ssid=", 5) == 0 && strlen(ptr) > 5) {
            strlcpy(ssid, ptr + 5, sizeof(ssid));
            has_ssid = true;
            ESP_LOGI(TAG, "Found SSID: %s", ssid);
        } else if (strncmp(ptr, "password=", 9) == 0 && strlen(ptr) > 9) {
            strlcpy(password, ptr + 9, sizeof(password));
            ESP_LOGI(TAG, "Found password: ***");
        } else if (strncmp(ptr, "server_url=", 11) == 0 && strlen(ptr) > 11) {
            strlcpy(server_url, ptr + 11, sizeof(server_url));
            ESP_LOGI(TAG, "Found server URL: %s", server_url);
        } else if (strncmp(ptr, "token=", 6) == 0 && strlen(ptr) > 6) {
            strlcpy(token, ptr + 6, sizeof(token));
            ESP_LOGI(TAG, "Found token: ***");
        }
        ptr = strtok_r(NULL, "&", &saveptr);
    }    // Log current state before validation
    ESP_LOGI(TAG, "Validating form data - has_ssid: %d, ssid length: %d", has_ssid, strlen(ssid));
    
    // Validate required fields
    if (!has_ssid) {
        ESP_LOGW(TAG, "SSID field not found in form data");
        if (!response_sent) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "{\"error\":\"SSID field is missing\"}");
            response_sent = true;
        }
        ret = ESP_FAIL;
        goto cleanup;
    }

    if (strlen(ssid) == 0) {
        ESP_LOGW(TAG, "SSID is empty");
        if (!response_sent) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "{\"error\":\"SSID value is empty\"}");
            response_sent = true;
        }
        ret = ESP_FAIL;
        goto cleanup;
    }

    // Check if SSID contains only whitespace
    bool is_ssid_empty = true;
    for (size_t i = 0; ssid[i]; i++) {
        if (!isspace((unsigned char)ssid[i])) {
            is_ssid_empty = false;
            break;
        }
    }

    if (is_ssid_empty) {
        ESP_LOGW(TAG, "SSID contains only whitespace");
        if (!response_sent) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "{\"error\":\"SSID cannot be only whitespace\"}");
            response_sent = true;
        }
        ret = ESP_FAIL;
        goto cleanup;
    }

    // Save to NVS with error checking for each operation
    if ((ret = nvs_set_str(nvs_handle_storage, "wifi_ssid", ssid)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save SSID to NVS: %s", esp_err_to_name(ret));
        if (!response_sent) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "{\"error\":\"Failed to save SSID\"}");
            response_sent = true;
        }
        goto cleanup;
    }

    // Save password (optional)
    if ((ret = nvs_set_str(nvs_handle_storage, "wifi_password", password)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save password to NVS: %s", esp_err_to_name(ret));
        if (!response_sent) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "{\"error\":\"Failed to save password\"}");
            response_sent = true;
        }
        goto cleanup;
    }

    // Save server URL
    if ((ret = nvs_set_str(nvs_handle_storage, "server_url", server_url)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save server URL to NVS: %s", esp_err_to_name(ret));
        if (!response_sent) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "{\"error\":\"Failed to save server URL\"}");
            response_sent = true;
        }
        goto cleanup;
    }

    // Save token
    if ((ret = nvs_set_str(nvs_handle_storage, "token", token)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save token to NVS: %s", esp_err_to_name(ret));
        if (!response_sent) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "{\"error\":\"Failed to save token\"}");
            response_sent = true;
        }
        goto cleanup;
    }

    // Commit NVS changes
    if ((ret = nvs_commit(nvs_handle_storage)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(ret));
        if (!response_sent) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "{\"error\":\"Failed to save configuration\"}");
            response_sent = true;
        }
        goto cleanup;
    }

    // Only send success response if we haven't sent any error response
    if (!response_sent) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"status\":\"success\",\"message\":\"Configuration saved successfully\"}");
        // Trigger WiFi connect and reboot if successful
        extern void wifi_manager_connect_from_nvs(void);
        wifi_manager_connect_from_nvs();
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait a bit for connection
        esp_restart();
    }

cleanup:
    if (buf) free(buf);
    if (decoded_buf) free(decoded_buf);
    return (ret == ESP_OK) ? ESP_OK : ESP_FAIL;
}

// Handler for GET /config: returns current WiFi/server config as JSON
esp_err_t handle_get_config(httpd_req_t *req) {
    char ssid[33] = {0};
    char server_url[128] = {0};
    char token[128] = {0};
    size_t len = 0;
    esp_err_t ret;

    // Read SSID
    len = sizeof(ssid);
    ret = nvs_get_str(nvs_handle_storage, "wifi_ssid", ssid, &len);
    if (ret != ESP_OK) ssid[0] = '\0';
    // Read server_url
    len = sizeof(server_url);
    ret = nvs_get_str(nvs_handle_storage, "server_url", server_url, &len);
    if (ret != ESP_OK) server_url[0] = '\0';
    // Read token
    len = sizeof(token);
    ret = nvs_get_str(nvs_handle_storage, "token", token, &len);
    if (ret != ESP_OK) token[0] = '\0';

    char json[384];
    snprintf(json, sizeof(json), "{\"ssid\":\"%s\",\"server_url\":\"%s\",\"token\":\"%s\"}", ssid, server_url, token);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

// Handler for POST /config: accept JSON, save config, connect WiFi
esp_err_t handle_post_json_config(httpd_req_t *req) {
    char buf[512] = {0};
    int ret, total_len = req->content_len, cur_len = 0;
    if (total_len <= 0 || total_len > sizeof(buf) - 1) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"Invalid content length\"}");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        int received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
        if (received <= 0) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"Failed to receive data\"}");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[cur_len] = '\0';
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return ESP_FAIL;
    }
    const cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    const cJSON *password = cJSON_GetObjectItem(root, "password");
    const cJSON *server_url = cJSON_GetObjectItem(root, "server_url");
    const cJSON *token = cJSON_GetObjectItem(root, "token");
    if (!ssid || !server_url || !token) {
        cJSON_Delete(root);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"Missing required fields\"}");
        return ESP_FAIL;
    }
    // Save to NVS
    ret = nvs_set_str(nvs_handle_storage, "wifi_ssid", ssid->valuestring);
    if (ret != ESP_OK) goto nvs_error;
    ret = nvs_set_str(nvs_handle_storage, "wifi_password", password ? password->valuestring : "");
    if (ret != ESP_OK) goto nvs_error;
    ret = nvs_set_str(nvs_handle_storage, "server_url", server_url->valuestring);
    if (ret != ESP_OK) goto nvs_error;
    ret = nvs_set_str(nvs_handle_storage, "token", token->valuestring);
    if (ret != ESP_OK) goto nvs_error;
    ret = nvs_commit(nvs_handle_storage);
    if (ret != ESP_OK) goto nvs_error;
    // Trigger WiFi connect (call your connect logic, e.g. wifi_manager_connect_from_nvs())
    extern void wifi_manager_connect_from_nvs(void);
    wifi_manager_connect_from_nvs();
    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"success\":true,\"message\":\"Configuration saved\"}");
    return ESP_OK;
nvs_error:
    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"Failed to save config\"}");
    return ESP_FAIL;
}

void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_get = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = handle_get_wifi_config_html,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_get);

        httpd_uri_t uri_scan = {
            .uri = "/scan",
            .method = HTTP_GET,
            .handler = handle_get_wifi_scan,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_scan);

        httpd_uri_t uri_config = {
            .uri = "/configure",
            .method = HTTP_POST,
            .handler = handle_post_wifi_config,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_config);

        // Register GET /config endpoint
        httpd_uri_t uri_get_config = {
            .uri = "/config",
            .method = HTTP_GET,
            .handler = handle_get_config,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_get_config);

        // Register POST /config for JSON config
        httpd_uri_t uri_post_config = {
            .uri = "/config",
            .method = HTTP_POST,
            .handler = handle_post_json_config,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_post_config);

        ESP_LOGI(TAG, "Web server started");
    }
}

void stop_webserver(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
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

void http_task(void *pvParameters) {
    const char *TAG = "HTTP_Task";
    static bool rebooted_after_firebase = false;
    while (1) {
        // Wait for data to be available in the sensor queue
        data_frame_t data;
        if (xQueueReceive(xSensorQueue, &data, portMAX_DELAY) == pdTRUE) {
            // Create the JSON object
            cJSON *root = cJSON_CreateObject();
            if (root == NULL) {
                ESP_LOGE(TAG, "Failed to create JSON object");
                continue;
            }

            // Add sensor data and LED states
            cJSON_AddNumberToObject(root, "sensor_value", data.sensor_data.f_sensor_value);
            cJSON_AddNumberToObject(root, "timestamp", data.timestamp);
            cJSON_AddBoolToObject(root, "led_green", data.led_green ? true : false);
            cJSON_AddBoolToObject(root, "led_red", data.led_red ? true : false);
            cJSON_AddBoolToObject(root, "led_yellow", data.led_yellow ? true : false);
            
            char *post_data = cJSON_Print(root);
            
            // Get server URL and token from NVS
            size_t required_size;
            char *server_url = NULL;
            char *token = NULL;
            
            if (nvs_get_str(nvs_handle_storage, "server_url", NULL, &required_size) == ESP_OK) {
                server_url = malloc(required_size);
                if (server_url) {
                    nvs_get_str(nvs_handle_storage, "server_url", server_url, &required_size);
                }
            }
            
            if (nvs_get_str(nvs_handle_storage, "token", NULL, &required_size) == ESP_OK) {
                token = malloc(required_size);
                if (token) {
                    nvs_get_str(nvs_handle_storage, "token", token, &required_size);
                }
            }
            
            if (server_url && token && post_data) {
                esp_http_client_config_t config = {
                    .url = server_url,
                    .method = HTTP_METHOD_POST,
                    .cert_pem = NULL,  // Use NULL for non-HTTPS
                    .timeout_ms = 5000,
                };
                
                esp_http_client_handle_t client = esp_http_client_init(&config);
                if (client) {
                    // Set headers
                    esp_http_client_set_header(client, "Content-Type", "application/json");
                    esp_http_client_set_header(client, "Authorization", token);
                    esp_http_client_set_post_field(client, post_data, strlen(post_data));
                    
                    esp_err_t err = esp_http_client_perform(client);
                    if (err == ESP_OK) {
                        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                                esp_http_client_get_status_code(client),
                                esp_http_client_get_content_length(client));
                        // Reboot after first successful Firebase POST
                        if (!rebooted_after_firebase) {
                            rebooted_after_firebase = true;
                            ESP_LOGI(TAG, "First Firebase POST success, rebooting...");
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            esp_restart();
                        }
                    } else {
                        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
                    }
                    
                    esp_http_client_cleanup(client);
                }
            } else {
                ESP_LOGE(TAG, "Failed to get server URL or token from NVS");
            }
            
            // Free allocated memory
            if (server_url) free(server_url);
            if (token) free(token);
            if (post_data) free(post_data);
            cJSON_Delete(root);
        }
        
        // Small delay to prevent tight loop
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
