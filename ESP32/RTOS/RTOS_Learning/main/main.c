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

void app_main(void)
{
    xTaskCreatePinnedToCore(increment_task, "Task0", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(increment_task, "Task1", 4096, NULL, 1, NULL, 1);
}
