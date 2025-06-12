#include "include/http_server.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_crt_bundle.h"
#include "mbedtls/pem.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "HTTP_SERVER";

// Global certificate storage
extern const uint8_t firebase_cert_pem_start[] asm("_binary_firebase_cert_pem_start");
extern const uint8_t firebase_cert_pem_end[] asm("_binary_firebase_cert_pem_end");

static httpd_handle_t server = NULL;

esp_err_t send_to_firebase(const char* server_url, const char* post_data, esp_http_client_method_t method)
{
    if (!server_url || !post_data) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // Create full URL with .json extension if needed
    char *firebase_url = malloc(strlen(server_url) + 10);
    if (!firebase_url) {
        ESP_LOGE(TAG, "Failed to allocate memory for Firebase URL");
        return ESP_ERR_NO_MEM;
    }
    
    strcpy(firebase_url, server_url);
    if (strstr(firebase_url, ".json") == NULL) {
        // Ensure we have a trailing slash before adding .json
        if (firebase_url[strlen(firebase_url)-1] != '/') {
            strcat(firebase_url, "/");
        }
        strcat(firebase_url, ".json");
    }

    ESP_LOGI(TAG, "Sending to Firebase URL: %s", firebase_url);
    ESP_LOGD(TAG, "POST data: %s", post_data);

    // Configure HTTP client with proper security
    esp_http_client_config_t config = {
        .url = firebase_url,
        .method = method,
        .crt_bundle_attach = esp_crt_bundle_attach, // Use global CA bundle
        .timeout_ms = 15000,
        .keep_alive_enable = false,
        .disable_auto_redirect = false, // Allow redirects
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        free(firebase_url);
        return ESP_FAIL;
    }

    // Set headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Connection", "close");
    
    // Set POST data
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // Execute request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code >= 200 && status_code < 300) {
            ESP_LOGI(TAG, "Data sent successfully to Firebase (Status: %d)", status_code);
        } else {
            ESP_LOGE(TAG, "Firebase responded with error: %d", status_code);
            
            // Read error response
            char error_buf[256] = {0};
            int read_len = esp_http_client_read(client, error_buf, sizeof(error_buf) - 1);
            if (read_len > 0) {
                ESP_LOGE(TAG, "Error response: %.*s", read_len, error_buf);
            }
            
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        // Get detailed TLS error
        int esp_tls_code = 0, esp_tls_flags = 0;
        esp_tls_get_and_clear_last_error(NULL, &esp_tls_code, &esp_tls_flags);
        ESP_LOGE(TAG, "TLS error: code=%d, flags=0x%x", esp_tls_code, esp_tls_flags);
    }

    esp_http_client_cleanup(client);
    free(firebase_url);
    return err;
}

char* get_server_certificate(const char* url)
{
    if (!url) {
        ESP_LOGE(TAG, "Invalid URL");
        return NULL;
    }

    // Extract hostname from URL
    const char *host_start = strstr(url, "://");
    if (!host_start) host_start = url;
    else host_start += 3;
    
    const char *host_end = strchr(host_start, '/');
    if (!host_end) host_end = host_start + strlen(host_start);
    
    char hostname[128] = {0};
    strncpy(hostname, host_start, host_end - host_start);
    hostname[host_end - host_start] = '\0';

    ESP_LOGI(TAG, "Getting certificate for: %s", hostname);

    // Configure TLS with timeout
    esp_tls_cfg_t cfg = {
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_tls_t *tls = esp_tls_init();
    if (!tls) {
        ESP_LOGE(TAG, "Failed to initialize TLS");
        return NULL;
    }

    // Connect to server
    if (esp_tls_conn_new_sync(hostname, strlen(hostname), 443, &cfg, tls) != 1) {
        ESP_LOGE(TAG, "Failed to connect to server");
        esp_tls_conn_destroy(tls);
        return NULL;
    }

    // Get peer certificate
    mbedtls_ssl_context *ssl = esp_tls_get_ssl_context(tls);
    const mbedtls_x509_crt *cert = mbedtls_ssl_get_peer_cert(ssl);
    if (!cert) {
        ESP_LOGE(TAG, "No certificate received");
        esp_tls_conn_destroy(tls);
        return NULL;
    }

    // Convert to PEM format
    size_t pem_size = 2048;
    char *pem_cert = malloc(pem_size);
    if (!pem_cert) {
        ESP_LOGE(TAG, "Memory allocation failed");
        esp_tls_conn_destroy(tls);
        return NULL;
    }

    int ret = mbedtls_pem_write_buffer(
        "-----BEGIN CERTIFICATE-----\n",
        "-----END CERTIFICATE-----\n",
        cert->raw.p,
        cert->raw.len,
        (unsigned char *)pem_cert,
        pem_size,
        &pem_size
    );

    esp_tls_conn_destroy(tls);

    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to convert certificate to PEM (error %d)", ret);
        free(pem_cert);
        return NULL;
    }

    return pem_cert;
}

esp_err_t save_cert_to_nvs(const char* cert_pem)
{
    if (!cert_pem) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, "firebase_cert", cert_pem);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving certificate to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

char* load_cert_from_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return NULL;
    }

    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "firebase_cert", NULL, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting certificate size from NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return NULL;
    }

    char* cert_pem = malloc(required_size);
    if (!cert_pem) {
        ESP_LOGE(TAG, "Failed to allocate memory for certificate");
        nvs_close(nvs_handle);
        return NULL;
    }

    err = nvs_get_str(nvs_handle, "firebase_cert", cert_pem, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading certificate from NVS: %s", esp_err_to_name(err));
        free(cert_pem);
        nvs_close(nvs_handle);
        return NULL;
    }

    nvs_close(nvs_handle);
    return cert_pem;
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "<html><body><h1>ESP32 Web Server</h1></body></html>", -1);
    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

esp_err_t start_webserver(void)
{
    if (server != NULL) {
        ESP_LOGI(TAG, "Web server already started");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;

    ESP_LOGI(TAG, "Starting web server on port: '%d'", config.server_port);
    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting server!");
        return ret;
    }

    httpd_register_uri_handler(server, &root);
    ESP_LOGI(TAG, "Web server started successfully");
    return ESP_OK;
}

void http_task(void *pvParameters)
{
    esp_err_t ret = start_webserver();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Task heartbeat
    }
}
