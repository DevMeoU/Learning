#include "spiffs_handler.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

static const char *TAG = "SPIFFS";
static bool s_spiffs_mounted = false;

esp_err_t initialize_spiffs(void) {
    if (s_spiffs_mounted) {
        ESP_LOGW(TAG, "SPIFFS already mounted");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
            ESP_LOGI(TAG, "Attempting to format SPIFFS...");
            
            // Try to format manually if automatic format failed
            ret = esp_spiffs_format(NULL);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Manual format failed (%s)", esp_err_to_name(ret));
                return ret;
            }
            
            // Try mounting again after format
            ret = esp_vfs_spiffs_register(&conf);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Mount after format failed (%s)", esp_err_to_name(ret));
                return ret;
            }
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
            return ret;
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
            return ret;
        }
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information");
        esp_vfs_spiffs_unregister(NULL);
        return ret;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    s_spiffs_mounted = true;
    return ESP_OK;
}

esp_err_t send_file_from_spiffs(void* req, const char *path, const char *content_type) {
    httpd_req_t *request = (httpd_req_t *)req;
    esp_err_t ret = ESP_OK;

    if (!s_spiffs_mounted) {
        ESP_LOGE(TAG, "SPIFFS not mounted");
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Filesystem not mounted");
        return ESP_ERR_INVALID_STATE;
    }    // Ensure path starts with /spiffs/
    char full_path[CONFIG_SPIFFS_OBJ_NAME_LEN * 2];
    if (strncmp(path, "/spiffs/", 8) != 0) {
        snprintf(full_path, sizeof(full_path), "/spiffs/%s", path[0] == '/' ? path + 1 : path);
    } else {
        strlcpy(full_path, path, sizeof(full_path));
    }

    // First check if file exists
    struct stat st;
    if (stat(full_path, &st) != 0) {
        ESP_LOGE(TAG, "File does not exist: %s", full_path);
        httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_ERR_NOT_FOUND;
    }

    FILE* f = fopen(full_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s (%s)", full_path, strerror(errno));
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file");
        return ESP_FAIL;
    }

    // Set content type with a default fallback
    const char* final_type = content_type;
    if (!final_type) {
        // Simple extension based content type detection
        const char* ext = strrchr(path, '.');
        if (ext) {
            if (strcasecmp(ext, ".html") == 0) final_type = "text/html";
            else if (strcasecmp(ext, ".css") == 0) final_type = "text/css";
            else if (strcasecmp(ext, ".js") == 0) final_type = "application/javascript";
            else if (strcasecmp(ext, ".json") == 0) final_type = "application/json";
            else if (strcasecmp(ext, ".png") == 0) final_type = "image/png";
            else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) final_type = "image/jpeg";
            else final_type = "application/octet-stream";
        } else {
            final_type = "application/octet-stream";
        }
    }
    httpd_resp_set_type(request, final_type);

    // Check file size and set Content-Length
    char size_str[32];
    snprintf(size_str, sizeof(size_str), "%ld", st.st_size);
    httpd_resp_set_hdr(request, "Content-Length", size_str);

    char *buf = NULL;
    size_t buf_size = 2048;
    size_t read_size;
    
    buf = malloc(buf_size);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate read buffer");
        fclose(f);
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_ERR_NO_MEM;
    }

    do {
        read_size = fread(buf, 1, buf_size, f);
        if (read_size > 0) {
            if (httpd_resp_send_chunk(request, buf, read_size) != ESP_OK) {
                ESP_LOGE(TAG, "File sending failed!");
                ret = ESP_FAIL;
                break;
            }
        }
    } while (read_size == buf_size);

    free(buf);
    fclose(f);
    
    if (ret == ESP_OK) {
        httpd_resp_send_chunk(request, NULL, 0);
        ESP_LOGI(TAG, "File sent successfully: %s", full_path);
    }
    
    return ret;
}

esp_err_t get_spiffs_info(size_t* total_bytes, size_t* used_bytes) {
    if (!s_spiffs_mounted) {
        ESP_LOGE(TAG, "SPIFFS not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_spiffs_info(NULL, total_bytes, used_bytes);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS info (%s)", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

bool spiffs_file_exists(const char *path) {
    if (!s_spiffs_mounted || !path) {
        return false;
    }    char full_path[CONFIG_SPIFFS_OBJ_NAME_LEN * 2];
    snprintf(full_path, sizeof(full_path), "/spiffs/%s", path[0] == '/' ? path + 1 : path);

    struct stat st;
    return (stat(full_path, &st) == 0);
}
