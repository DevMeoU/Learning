idf_component_register(
    SRCS 
        "main.c"
        "gpio_control.c"
        "uart_handler.c"
        "wifi_manager.c"
        "spiffs_handler.c"
        "http_server.c"
        "sntp_handler.c"    
    INCLUDE_DIRS 
        "include"
    EMBED_FILES 
        "firebase_cert.pem"
    REQUIRES
        esp_http_server
        esp_http_client
        nvs_flash
        esp_timer
        esp_wifi
        driver
        spiffs
        esp_netif
        esp-tls
        mbedtls
        spi_flash
        freertos
        esp_event
        esp_common
        json
)

# Create SPIFFS image from 'spiffs_data' directory with partition name 'storage'
spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)