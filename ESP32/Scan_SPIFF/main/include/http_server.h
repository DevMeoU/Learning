#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <esp_http_client.h>
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_tls.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the HTTP server
 * @return ESP_OK on success
 */
esp_err_t start_webserver(void);

/**
 * @brief HTTP server task function
 * @param pvParameters Task parameters (unused)
 */
void http_task(void *pvParameters);

/**
 * @brief Send data to Firebase
 * @param server_url Firebase URL
 * @param post_data JSON data to send
 * @return ESP_OK on success
 */
esp_err_t send_to_firebase(const char* server_url, const char* post_data, esp_http_client_method_t method);

/**
 * @brief Get server certificate
 * @param url Server URL
 * @return Certificate in PEM format or NULL on failure
 */
char* get_server_certificate(const char* url);

/**
 * @brief Save certificate to NVS
 * @param cert_pem Certificate in PEM format
 * @return ESP_OK on success
 */
esp_err_t save_cert_to_nvs(const char* cert_pem);

/**
 * @brief Load certificate from NVS
 * @return Certificate in PEM format or NULL if not found
 */
char* load_cert_from_nvs(void);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_H */
