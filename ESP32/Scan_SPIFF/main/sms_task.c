#include <esp_log.h>
#include <string.h>
#include <stdlib.h>
#include <esp_err.h>
#include <ctype.h>                // for isalnum()
#include <esp_http_client.h>      // esp_http_client_config_t, esp_http_client_*
#include <esp_tls.h>              // esp_crt_bundle_attach()
#include <mbedtls/base64.h>       // mbedtls_base64_encode()
#include <cJSON.h>                // cJSON_CreateObject(), …
#include <nvs_flash.h>            // nvs_flash_init()
#include <nvs.h>                  // nvs_open(), nvs_get_str()
#include <esp_crt_bundle.h>   // for esp_crt_bundle_attach()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "include/gpio_control.h"
#include "sms_task.h"

#define DEBOUNCE_MS        200
#define POLL_INTERVAL_MS   50

extern nvs_handle_t nvs_handle_storage;
extern SemaphoreHandle_t network_mutex;
static const char *TAG = "SMS";

static esp_err_t send_sms_via_twilio(const char* account_sid, const char* auth_token, 
                                     const char* from_number, const char* to_number, 
                                     const char* url, const char* message);

static char* base64_encode(const char* username, const char* password);
static esp_err_t send_sms_via_aws(const char* aws_url, const char* phone, const char* message);
static char* http_escape(const char* str);
static void send_sms_to_all_recipients();

// Enhanced Twilio SMS function with URL encoding and memory safety
static esp_err_t send_sms_via_twilio(const char* account_sid, const char* auth_token, 
                                     const char* from_number, const char* to_number, 
                                     const char* url, const char* message) {
    // Validate all parameters
    if (!account_sid || !*account_sid || !auth_token || !*auth_token ||
        !from_number || !*from_number || !to_number || !*to_number || 
        !url || !*url || !message || !*message) {
        ESP_LOGE(TAG, "Invalid Twilio parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // URL‐encode all components
    char *to_encoded   = http_escape(to_number);
    char *from_encoded = http_escape(from_number);
    char *body_encoded = http_escape(message);
    if (!to_encoded || !from_encoded || !body_encoded) {
        free(to_encoded);
        free(from_encoded);
        free(body_encoded);
        return ESP_ERR_NO_MEM;
    }

    // Build URL and payload
    char *url_full       = NULL;
    char *post_data = NULL;

    if (asprintf(&url_full, url, account_sid) == -1) {
        ESP_LOGE(TAG, "asprintf for url failed");
        return ESP_ERR_INVALID_ARG;
    }

    if (asprintf(&post_data, "To=%s&From=%s&Body=%s", to_encoded, from_encoded, body_encoded) == -1) {
        ESP_LOGE(TAG, "asprintf for post_data failed");
        free(url_full);  // tránh rò rỉ bộ nhớ nếu url đã cấp phát
        return ESP_ERR_INVALID_ARG;
    }

    // Prepare HTTP client
    if (!url_full || !post_data) {
        free(to_encoded);
        free(from_encoded);
        free(body_encoded);
        free(url_full);
        free(post_data);
        return ESP_ERR_NO_MEM;
    }

    // Predeclare & initialize err so that even a 'goto cleanup' returns something defined
    esp_err_t err = ESP_FAIL;

    if (xSemaphoreTake(network_mutex, pdMS_TO_TICKS(10000)) == pdTRUE) 
    {
        // Configure HTTP client
        esp_http_client_config_t config = {
            .url               = url_full,
            .method            = HTTP_METHOD_POST,
            .skip_cert_common_name_check = true,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .timeout_ms        = 15000,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (!client) {
            ESP_LOGE(TAG, "HTTP client init failed");
            goto cleanup;        // err is already ESP_FAIL
        }

        // Authentication header
        char *auth_header = NULL;

        asprintf(&auth_header, "Basic %s", base64_encode(account_sid, auth_token));
        if (!auth_header) {
            err = ESP_ERR_NO_MEM;
            esp_http_client_cleanup(client);
            goto cleanup;
        }

        esp_http_client_set_header(client, "Authorization", auth_header);
        esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
        esp_http_client_set_post_field(client, post_data, strlen(post_data));

        // Perform request
        err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "URL: %s", url_full);
            ESP_LOGI(TAG, "Message sent: %s", post_data);
            ESP_LOGI(TAG, "To: %s", to_number);
            ESP_LOGI(TAG, "From: %s", from_number);
            ESP_LOGI(TAG, "Body: %s", message);
            ESP_LOGI(TAG, "SID: %s", account_sid);
            ESP_LOGI(TAG, "Token: %s", auth_token);

            int status = esp_http_client_get_status_code(client);
            if (status == 200 || status == 201) {
                ESP_LOGI(TAG, "Twilio SMS sent successfully");
            } else {
                ESP_LOGE(TAG, "Twilio error, status: %d", status);
                char *response = malloc(256);
                if (response) {
                    int len = esp_http_client_read(client, response, 255);
                    response[len > 0 ? len : 0] = '\0';
                    ESP_LOGE(TAG, "Twilio response: %s", response);
                    free(response);
                }
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "Twilio request failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);

        xSemaphoreGive(network_mutex);
    cleanup:
        free(to_encoded);
        free(from_encoded);
        free(body_encoded);
        free(url_full);
        free(post_data);
        free(auth_header);
    }
    return err;
}

// Robust base64 encoder with proper memory handling
static char* base64_encode(const char* username, const char* password) {
    if (!username || !password) {
        ESP_LOGE(TAG, "base64_encode: NULL input");
        return NULL;
    }

    char *creds = NULL;
    if (asprintf(&creds, "%s:%s", username, password) < 0 || creds == NULL) {
        ESP_LOGE(TAG, "asprintf failed");
        return NULL;
    }

    size_t len = strlen(creds);
    size_t output_len = 0;

    // Tính toán độ dài buffer cần thiết
    int ret = mbedtls_base64_encode(NULL, 0, &output_len, (const uint8_t *)creds, len);
    if (ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
        ESP_LOGE(TAG, "Unexpected error calculating base64 length: %d", ret);
        free(creds);
        return NULL;
    }

    // Cấp phát bộ nhớ cho kết quả
    unsigned char *encoded = malloc(output_len + 1);  // +1 để kết thúc chuỗi
    if (!encoded) {
        ESP_LOGE(TAG, "malloc failed for size %zu", output_len + 1);
        free(creds);
        return NULL;
    }

    size_t actual_len = 0;
    ret = mbedtls_base64_encode(encoded, output_len, &actual_len, (const uint8_t *)creds, len);
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_base64_encode failed: %d", ret);
        free(encoded);
        free(creds);
        return NULL;
    }

    encoded[actual_len] = '\0';  // Đảm bảo chuỗi null-terminated
    free(creds);
    return (char *)encoded;
}

// Improved AWS SMS function with JSON escaping
static esp_err_t send_sms_via_aws(const char* aws_url, const char* phone, const char* message) {
    // Validate parameters
    if (!aws_url || !*aws_url || !phone || !*phone || !message || !*message) {
        ESP_LOGE(TAG, "Invalid AWS parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // Create JSON payload with escaping
    cJSON *root = cJSON_CreateObject();
    if (!root) return ESP_ERR_NO_MEM;
    
    cJSON_AddStringToObject(root, "phone", phone);
    cJSON_AddStringToObject(root, "message", message);
    
    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    if (!post_data) return ESP_ERR_NO_MEM;

    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = aws_url,
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        free(post_data);
        return ESP_FAIL;
    }
    
    // Set headers and payload
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    // Execute request
    esp_err_t err = esp_http_client_perform(client);
    free(post_data);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status >= 200 && status < 300) {
            ESP_LOGI(TAG, "AWS SMS sent, status: %d", status);
        } else {
            ESP_LOGE(TAG, "AWS SMS error, status: %d", status);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "AWS SMS failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return err;
}

// URL encoder helper for form data
static char* http_escape(const char* str) {
    if (!str) return NULL;
    
    const char *p;
    char *buf = malloc(strlen(str) * 3 + 1); // Max 3x expansion
    if (!buf) return NULL;
    
    char *ptr = buf;
    for (p = str; *p; p++) {
        if (isalnum((unsigned char)*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~') {
            *ptr++ = *p;
        } else if (*p == ' ') {
            *ptr++ = '+';  // Space becomes '+' in form data
        } else {
            ptr += sprintf(ptr, "%%%02X", (unsigned char)*p);
        }
    }
    *ptr = '\0';
    return buf;
}

// Enhanced SMS sending with proper encoding and error handling
void send_sms_to_all_recipients(void)
{
    char phone_numbers[256] = {0};
    char sms_messages[1024] = {0};
    char twilio_acc_sid[64] = {0};
    char twilio_token[64]   = {0};
    char twilio_from_n[32]  = {0};
    char twilio_url[256]    = {0};
    char aws_url[256]       = {0};
    char enable_twilio[8]   = "off";
    char enable_aws[8]      = "off";

    #define SAFE_NVS_GET(key, buf) \
        do { \
            size_t len = sizeof(buf); \
            if (nvs_get_str(nvs_handle_storage, key, buf, &len) != ESP_OK) { \
                buf[0] = '\0'; \
            } \
        } while (0)

    SAFE_NVS_GET("phone_numbers",   phone_numbers);
    SAFE_NVS_GET("sms_messages",    sms_messages);
    SAFE_NVS_GET("twilio_acc_sid",  twilio_acc_sid);
    SAFE_NVS_GET("twilio_token",    twilio_token);
    SAFE_NVS_GET("twilio_from_n",   twilio_from_n);
    SAFE_NVS_GET("twilio_url",      twilio_url);
    SAFE_NVS_GET("aws_url",         aws_url);
    SAFE_NVS_GET("enable_twilio",   enable_twilio);
    SAFE_NVS_GET("enable_aws",      enable_aws);
    #undef SAFE_NVS_GET

    char *phones_copy   = strdup(phone_numbers);
    char *messages_copy = strdup(sms_messages);
    if (!phones_copy || !messages_copy) {
        free(phones_copy);
        free(messages_copy);
        return;
    }

    char *phone   = strtok(phones_copy, ",");
    char *message = strtok(messages_copy, "|||");
    int count = 0;

    while (phone && message) {
        // trim
        while (*phone   == ' ') phone++;
        while (*message == ' ') message++;

        if (!*phone || !*message) {
            ESP_LOGW(TAG, "Skipping empty entry");
            goto next_pair;
        }

        ESP_LOGI(TAG, "Sending SMS to %s: %s", phone, message);

        if (strcmp(enable_twilio, "on") == 0) {
            if (send_sms_via_twilio(twilio_acc_sid, twilio_token,
                                    twilio_from_n, phone, 
                                    twilio_url, message) == ESP_OK) {
                count++;
            } else {
                ESP_LOGE(TAG, "Twilio send failed");
            }
        }
        if (strcmp(enable_aws, "on") == 0) {
            if (send_sms_via_aws(aws_url, phone, message) == ESP_OK) {
                count++;
            } else {
                ESP_LOGE(TAG, "AWS send failed");
            }
        }

    next_pair:
        phone   = strtok(NULL, ",");
        message = strtok(NULL, "|||");
    }

    ESP_LOGI(TAG, "Sent %d SMS messages", count);
    free(phones_copy);
    free(messages_copy);
}

void sms_task_wrapper(void *pvParameter)
{
    // Initialize NVS if not already
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle_storage));

    int last_level = 1;
    for (;;) {
        int lvl = gpio_get_level(BUTTON_SEND_SMS);
        if (lvl == 0 && last_level == 1) {
            ESP_LOGI(TAG, "Button pressed – sending SMS batch");
            send_sms_to_all_recipients();
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        }
        last_level = lvl;
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}