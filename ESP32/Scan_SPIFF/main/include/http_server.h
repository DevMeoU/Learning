#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_http_server.h"
#include "esp_err.h"
#include "common_types.h"
#include "nvs.h"

void start_webserver(void);
void stop_webserver(void);
void http_task(void *pvParameters);
esp_err_t handle_get_wifi_config_html(httpd_req_t *req);
esp_err_t handle_get_wifi_scan(httpd_req_t *req);
esp_err_t handle_post_wifi_config(httpd_req_t *req);
esp_err_t handle_get_config(httpd_req_t *req);

#endif /* HTTP_SERVER_H */
