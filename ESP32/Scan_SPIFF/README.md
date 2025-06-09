| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- |

# ESP32 SPIFFS Project with WiFi Configuration and Firebase Integration

This project demonstrates how to use SPIFFS with ESP32, along with WiFi configuration capabilities, HTTP server functionality, and Firebase integration. The project performs the following:

1. **SPIFFS Operations**:
   - Initializes SPIFFS with optimized settings for faster performance
   - Mounts SPIFFS filesystem using SPIFFS library (and formats if needed)
   - Registers SPIFFS filesystem in VFS, enabling C standard library and POSIX functions
   - Demonstrates file operations: creating, writing, renaming, and reading files

2. **WiFi Configuration**:
   - Provides AP mode for WiFi setup through a web interface
   - Stores configuration in NVS (Non-Volatile Storage)
   - Connects to configured WiFi network automatically on boot

3. **HTTP Server**:
   - Serves configuration pages from SPIFFS storage
   - Handles form submissions for WiFi and server configuration

4. **Firebase Integration**:
   - Securely communicates with Firebase using embedded certificates
   - Sends sensor data to Firebase

## Partition Table Configuration

The project uses a custom partition table defined in `partitions_example.csv`. The partition layout is as follows:

- **nvs**: 0x6000 bytes - For storing WiFi and other configuration
- **phy_init**: 0x1000 bytes - For PHY calibration data
- **factory**: 0x110000 bytes (1.1MB) - For the application binary
- **spiffs**: 0xF0000 bytes (960KB) - For storing web interface files and other data

See [Partition Tables](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/partition-tables.html) documentation for more information.

## Lý do chọn công nghệ

Dự án này kết hợp SPIFFS, cấu hình WiFi và tích hợp Firebase vì những nguyên nhân sau:

- **SPIFFS (SPI Flash File System)**: Được chọn vì khả năng lưu trữ hiệu quả trên bộ nhớ flash SPI của ESP32. Hệ thống này cho phép lưu trữ các tệp cấu hình, trang web và chứng chỉ bảo mật mà không cần bộ nhớ ngoài, giúp thiết bị nhỏ gọn và tiết kiệm chi phí.

- **Cấu hình WiFi**: Cơ chế cấu hình kép (chế độ AP và STA) được triển khai để tạo trải nghiệm người dùng linh hoạt. Người dùng có thể dễ dàng thiết lập thiết bị thông qua giao diện web mà không cần phải biên dịch lại mã nguồn, giúp sản phẩm thân thiện với người dùng không chuyên.

- **Tích hợp Firebase**: Được chọn làm nền tảng đám mây vì khả năng lưu trữ dữ liệu thời gian thực, xác thực bảo mật và khả năng mở rộng. Việc kết nối với Firebase cho phép giám sát từ xa và phân tích dữ liệu cảm biến, biến thiết bị thành một giải pháp IoT hoàn chỉnh.

Sự kết hợp này tạo nên một hệ thống nhúng mạnh mẽ, dễ cấu hình và an toàn, phù hợp cho các ứng dụng IoT thực tế.

## How to use example

### Hardware required

This example does not require any special hardware, and can be run on any common development board.

### Configure the project

* Open the project configuration menu (`idf.py menuconfig`)
* Configure SPIFFS settings under "SPIFFS Example menu". See note about `esp_spiffs_check` function on [SPIFFS Filesystem](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spiffs.html) page.
* Configure WiFi settings if needed

### Build and flash

Replace PORT with serial port name:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example output

Here is an example console output. In this case `format_if_mount_failed` parameter was set to `true` in the source code. SPIFFS was unformatted, so the initial mount has failed. SPIFFS was then formatted, and mounted again.

```
I (324) example: Initializing SPIFFS
W (324) SPIFFS: mount failed, -10025. formatting...
I (19414) example: Partition size: total: 896321, used: 0
I (19414) example: Opening file
I (19504) example: File written
I (19544) example: Renaming file
I (19584) example: Reading file
I (19584) example: Read from file: 'Hello World!'
I (19584) example: SPIFFS unmounted
```

To erase the contents of SPIFFS partition, run `idf.py erase-flash` command. Then upload the example again as described above.