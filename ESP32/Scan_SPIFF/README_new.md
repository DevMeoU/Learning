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

SPIFFS partition size is set in partitions_example.csv file (0xF0000 bytes). See [Partition Tables](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/partition-tables.html) documentation for more information.

## How to use example

### Hardware required

This example requires:
- Any ESP32 development board (ESP32, ESP32-S2, ESP32-S3, etc.)
- USB cable for power and programming
- Optional: LEDs and buttons for physical interface (see pin definitions in code)

### Configure the project

* Open the project configuration menu (`idf.py menuconfig`)
* Configure SPIFFS settings under "SPIFFS Example menu". See note about `esp_spiffs_check` function on [SPIFFS Filesystem](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spiffs.html) page.
* Configure WiFi settings if needed

### Build and flash

To build for a specific ESP32 target, use:

```
idf.py set-target [TARGET]
```

Where [TARGET] can be esp32, esp32s2, esp32s3, etc.

Then build, flash, and monitor:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Performance Optimizations

This project includes several optimizations for improved performance:

1. **SPIFFS Configuration**:
   - Increased max_files from 5 to 10 to allow more concurrent file operations
   - Optimized file handling for faster read/write operations

2. **HTTP Server**:
   - Efficient handling of requests for faster response times
   - Serving static files from SPIFFS for better performance

3. **Memory Management**:
   - Proper allocation and deallocation of resources
   - Careful file handling with proper open/close operations

## Usage Instructions

1. Power on the device
2. Press and hold the button for 5 seconds to enter AP mode (if not already configured)
3. Connect to the "ESP32_Config" WiFi network with password "123456789"
4. Open a web browser and navigate to the device's IP address
5. Configure your WiFi settings and server URL
6. The device will connect to your WiFi network and begin normal operation

## Troubleshooting

To erase the contents of SPIFFS partition and reset all configurations:
```
idf.py erase-flash
```

Then upload the example again as described above.