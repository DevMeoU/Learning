
Dưới đây là hướng dẫn chi tiết cách sử dụng endpoint `/config` (config_uri) trong hệ thống của bạn:

**1. Nguyên lý hoạt động của config_uri:**

* Endpoint: `/config`
* Phương thức: POST
* Chức năng: Nhận cấu hình server URL từ client và lưu vào bộ nhớ NVS

**2. Cách sử dụng qua trình duyệt:**

1. Kết nối vào WiFi AP "ESP32_Config"
2. Mở trình duyệt truy cập: `http://192.168.4.1`
3. Điền URL server vào form (ví dụ: `http://192.168.1.100:8080/data`)
4. Nhấn nút Save - Hệ thống tự động gửi POST request đến `/config`

**3. Cách sử dụng qua công cụ API (Postman/cURL):**

**bash**

Copy

Download

```
# Sử dụng cURL
curl -X POST http://192.168.4.1/config \
-H "Content-Type: application/json" \
-d '{"server_url":"http://your-server.com/api/data"}'
```

**4. Cấu trúc dữ liệu yêu cầu:**

**json**

Copy

Download

```
{
  "server_url": "string"
}
```

**5. Flow xử lý:**

Copy

Download

```
Client -> POST /config (JSON data)
       <- 200 OK + Message
ESP32 khởi động lại
ESP32 kết nối WiFi STA
ESP32 gửi dữ liệu cảm biến đến server_url
```

**6. Lỗi thường gặp và cách khắc phục:**

* **Lỗi 400 Bad Request:**
  * Nguyên nhân: Sai định dạng JSON
  * Giải pháp: Kiểm tra lại cấu trúc JSON và encoding
* **Lỗi không lưu được cấu hình:**
  * Nguyên nhân: Full bộ nhớ NVS
  * Giải pháp: Xóa toàn bộ cấu hình bằng lệnh NVS erase
* **Lỗi timeout:**
  * Nguyên nhân: ESP32 không nhận đủ dữ liệu
  * Giải pháp: Tăng kích thước buffer trong post_handler

**7. Gỡ lỗi với config_uri:**
Thêm log debug trong post_handler:

**c**

Copy

Download

```
static esp_err_t post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Received %d bytes payload", req->content_len);
    //... existing code
    ESP_LOGI(TAG, "Parsed server_url: %s", url->valuestring);
}
```

**8. Customize config_uri:**
Thay đổi các tham số trong cấu trúc:

**c**

Copy

Download

```
httpd_uri_t config_uri = {
    .uri = "/new-config-path",  // Thay đổi endpoint
    .method = HTTP_PUT,        // Đổi phương thức
    .handler = post_handler
};
```

**9. Bảo mật:**
Thêm xác thực cho config_uri:

**c**

Copy

Download

```
// Trong post_handler
const char *auth_header = httpd_req_get_header(req, "Authorization");
if (!auth_header || strcmp(auth_header, "Bearer my-secret-token") != 0) {
    httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Invalid token");
    return ESP_FAIL;
}
```

**10. Kiểm tra trạng thái config_uri:**
Thêm endpoint GET để đọc cấu hình:

**c**

Copy

Download

```
// Thêm handler mới
static esp_err_t get_config_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "server_url", server_url);
    char *json_str = cJSON_Print(root);
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
    cJSON_Delete(root);
    free(json_str);
    return ESP_OK;
}

// Đăng ký thêm URI
httpd_uri_t get_config_uri = {
    .uri = "/config",
    .method = HTTP_GET,
    .handler = get_config_handler
};
```
