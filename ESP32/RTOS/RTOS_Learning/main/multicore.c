/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

 #include <stdio.h>
 #include <inttypes.h>
 #include "sdkconfig.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "esp_chip_info.h"
 #include "esp_flash.h"
 #include "esp_system.h"
 
 // Sửa thành portMUX_TYPE cho ESP-IDF
 static portMUX_TYPE counter_spinlock = portMUX_INITIALIZER_UNLOCKED;
 volatile int shared_counter = 0;
 
 void increment_task(void *pvParameter) {
     while (1) {
         // Sử dụng portENTER_CRITICAL thay cho taskENTER_CRITICAL
         portENTER_CRITICAL(&counter_spinlock);
         shared_counter++;
         
         // Di chuyển printf() RA NGOÀI critical section
         int current_counter = shared_counter;
         portEXIT_CRITICAL(&counter_spinlock); // Nhả spinlock trước khi in
         
         printf("[Core %d] Counter: %d\n", xPortGetCoreID(), current_counter);
         
         vTaskDelay(100 / portTICK_PERIOD_MS);
     }
 }
 
 // SemaphoreHandle_t mutex;
 // volatile int shared_counter = 0;
 
 // // Hàm in ra core ID và thông điệp
 // void task_function(void *pvParameter) {
 //     while (1) {
 //         int core_id = xPortGetCoreID();
 //         printf("Task running on Core %d\n", core_id);
 //         vTaskDelay(1000 / portTICK_PERIOD_MS);
 //     }
 // }
 
 // void task_increment(void *pvParameter) {
 //     while (1) {
 //         if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
 //             shared_counter++;
 //             printf("Core %d: Counter = %d\n", xPortGetCoreID(), shared_counter);
 //             xSemaphoreGive(mutex);
 //         }
 //         vTaskDelay(500 / portTICK_PERIOD_MS);
 //     }
 // }
 
 void app_main(void)
 {
     // // Tạo Task 0 chạy trên Core 0
     // xTaskCreatePinnedToCore(
     //     task_function,   // Hàm thực thi
     //     "Task0",         // Tên task
     //     4096,            // Kích thước stack
     //     NULL,            // Tham số
     //     1,               // Độ ưu tiên
     //     NULL,            // Task handle
     //     0                // Core ID (0 hoặc 1)
     // );
 
     // // Tạo Task 1 chạy trên Core 1
     // xTaskCreatePinnedToCore(
     //     task_function,
     //     "Task1",
     //     4096,
     //     NULL,
     //     1,
     //     NULL,
     //     1
     // );
 
     // mutex = xSemaphoreCreateMutex(); // Khởi tạo mutex
 
     // xTaskCreatePinnedToCore(task_increment, "Task0", 4096, NULL, 1, NULL, 0);
     // xTaskCreatePinnedToCore(task_increment, "Task1", 4096, NULL, 1, NULL, 1);
 
     xTaskCreatePinnedToCore(increment_task, "Task0", 4096, NULL, 1, NULL, 0);
     xTaskCreatePinnedToCore(increment_task, "Task1", 4096, NULL, 1, NULL, 1);
 }
 