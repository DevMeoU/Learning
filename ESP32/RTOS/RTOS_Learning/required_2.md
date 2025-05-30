**Đề bài ứng dụng**

Xây dựng một “Smart Sensor Node” trên STM32 (ví dụ Nucleo-L4) với các yêu cầu sau:

1. **Đọc cảm biến** : Đọc giá trị nhiệt độ giả lập (thiết lập trong phần mềm) mỗi 1 giây.
2. **Blink LED** : Nút LED báo nhịp 500 ms (task riêng).
3. **Giao tiếp UART** : Khi người dùng bấm nút (EXTI), gửi chuỗi “Button Pressed!” qua UART.
4. **Logging** : Ghi chuỗi “Temp: xx.x°C” lên file giả lập (ở RAM) mỗi lần đọc cảm biến.
5. **Yêu cầu đồng bộ và giao tiếp** :

* **Queue** : Đưa giá trị sensor (float) từ task đọc cảm biến sang task logging.
* **Mutex** : Bảo vệ việc in ra UART (cả task nút và task logging cùng dùng UART).
* **Binary Semaphore** : Từ ISR nút bấm tín hiệu cho task UART.
* **Event Group** : Đánh dấu hai sự kiện “sensor ready” và “button pressed” để task chính có thể chờ kết hợp AND/OR.
* **Software Timer** : Định kỳ 200 ms để trigger một callback (vd. watchdog).

---

## Sơ đồ tasks và cơ chế RTOS

| Task name      | Chức năng                                       | Chờ / Block         |
| -------------- | ------------------------------------------------- | -------------------- |
| `SensorTask` | Đọc cảm biến (giả lập) mỗi 1 s, gửi queue | `vTaskDelay(1000)` |
| `LoggerTask` | Nhận queue, ghi RAM “Temp: …” và printf UART | `xQueueReceive()`  |
| `LedTask`    | Toggle LED (PA5) mỗi 500 ms                      | `vTaskDelay(500)`  |
| `UartTask`   | Chờ semaphore từ nút, gửi “Button Pressed!” | `xSemaphoreTake()` |

**Shared resources**

* **`xUartMutex`** : bảo vệ `printf()` hay `HAL_UART_Transmit()`
* **`xSensorQueue`** : chứa giá trị nhiệt độ (float)
* **`xButtonSemaphore`** : binary semaphore từ EXTI_IRQHandler
* **`xEventGroup`** : bit0 = SENSOR_READY, bit1 = BUTTON_PRESSED

---

## Mã nguồn minh họa (FreeRTOS + STM32 HAL)

<pre class="overflow-visible!" data-start="1915" data-end="5743"><div class="contain-inline-size rounded-md border-[0.5px] border-token-border-medium relative bg-token-sidebar-surface-primary"><div class="flex items-center text-token-text-secondary px-4 py-2 text-xs font-sans justify-between h-9 bg-token-sidebar-surface-primary dark:bg-token-main-surface-secondary select-none rounded-t-[5px]">c</div><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-sidebar-surface-primary text-token-text-secondary dark:bg-token-main-surface-secondary flex items-center rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"><button class="flex gap-1 items-center select-none px-4 py-1" aria-label="Copy"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path fill-rule="evenodd" clip-rule="evenodd" d="M7 5C7 3.34315 8.34315 2 10 2H19C20.6569 2 22 3.34315 22 5V14C22 15.6569 20.6569 17 19 17H17V19C17 20.6569 15.6569 22 14 22H5C3.34315 22 2 20.6569 2 19V10C2 8.34315 3.34315 7 5 7H7V5ZM9 7H14C15.6569 7 17 8.34315 17 10V15H19C19.5523 15 20 14.5523 20 14V5C20 4.44772 19.5523 4 19 4H10C9.44772 4 9 4.44772 9 5V7ZM5 9C4.44772 9 4 9.44772 4 10V19C4 19.5523 4.44772 20 5 20H14C14.5523 20 15 19.5523 15 19V10C15 9.44772 14.5523 9 14 9H5Z" fill="currentColor"></path></svg>Copy</button></span><span class="" data-state="closed"><button class="flex items-center gap-1 px-4 py-1 select-none"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path d="M2.5 5.5C4.3 5.2 5.2 4 5.5 2.5C5.8 4 6.7 5.2 8.5 5.5C6.7 5.8 5.8 7 5.5 8.5C5.2 7 4.3 5.8 2.5 5.5Z" fill="currentColor" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"></path><path d="M5.66282 16.5231L5.18413 19.3952C5.12203 19.7678 5.09098 19.9541 5.14876 20.0888C5.19933 20.2067 5.29328 20.3007 5.41118 20.3512C5.54589 20.409 5.73218 20.378 6.10476 20.3159L8.97693 19.8372C9.72813 19.712 10.1037 19.6494 10.4542 19.521C10.7652 19.407 11.0608 19.2549 11.3343 19.068C11.6425 18.8575 11.9118 18.5882 12.4503 18.0497L20 10.5C21.3807 9.11929 21.3807 6.88071 20 5.5C18.6193 4.11929 16.3807 4.11929 15 5.5L7.45026 13.0497C6.91175 13.5882 6.6425 13.8575 6.43197 14.1657C6.24513 14.4392 6.09299 14.7348 5.97903 15.0458C5.85062 15.3963 5.78802 15.7719 5.66282 16.5231Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path><path d="M14.5 7L18.5 11" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path></svg>Edit</button></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-c"><span><span>/* Includes ------------------------------------------------------------------*/</span><span>
</span><span>#include</span><span></span><span>"stm32l4xx_hal.h"</span><span>
</span><span>#include</span><span></span><span>"FreeRTOS.h"</span><span>
</span><span>#include</span><span></span><span>"task.h"</span><span>
</span><span>#include</span><span></span><span>"queue.h"</span><span>
</span><span>#include</span><span></span><span>"semphr.h"</span><span>
</span><span>#include</span><span></span><span>"event_groups.h"</span><span>
</span><span>#include</span><span></span><span>"timers.h"</span><span>

</span><span>/* Definitions ----------------------------------------------------------------*/</span><span>
</span><span>#define</span><span> SENSOR_READY_BIT   (1<<0)
</span><span>#define</span><span> BUTTON_PRESSED_BIT (1<<1)

</span><span>/* Handles --------------------------------------------------------------------*/</span><span>
SemaphoreHandle_t xUartMutex;
SemaphoreHandle_t xButtonSemaphore;
QueueHandle_t     xSensorQueue;
EventGroupHandle_t xEventGroup;
TimerHandle_t     xWatchdogTimer;

</span><span>/* Prototypes -----------------------------------------------------------------*/</span><span>
</span><span>void</span><span></span><span>SystemClock_Config</span><span>(void</span><span>);
</span><span>void</span><span></span><span>MX_GPIO_Init</span><span>(void</span><span>);
</span><span>void</span><span></span><span>MX_USART2_UART_Init</span><span>(void</span><span>);

</span><span>void</span><span></span><span>SensorTask</span><span>(void</span><span> *pv);
</span><span>void</span><span></span><span>LoggerTask</span><span>(void</span><span> *pv);
</span><span>void</span><span></span><span>LedTask</span><span>(void</span><span> *pv);
</span><span>void</span><span></span><span>UartTask</span><span>(void</span><span> *pv);
</span><span>void</span><span></span><span>WatchdogCallback</span><span>(TimerHandle_t xTimer)</span><span>;

</span><span>/* Main -----------------------------------------------------------------------*/</span><span>
</span><span>int</span><span></span><span>main</span><span>(void</span><span>)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  </span><span>/* Tạo RTOS objects */</span><span>
  xUartMutex       = xSemaphoreCreateMutex();
  xButtonSemaphore = xSemaphoreCreateBinary();
  xSensorQueue     = xQueueCreate( </span><span>10</span><span>, </span><span>sizeof</span><span>(</span><span>float</span><span>) );
  xEventGroup      = xEventGroupCreate();
  xWatchdogTimer   = xTimerCreate(</span><span>"WD"</span><span>, pdMS_TO_TICKS(</span><span>200</span><span>), pdTRUE, </span><span>NULL</span><span>, WatchdogCallback);

  </span><span>/* Các task */</span><span>
  xTaskCreate(SensorTask, </span><span>"Sensor"</span><span>, </span><span>128</span><span>, </span><span>NULL</span><span>, </span><span>2</span><span>, </span><span>NULL</span><span>);
  xTaskCreate(LoggerTask, </span><span>"Logger"</span><span>, </span><span>256</span><span>, </span><span>NULL</span><span>, </span><span>1</span><span>, </span><span>NULL</span><span>);
  xTaskCreate(LedTask,    </span><span>"LED"</span><span>,    </span><span>128</span><span>, </span><span>NULL</span><span>, </span><span>1</span><span>, </span><span>NULL</span><span>);
  xTaskCreate(UartTask,   </span><span>"UART"</span><span>,   </span><span>128</span><span>, </span><span>NULL</span><span>, </span><span>2</span><span>, </span><span>NULL</span><span>);

  </span><span>/* Start timer và scheduler */</span><span>
  xTimerStart(xWatchdogTimer, </span><span>0</span><span>);
  vTaskStartScheduler();

  </span><span>while</span><span>(</span><span>1</span><span>); </span><span>/* Không tới đây */</span><span>
}

</span><span>/* SensorTask: đọc sensor giả lập, gửi queue và set event */</span><span>
</span><span>void</span><span></span><span>SensorTask</span><span>(void</span><span> *pv)
{
  </span><span>float</span><span> fakeTemp = </span><span>25.0f</span><span>;
  </span><span>for</span><span>(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(</span><span>1000</span><span>));
    fakeTemp += </span><span>0.1f</span><span>;  </span><span>// giả lập thay đổi</span><span>
    xQueueSend(xSensorQueue, &fakeTemp, portMAX_DELAY);
    xEventGroupSetBits(xEventGroup, SENSOR_READY_BIT);
  }
}

</span><span>/* LoggerTask: nhận từ queue, ghi RAM + UART qua mutex */</span><span>
</span><span>void</span><span></span><span>LoggerTask</span><span>(void</span><span> *pv)
{
  </span><span>float</span><span> recvTemp;
  </span><span>char</span><span> buf[</span><span>32</span><span>];
  </span><span>for</span><span>(;;)
  {
    xEventGroupWaitBits(xEventGroup, SENSOR_READY_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    xQueueReceive(xSensorQueue, &recvTemp, </span><span>0</span><span>);
    </span><span>sprintf</span><span>(buf, </span><span>"Temp: %.1f C\r\n"</span><span>, recvTemp);

    </span><span>/* Ghi RAM (giả lập) */</span><span>
    </span><span>// ram_log_push(buf);</span><span>

    </span><span>/* In qua UART */</span><span>
    xSemaphoreTake(xUartMutex, portMAX_DELAY);
      HAL_UART_Transmit(&huart2, (</span><span>uint8_t</span><span>*)buf, </span><span>strlen</span><span>(buf), HAL_MAX_DELAY);
    xSemaphoreGive(xUartMutex);
  }
}

</span><span>/* LedTask: toggle PA5 */</span><span>
</span><span>void</span><span></span><span>LedTask</span><span>(void</span><span> *pv)
{
  </span><span>for</span><span>(;;)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    vTaskDelay(pdMS_TO_TICKS(</span><span>500</span><span>));
  }
}

</span><span>/* UartTask: chờ nút qua semaphore, gửi message */</span><span>
</span><span>void</span><span></span><span>UartTask</span><span>(void</span><span> *pv)
{
  </span><span>for</span><span>(;;)
  {
    xSemaphoreTake(xButtonSemaphore, portMAX_DELAY);
    xEventGroupSetBits(xEventGroup, BUTTON_PRESSED_BIT);

    xSemaphoreTake(xUartMutex, portMAX_DELAY);
      </span><span>char</span><span> *msg = </span><span>"Button Pressed!\r\n"</span><span>;
      HAL_UART_Transmit(&huart2, (</span><span>uint8_t</span><span>*)msg, </span><span>strlen</span><span>(msg), HAL_MAX_DELAY);
    xSemaphoreGive(xUartMutex);
  }
}

</span><span>/* Callback timer: giả lập watchdog */</span><span>
</span><span>void</span><span></span><span>WatchdogCallback</span><span>(TimerHandle_t xTimer)</span><span>
{
  </span><span>/* ... có thể reset CPU nếu cần ... */</span><span>
}

</span><span>/* EXTI interrupt cho nút user button (PA13) */</span><span>
</span><span>void</span><span></span><span>EXTI15_10_IRQHandler</span><span>(void</span><span>)
{
  </span><span>if</span><span>(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13))
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

</span><span>/* Nhớ: config PA5, PA2/PA3 (UART2), PA13 exti, clock, UART2 trong MX_xxx hàm */</span><span>
</span></span></code></div></div></pre>

---

### Giải thích cách dùng đầy đủ:

1. **Task**
   * `xTaskCreate()` khởi tạo Sensor/Logger/LED/Uart.
2. **Queue**
   * `xSensorQueue` truyền `float` giữa SensorTask → LoggerTask.
3. **Mutex**
   * `xUartMutex` bảo vệ chung tài nguyên UART trong hai task LoggerTask và UartTask.
4. **Binary Semaphore**
   * `xButtonSemaphore` do ISR EXTI cấp, UartTask chờ và phát tín hiệu khi nút bấm.
5. **Event Group**
   * `xEventGroup` chứa hai bit:
     * `SENSOR_READY_BIT` để LoggerTask được wake sau Sensor đọc xong.
     * `BUTTON_PRESSED_BIT` có thể dùng nếu một task khác muốn chờ cả hai sự kiện.
6. **Software Timer**
   * `xWatchdogTimer` gọi `WatchdogCallback()` mỗi 200 ms (ta có thể reset watchdog thật hoặc làm log).
