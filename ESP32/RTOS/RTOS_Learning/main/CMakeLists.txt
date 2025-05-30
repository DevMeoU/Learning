idf_component_register(SRCS "smarthome.c"
                    PRIV_REQUIRES spi_flash esp_driver_ledc esp_driver_gpio
                    INCLUDE_DIRS ".")
