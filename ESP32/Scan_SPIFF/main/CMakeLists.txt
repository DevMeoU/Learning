idf_component_register(SRCS "main.c" "apmode_cfg.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES "firebase_cert.pem")

# Tạo image SPIFFS từ thư mục 'spiffs_data' với tên partition là 'spiffs' (chuẩn ESP-IDF)
spiffs_create_partition_image(spiffs ../spiffs_data FLASH_IN_PROJECT)