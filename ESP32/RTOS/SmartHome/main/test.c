#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

TaskHandle_t task1Handle = NULL, task2Handle = NULL;

static const char* pcTaskState(eTaskState state) {
    switch(state) {
        case eRunning:   return "Running";
        case eReady:     return "Ready";
        case eBlocked:   return "Blocked";
        case eSuspended: return "Suspended";
        case eDeleted:   return "Deleted";
        default:         return "Unknown";
    }
}

void task1(void *pv) {
    for(;;) {
        printf("== Task1 ==\n");
        UBaseType_t hw = uxTaskGetStackHighWaterMark(NULL);
        printf(" T1 high water mark: %u bytes\n", hw * sizeof(StackType_t)); // Đổi sang byte
        if(task1Handle) {
            printf("Task1 state: %s\n", pcTaskState(eTaskGetState(task1Handle)));
        }
        if(task2Handle) {
            printf("Task2 state: %s\n", pcTaskState(eTaskGetState(task2Handle)));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task2(void *pv) {
    for(;;) {
        printf("== Task2 ==\n");
        UBaseType_t hw = uxTaskGetStackHighWaterMark(NULL);
        printf(" T2 high water mark: %u bytes\n", hw * sizeof(StackType_t)); // Đổi sang byte
        if(task1Handle) {
            printf("Task1 state: %s\n", pcTaskState(eTaskGetState(task1Handle)));
        }
        if(task2Handle) {
            printf("Task2 state: %s\n", pcTaskState(eTaskGetState(task2Handle)));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    printf("Hello Deesol!\n");

    // Tăng stack size cho Task2 lên 4096 bytes
    BaseType_t r1 = xTaskCreate(task1, "task1", 2048, NULL, 1, &task1Handle);
    configASSERT(r1 == pdPASS);

    BaseType_t r2 = xTaskCreate(task2, "task2", 4096, NULL, 3, &task2Handle); // Sửa thành 4096
    configASSERT(r2 == pdPASS);

    TaskHandle_t self = xTaskGetCurrentTaskHandle();
    eTaskState selfState = eTaskGetState(self);
    printf("State of app_main: %s\n", pcTaskState(selfState));
    // Giữ app_main lâu một chút để bạn có thể quan sát log trước khi nó tự xóa
    vTaskDelay(pdMS_TO_TICKS(100));

    // Nếu bạn không cần app_main nữa, có thể xóa nó để chỉ còn task1 và task2
    vTaskDelete(NULL);
}
