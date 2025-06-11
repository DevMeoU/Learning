idf_component_register(    SRCS 
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
        "firebase_cert.pem")

# Create SPIFFS image from 'spiffs_data' directory with partition name 'storage'
spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)