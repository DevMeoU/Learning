/**
 * @file main.c
 * @brief A simple RTOS implementation for STM32
 * @details This file implements a basic RTOS with task scheduling, context switching,
 * and peripheral management for STM32 microcontrollers.
 *
 * Tệp này triển khai một RTOS cơ bản với lập lịch task, chuyển đổi ngữ cảnh,
 * và quản lý ngoại vi cho vi điều khiển STM32.
 *
 * Memory Layout / Bố trí bộ nhớ:
 *
 * SRAM (64KB)
 * 0x20000000 +------------------+
 *            |     Reserved     |
 *            |----------------  |
 *            |  Scheduler Stack |
 * SCHED_STACK|       5KB       |
 *            |----------------  |
 *            |   Idle Stack    |
 * IDLE_STACK |       5KB       |
 *            |----------------  |
 *            |   Task4 Stack   |
 * TASK4_STACK|       5KB       |
 *            |----------------  |
 *            |   Task3 Stack   |
 * TASK3_STACK|       5KB       |
 *            |----------------  |
 *            |   Task2 Stack   |
 * TASK2_STACK|       5KB       |
 *            |----------------  |
 *            |   Task1 Stack   |
 * TASK1_STACK|       5KB       |
 *            |----------------  |
 *            |  System Memory  |
 * 0x20000000 +------------------+
 *
 * Process Stack Pointer (PSP):
 * - Mỗi task có một PSP riêng trỏ đến stack của task đó
 * - Khi chuyển task, PSP được cập nhật để trỏ đến stack của task mới
 * - Stack frame của task chứa các thanh ghi (R0-R12, LR, PC, xPSR)
 * - PSP được lưu trong TCB của mỗi task
 *
 * Context Switch / Chuyển đổi ngữ cảnh:
 * 1. Lưu ngữ cảnh task hiện tại vào stack (thông qua PSP)
 * 2. Cập nhật PSP trong TCB của task hiện tại
 * 3. Chọn task tiếp theo để chạy
 * 4. Khôi phục PSP từ TCB của task mới
 * 5. Khôi phục ngữ cảnh task mới từ stack
 */

#include < stdio.h > #include < stdint.h >

// -----------------------------
// System Configuration Cấu hình hệ thống
// -----------------------------

// Memory and Stack Configuration Cấu hình bộ nhớ và ngăn xếp
#define DEFAULT_STACK_SIZE(5 * 1024) // Default 5KB stack size / Kích thước ngăn xếp mặc định 5KB
#define SRAM_START(0x20000000 U) // SRAM start address / Địa chỉ bắt đầu SRAM
#define SRAM_SIZE(64 * 1024) // 64KB SRAM
#define SRAM_END(SRAM_START + SRAM_SIZE)
#define SCHEDULER_STACK_SIZE(5 * 1024) // 5KB for scheduler / 5KB cho bộ lập lịch

// Stack Management Quản lý ngăn xếp
#define SCHEDULER_STACK_START(SRAM_END - SCHEDULER_STACK_SIZE)
#define TASK_STACK_START(SCHEDULER_STACK_START - DEFAULT_STACK_SIZE) // Start of task stack area

// System Timer Configuration Cấu hình bộ định thời hệ thống
#define SYSTICK_FREQ_HZ(1000) // 1ms tick interval / Chu kỳ tick 1ms
#define CPU_CLOCK_FREQ_HZ(16000000) // 16MHz CPU clock / Xung nhịp CPU 16MHz

// Task Configuration Cấu hình task
#define MAX_TASKS(10) // Maximum number of tasks / Số lượng task tối đa
#define TASK_RUNNING(0x00) // Task is running / Task đang chạy
#define TASK_BLOCKED(0x01) // Task is blocked / Task bị chặn
#define TASK_READY(0x02) // Task is ready / Task sẵn sàng
#define TASK_DELETED(0x03) // Task is deleted / Task đã bị xóa

// Interrupt Control Macros Macro điều khiển ngắt
#define DISABLE_INTERRUPTS()do {
    __asm volatile("MOV R0, #0x01");
    \
    __asm volatile("MSR PRIMASK, R0");
} while (0)
#define ENABLE_INTERRUPTS()do {
    __asm volatile("MOV R0, #0x00");
    \
    __asm volatile("MSR PRIMASK, R0");
} while (0)

// Peripheral Base Addresses Địa chỉ cơ sở các ngoại vi
#define RCC_BASE(0x40021000 U)
#define GPIOA_BASE(0x40020000 U)
#define LPUART1_BASE(0x40008000 U)

// RCC Registers Các thanh ghi RCC
#define RCC_AHB1ENR( * (volatile uint32_t *)(RCC_BASE + 0x48 U))
#define RCC_APB1ENR2( * (volatile uint32_t *)(RCC_BASE + 0x5C U))

// GPIO Registers Các thanh ghi GPIO
#define GPIOA_MODER( * (volatile uint32_t *)(GPIOA_BASE + 0x00 U))
#define GPIOA_OTYPER( * (volatile uint32_t *)(GPIOA_BASE + 0x04 U))
#define GPIOA_OSPEEDR( * (volatile uint32_t *)(GPIOA_BASE + 0x08 U))
#define GPIOA_PUPDR( * (volatile uint32_t *)(GPIOA_BASE + 0x0C U))
#define GPIOA_ODR( * (volatile uint32_t *)(GPIOA_BASE + 0x14 U))

// LPUART Registers Các thanh ghi LPUART
#define LPUART1_CR1( * (volatile uint32_t *)(LPUART1_BASE + 0x00 U))
#define LPUART1_BRR( * (volatile uint32_t *)(LPUART1_BASE + 0x0C U))
#define LPUART1_ISR( * (volatile uint32_t *)(LPUART1_BASE + 0x1C U))
#define LPUART1_TDR( * (volatile uint32_t *)(LPUART1_BASE + 0x28 U))

// -----------------------------
// Task Control Block (TCB) Khối điều khiển task (TCB)
// -----------------------------
typedef struct {
    uint32_t stack_ptr; // Stack pointer / Con trỏ ngăn xếp
    uint32_t stack_start; // Stack start address / Địa chỉ bắt đầu ngăn xếp
    uint32_t stack_size; // Stack size / Kích thước ngăn xếp
    uint32_t block_count; // Blocking counter / Bộ đếm chặn
    uint8_t state; // Task state / Trạng thái task
    uint8_t priority; // Task priority / Độ ưu tiên của task
    void( * handler)(void); // Task handler function / Hàm xử lý task
}
task_control_block_t;

// Global Variables Các biến toàn cục
static task_control_block_t task_list[MAX_TASKS]; // Task list / Danh sách task
static uint32_t current_task = 0; // Current task index / Chỉ số task hiện tại
static uint32_t next_task_id = 1; // Next available task ID / ID task tiếp theo
static uint32_t system_tick_count = 0; // System tick counter / Bộ đếm tick hệ thống
static uint32_t next_stack_address = TASK_STACK_START; // Next available stack address / Địa chỉ ngăn xếp tiếp theo

// Function Prototypes Khai báo nguyên mẫu hàm
void idle_task_handler(void);
void init_systick_timer(uint32_t tick_hz);
__attribute__((naked))void init_scheduler_stack(uint32_t sched_top);
void init_task_stacks(void);
void enable_processor_faults(void);
uint32_t get_psp_value(void);
void save_psp_value(uint32_t psp_val);
__attribute__((naked))void switch_sp_to_psp(void);
void update_next_task(void);
void init_led_gpio(void);
void task_delay(uint32_t delay_ticks);
void init_uart(void);
void uart_send_char(char c);
void uart_send_string(const char * str);

// Task Management Functions Các hàm quản lý task
uint8_t create_task(
    void( * handler)(void),
    uint32_t stack_size,
    uint8_t priority
);
void delete_task(uint8_t task_id);
void init_task(
    uint8_t task_id,
    void( * handler)(void),
    uint32_t stack_size,
    uint8_t priority
);

// -----------------------------
// Main Function Hàm chính
// -----------------------------
// Example task handler / Hàm xử lý task mẫu
void example_task_handler(void) {
    while (1) {
        uart_send_string("Example task running\n");
        GPIOA_ODR |= (1 U << 5); // LED on / Bật LED
        task_delay(1000); // 1 second delay / Trễ 1 giây
        GPIOA_ODR &= ~ (1 U << 5); // LED off / Tắt LED
        task_delay(1000); // 1 second delay / Trễ 1 giây
    }
}

int main(void) {
    // Initialize system components / Khởi tạo các thành phần hệ thống
    enable_processor_faults();
    init_scheduler_stack(SCHEDULER_STACK_START);
    init_task_stacks();
    init_systick_timer(SYSTICK_FREQ_HZ);

    // Initialize peripherals / Khởi tạo ngoại vi
    init_led_gpio();
    init_uart();

    // Create example task / Tạo task mẫu
    create_task(example_task_handler, DEFAULT_STACK_SIZE, 1);

    // Start task execution / Bắt đầu thực thi task
    switch_sp_to_psp();

    // Start with idle task / Bắt đầu với task rỗng
    idle_task_handler();

    // Should never reach here / Không bao giờ nên đến đây
    for (;;) ;
    }

// -----------------------------
// Task Handlers Các hàm xử lý task
// -----------------------------

void idle_task_handler(void) {
    while (1) {
        // Idle task - does nothing / Task rỗng - không làm gì
    }
}

void task1_handler(void) {
    while (1) {
        uart_send_string("Task 1 running\n");
        GPIOA_ODR |= (1 U << 5); // LED on / Bật LED
        task_delay(1000); // 1 second delay / Trễ 1 giây
        GPIOA_ODR &= ~ (1 U << 5); // LED off / Tắt LED
        task_delay(1000); // 1 second delay / Trễ 1 giây
    }
}

void task2_handler(void) {
    while (1) {
        uart_send_string("Task 2 running\n");
        task_delay(500); // 500ms delay / Trễ 500ms
    }
}

void task3_handler(void) {
    while (1) {
        uart_send_string("Task 3 running\n");
        task_delay(250); // 250ms delay / Trễ 250ms
    }
}

void task4_handler(void) {
    while (1) {
        uart_send_string("Task 4 running\n");
        task_delay(2000); // 2 seconds delay / Trễ 2 giây
    }
}

// -----------------------------
// Peripheral Initialization Khởi tạo ngoại vi
// -----------------------------

void init_led_gpio(void) {
    // Enable GPIOA clock / Bật clock cho GPIOA
    RCC_AHB1ENR |= (1 U << 0);

    // Configure PA5 as output / Cấu hình PA5 là đầu ra
    GPIOA_MODER &= ~ (3 U << (5 * 2));
    GPIOA_MODER |= (1 U << (5 * 2));

    // Configure as push-pull output / Cấu hình đầu ra dạng push-pull
    GPIOA_OTYPER &= ~ (1 U << 5);

    // Set high speed / Đặt tốc độ cao
    GPIOA_OSPEEDR |= (3 U << (5 * 2));

    // No pull-up/pull-down / Không có pull-up/pull-down
    GPIOA_PUPDR &= ~ (3 U << (5 * 2));
}

void init_uart(void) {
    // Enable LPUART1 and GPIOA clocks / Bật clock cho LPUART1 và GPIOA
    RCC_APB1ENR2 |= (1 U << 0);
    RCC_AHB1ENR |= (1 U << 0);

    // Configure PA2 for UART / Cấu hình PA2 cho UART
    GPIOA_MODER &= ~ (3 U << (2 * 2));
    GPIOA_MODER |= (2 U << (2 * 2));

    // Set alternate function / Đặt chức năng thay thế
    uint32_t * GPIOA_AFRL = (uint32_t *)(GPIOA_BASE + 0x20 U);
    *GPIOA_AFRL &= ~ (0xF << (4 * 2));
    *GPIOA_AFRL |= (8 U << (4 * 2));

    // Configure UART / Cấu hình UART
    LPUART1_CR1 = 0;
    LPUART1_BRR = (CPU_CLOCK_FREQ_HZ / 9600);
    LPUART1_CR1 |= (1 U << 3); // Enable transmitter / Bật bộ phát
    LPUART1_CR1 |= (1 U << 0); // Enable UART / Bật UART
}

// -----------------------------
// UART Communication Giao tiếp UART
// -----------------------------

void uart_send_char(char c) {
    // Wait until transmit buffer is empty Chờ cho đến khi bộ đệm truyền rỗng
    while (!(LPUART1_ISR & (1 U << 7))) 
    ;
    LPUART1_TDR = c;
}

void uart_send_string(const char * str) {
    while ( * str) 
        uart_send_char( * str++);
    }

// -----------------------------
// Task Management Quản lý task
// -----------------------------

void init_task_stacks(void) {
    // Initialize all task slots as deleted Khởi tạo tất cả các slot task là đã xóa
    for (int i = 0; i < MAX_TASKS; i++) {
        task_list[i].state = TASK_DELETED;
    }

    // Create idle task Tạo task rỗng
    create_task(idle_task_handler, DEFAULT_STACK_SIZE, 0);
}

void init_task(
    uint8_t task_id,
    void( * handler)(void),
    uint32_t stack_size,
    uint8_t priority
) {
    // Calculate stack addresses Tính toán địa chỉ ngăn xếp
    uint32_t stack_start = next_stack_address - stack_size;
    next_stack_address = stack_start;

    // Initialize TCB Khởi tạo TCB
    task_list[task_id].handler = handler;
    task_list[task_id].stack_start = stack_start;
    task_list[task_id].stack_size = stack_size;
    task_list[task_id].priority = priority;
    task_list[task_id].state = TASK_READY;

    // Initialize stack Khởi tạo ngăn xếp
    uint32_t * psp = (uint32_t *)stack_start;

    *(--psp) = 0x01000000; // xPSR (Thumb bit set)
    *(--psp) = (uint32_t)handler; // PC
    *(--psp) = 0xFFFFFFFD; // LR (return to thread with PSP)

    // Initialize R0-R12 with 0 Khởi tạo R0-R12 với giá trị 0
    for (int j = 0; j < 13; j++) { * (--psp) = 0;
    }

    task_list[task_id].stack_ptr = (uint32_t)psp;
}

uint8_t create_task(
    void( * handler)(void),
    uint32_t stack_size,
    uint8_t priority
) {
    // Find free task slot Tìm slot task trống
    uint8_t task_id = 0xFF;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].state == TASK_DELETED) {
            task_id = i;
            break;
        }
    }

    // Check if we found a free slot Kiểm tra xem có tìm được slot trống không
    if (task_id == 0xFF) {
        return 0xFF; // No free slots / Không có slot trống
    }

    // Check if we have enough stack space Kiểm tra xem có đủ không gian ngăn xếp
    // không
    if (next_stack_address - stack_size < SRAM_START) {
        return 0xFF; // Not enough stack space / Không đủ không gian ngăn xếp
    }

    // Initialize the task Khởi tạo task
    init_task(task_id, handler, stack_size, priority);

    return task_id;
}

void delete_task(uint8_t task_id) {
    if (task_id >= MAX_TASKS || task_id == 0) {
        return; // Invalid task ID or attempt to delete idle task
        // ID task không hợp lệ hoặc cố gắng xóa task rỗng
    }

    DISABLE_INTERRUPTS();
    task_list[task_id].state = TASK_DELETED;
    ENABLE_INTERRUPTS();

    if (task_id == current_task) {
        schedule(); // Reschedule if deleting current task
        // Lập lịch lại nếu xóa task hiện tại
    }
}

void update_next_task(void) {
    // Priority-based round-robin scheduling Lập lịch vòng tròn dựa trên độ ưu tiên
    uint8_t highest_priority = 0;
    uint8_t next_task = 0;

    // Find highest priority among ready tasks Tìm độ ưu tiên cao nhất trong các
    // task sẵn sàng
    for (int i = 1; i < MAX_TASKS; i++) {
        if ((task_list[i].state == TASK_READY || task_list[i].state == TASK_RUNNING) && task_list[i].priority >= highest_priority) {
            highest_priority = task_list[i].priority;
        }
    }

    // Find next task with highest priority Tìm task tiếp theo có độ ưu tiên cao
    // nhất
    int start = (current_task + 1) % MAX_TASKS;
    for (int i = 0; i < MAX_TASKS; i++) {
        int task_idx = (start + i) % MAX_TASKS;
        if (task_idx != 0 && // Skip idle task / Bỏ qua task rỗng
        (
            task_list[task_idx].state == TASK_READY || task_list[task_idx].state == TASK_RUNNING
        ) && task_list[task_idx].priority == highest_priority) {
            next_task = task_idx;
            break;
        }
    }

    // Update task states Cập nhật trạng thái task
    if (current_task != 0 && task_list[current_task].state == TASK_RUNNING) {
        task_list[current_task].state = TASK_READY;
    }

    current_task = next_task;
    if (current_task != 0) {
        task_list[current_task].state = TASK_RUNNING;
    }
}

// -----------------------------
// System Timer and Interrupts Bộ định thời và ngắt hệ thống
// -----------------------------

void init_systick_timer(uint32_t tick_hz) {
    uint32_t * pSRVR = (uint32_t *)0xE000E014; // SysTick Reload Value Register
    uint32_t * pSCSR = (uint32_t *)0xE000E010; // SysTick Control and Status Register

    // Calculate reload value / Tính giá trị nạp lại
    uint32_t count = (CPU_CLOCK_FREQ_HZ / tick_hz) - 1;

    // Clear current value / Xóa giá trị hiện tại
    *pSCSR &= ~ 0x00FFFFFF;

    // Load reload value / Nạp giá trị
    *pSRVR = count;

    // Enable SysTick / Bật SysTick
    *pSCSR = 0x00000007;

    // Enable SysTick interrupt / Bật ngắt SysTick
    uint32_t * pNVIC_ISER = (uint32_t *)0xE000E100;
    *pNVIC_ISER |= (1 << 15);
}

void enable_processor_faults(void) {
    // Enable usage, bus and memory faults Bật các lỗi sử dụng, bus và bộ nhớ
    uint32_t * pSHCSR = (uint32_t *)0xE000ED24;
    *pSHCSR |= (1 << 16) | (1 << 17) | (1 << 18);
}

// -----------------------------
// Context Switching Chuyển đổi ngữ cảnh
// -----------------------------

__attribute__((naked))void init_scheduler_stack(uint32_t sched_top) {
    __asm volatile("MSR MSP, %0" : : "r" (sched_top));
    __asm volatile("BX LR");
}

__attribute__((naked))void switch_sp_to_psp(void) {
    // Initialize PSP with current task stack Khởi tạo PSP với ngăn xếp task hiện
    // tại
    __asm volatile("PUSH {LR}");
    __asm volatile("BL get_psp_value");
    __asm volatile("MSR PSP, R0");
    __asm volatile("POP {LR}");

    // Switch to use PSP / Chuyển sang sử dụng PSP
    __asm volatile("MOV R0, #0x02");
    __asm volatile("MSR CONTROL, R0");
    __asm volatile("BX LR");
}

__attribute__((naked))void PendSV_Handler(void) {
    // Save current task context / Lưu ngữ cảnh task hiện tại
    __asm volatile("MRS R0, PSP");
    __asm volatile("STMDB R0!, {R4-R11}");
    __asm volatile("PUSH {LR}");

    // Save PSP / Lưu PSP
    __asm volatile("BL save_psp_value");

    // Update next task / Cập nhật task tiếp theo
    __asm volatile("BL update_next_task");

    // Get new task's PSP / Lấy PSP của task mới
    __asm volatile("BL get_psp_value");

    // Restore new task context / Khôi phục ngữ cảnh task mới
    __asm volatile("LDMIA R0!, {R4-R11}");
    __asm volatile("MSR PSP, R0");

    __asm volatile("POP {LR}");
    __asm volatile("BX LR");
}

// -----------------------------
// Task Scheduling Support Hỗ trợ lập lịch task
// -----------------------------

void update_global_tick_count(void) {
    system_tick_count++;
}

void unblock_tasks(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].state == TASK_BLOCKED) {
            if (task_list[i].block_count <= system_tick_count) {
                task_list[i].state = TASK_RUNNING;
            }
        }
    }
}

void SysTick_Handler(void) {
    uint32_t * pICSR = (uint32_t *)0xE000ED04;

    update_global_tick_count();
    unblock_tasks();

    // Trigger PendSV for context switch Kích hoạt PendSV để chuyển đổi ngữ cảnh
    *pICSR = 0x10000000;
}

void schedule(void) {
    uint32_t * pICSR = (uint32_t *)0xE000ED04;
    *pICSR = 0x10000000;
}

void task_delay(uint32_t delay_ticks) {
    DISABLE_INTERRUPTS();
    if (current_task) {
        task_list[current_task].block_count = delay_ticks +
                system_tick_count;
        task_list[current_task].state = TASK_BLOCKED;
        schedule();
    }
    ENABLE_INTERRUPTS();
}

// -----------------------------
// Exception Handlers Xử lý ngoại lệ
// -----------------------------

void HardFault_Handler(void) {
    printf("Hard Fault occurred\n");
    while (1) ;
    }

void MemManage_Handler(void) {
    printf("Memory Management Fault occurred\n");
    while (1) ;
    }

void BusFault_Handler(void) {
    printf("Bus Fault occurred\n");
    while (1) ;
    }

void UsageFault_Handler(void) {
    printf("Usage Fault occurred\n");
    while (1) ;
    }

// -----------------------------
// PSP Management Quản lý PSP
// -----------------------------

uint32_t get_psp_value(void) {
    return task_list[current_task].stack_ptr;
}

void save_psp_value(uint32_t psp_val) {
    task_list[current_task].stack_ptr = psp_val;
}