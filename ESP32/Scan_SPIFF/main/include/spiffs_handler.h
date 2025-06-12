#ifndef SPIFFS_HANDLER_H
#define SPIFFS_HANDLER_H

#include "esp_spiffs.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>

/**
 * @brief Initialize and mount SPIFFS filesystem
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t initialize_spiffs(void);

/**
 * @brief Deinitialize and unmount SPIFFS filesystem
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t deinit_spiffs(void);

/**
 * @brief Read data from a file in SPIFFS
 * 
 * @param path Path to file in SPIFFS
 * @param buffer Buffer to store read data
 * @param max_size Maximum number of bytes to read
 * @return esp_err_t ESP_OK on success
 */
esp_err_t read_from_spiffs(const char* path, char* buffer, size_t max_size);

/**
 * @brief Write data to a file in SPIFFS
 * 
 * @param path Path to file in SPIFFS
 * @param data Data to write to file
 * @return esp_err_t ESP_OK on success
 */
esp_err_t write_to_spiffs(const char* path, const char* data);

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
