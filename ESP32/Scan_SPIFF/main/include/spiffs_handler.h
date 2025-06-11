#ifndef SPIFFS_HANDLER_H
#define SPIFFS_HANDLER_H

#include "esp_spiffs.h"
#include "esp_err.h"

/**
 * @brief Initialize and mount SPIFFS filesystem
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t initialize_spiffs(void);

/**
 * @brief Send a file from SPIFFS via HTTP
 * 
 * @param req HTTP request handle
 * @param path Path to file in SPIFFS
 * @param content_type MIME type of the file
 * @return esp_err_t ESP_OK on success
 */
esp_err_t send_file_from_spiffs(void* req, const char *path, const char *content_type);

/**
 * @brief Get SPIFFS partition info
 * 
 * @param total_bytes Total size in bytes (can be NULL)
 * @param used_bytes Used size in bytes (can be NULL) 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t get_spiffs_info(size_t* total_bytes, size_t* used_bytes);

/**
 * @brief Check if a file exists in SPIFFS
 * 
 * @param path Path to file
 * @return true if file exists, false otherwise
 */
bool spiffs_file_exists(const char *path);

#endif /* SPIFFS_HANDLER_H */
