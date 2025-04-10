#include <stdio.h>
#include <stdint.h>

// -----------------------------
// Memory and Stack Definitions
// -----------------------------
#define SIZE_TASK_STACK         (5*1024)  // Stack size for each task
#define SIZE_SCHED_STACK        (5*1024)  // Stack size for scheduler

#define SRAM_START              (0x20000000U)  // Start address of SRAM
#define SIZE_SRAM               (64*1024)      // 64kB SRAM
#define SRAM_END                (SRAM_START + SIZE_SRAM)

#define T1_STACK_START          (SRAM_END)
#define T2_STACK_START          (SRAM_END - (1*SIZE_TASK_STACK))
#define T3_STACK_START          (SRAM_END - (2*SIZE_TASK_STACK))
#define T4_STACK_START          (SRAM_END - (3*SIZE_TASK_STACK))
#define IDLE_STACK_START        (SRAM_END - (4*SIZE_TASK_STACK))
#define SCHED_STACK_START       (SRAM_END - (5*SIZE_TASK_STACK))

// -----------------------------
// System Timer Configuration
// -----------------------------
#define TICK_HZ                 (1000)        // 1ms tick
#define HSI_FREQ                (16000000)    // 16MHz HSI
#define SYSTICK_TIMER_CLK       (HSI_FREQ)

// -----------------------------
// Task and Scheduler Definitions
// -----------------------------
#define MAX_TASKS               (5)           // Number of tasks

#define INTERRUPT_DISABLE()    do{__asm volatile("MOV R0, #0x01"); __asm volatile("MSR PRIMASK, R0");}while(0) // Disable interrupts
#define INTERRUPT_ENABLE()     do{__asm volatile("MOV R0, #0x00"); __asm volatile("MSR PRIMASK, R0");}while(0) // Enable interrupts

// Task stack pointers and handlers
uint32_t current_task = 1; // Current task index
uint32_t g_tick_count = 0; // Global tick count

// -----------------------------
// Register Definitions
// -----------------------------
#define RCC_BASE                (0x40021000U) // Correct base address of RCC for STM32L433
#define GPIOA_BASE              (0x40020000U) // Base address of GPIOA (unchanged)

#define RCC_AHB1ENR             (*(volatile uint32_t *)(RCC_BASE + 0x48U)) // RCC AHB1ENR for STM32L433
#define GPIOA_MODER             (*(volatile uint32_t *)(GPIOA_BASE + 0x00U)) // GPIO port mode register
#define GPIOA_OTYPER            (*(volatile uint32_t *)(GPIOA_BASE + 0x04U)) // GPIO port output type register
#define GPIOA_OSPEEDR           (*(volatile uint32_t *)(GPIOA_BASE + 0x08U)) // GPIO port output speed register
#define GPIOA_PUPDR             (*(volatile uint32_t *)(GPIOA_BASE + 0x0CU)) // GPIO port pull-up/pull-down register
#define GPIOA_ODR               (*(volatile uint32_t *)(GPIOA_BASE + 0x14U)) // GPIO port output data register

#define TASK_RUNNING_STATE      (0x00) // Task running state
#define TASK_BLOCKED_STATE      (0x01) // Task blocked state

// LPUART1 Register Definitions
#define LPUART1_BASE            (0x40008000U) // Base address of LPUART1
#define RCC_APB1ENR2            (*(volatile uint32_t *)(RCC_BASE + 0x5CU)) // RCC APB1 peripheral clock enable register 2

#define LPUART1_CR1             (*(volatile uint32_t *)(LPUART1_BASE + 0x00U)) // Control register 1
#define LPUART1_BRR             (*(volatile uint32_t *)(LPUART1_BASE + 0x0CU)) // Baud rate register
#define LPUART1_ISR             (*(volatile uint32_t *)(LPUART1_BASE + 0x1CU)) // Interrupt and status register
#define LPUART1_TDR             (*(volatile uint32_t *)(LPUART1_BASE + 0x28U)) // Transmit data register

// -----------------------------
// Function Prototypes
// -----------------------------
void task1_handle(void);
void task2_handle(void);
void task3_handle(void);
void task4_handle(void);
void idle_task(void); // Idle task (optional)
void init_systick_timer(uint32_t tick_hz);
__attribute__((naked)) void init_scheduler_task(uint32_t sched_top_of_stack);
void init_task_stack(void);
void enable_rprocessor_exceptions(void);
uint32_t get_psp(void);
void save_psp_value(uint32_t psp_value);
__attribute__((naked)) void switch_sp_to_psp(void);
void update_next_task(void);
void init_led_gpio(void); // Initialize GPIO for LED
void task_delay(uint32_t delay); // Task delay function (optional)
void init_lpuart1(void); // Initialize LPUART1
void lpuart1_send_char(char c); // Send a character via LPUART1
void lpuart1_send_string(const char *str); // Send a string via LPUART1

typedef struct
{
    uint32_t psp_value; // Process Stack Pointer value
    uint32_t block_count; // Block count for the task
    uint8_t current_state; // Current state of the task
    void (*task_handler)(void); // Task handler function pointer
} TCB_t; // Task Control Block structure

TCB_t user_task[MAX_TASKS]; // Array of Task Control Blocks

// -----------------------------
// Main Function
// -----------------------------
int main(void)
{
    enable_rprocessor_exceptions(); // Enable processor exceptions (if needed)
    init_scheduler_task(SCHED_STACK_START); // Initialize scheduler task stack pointer

    init_task_stack(); // Initialize task stacks

    // Initialize SysTick timer for 1ms tick
    init_systick_timer(TICK_HZ);

    // Initialize GPIO for LED
    init_led_gpio();

    // Initialize LPUART1
    init_lpuart1();

    // Switch to PSP (Process Stack Pointer) mode
    switch_sp_to_psp();

    // Start the first task
    task1_handle();

    // Infinite loop
    for (;;);
}

// -----------------------------
// LED GPIO Initialization
// -----------------------------
// Configure PA5 as output for LED
void init_led_gpio(void)
{
    // Enable GPIOA clock
    RCC_AHB1ENR |= (1U << 0); // Set bit 0 to enable GPIOA clock

    // Set PA5 as output mode
    GPIOA_MODER &= ~(3U << (5 * 2)); // Clear mode bits for PA5
    GPIOA_MODER |= (1U << (5 * 2));  // Set PA5 to output mode

    // Set PA5 output type to push-pull
    GPIOA_OTYPER &= ~(1U << 5);

    // Set PA5 speed to high
    GPIOA_OSPEEDR |= (3U << (5 * 2));

    // Disable pull-up/pull-down for PA5
    GPIOA_PUPDR &= ~(3U << (5 * 2));
}

// -----------------------------
// Task Handlers
// -----------------------------
// Task handlers are functions that will be executed by the scheduler
// when the corresponding task is scheduled to run. Each task has its own
// handler function.
void idle_task(void)
{
    while (1)
    {
        // Idle task does nothing, just waits for SysTick interrupt
    }
}

// Task 1: Blink LED
void task1_handle(void)
{
    while (1)
    {
        lpuart1_send_string("Task 1 is running\n"); // Send message via LPUART1
        // Turn on LED
        GPIOA_ODR |= (1U << 5);

        task_delay(1000); // Delay for 500ms

        // Turn off LED
        GPIOA_ODR &= ~(1U << 5);

        // Delay for 500ms
        task_delay(1000); // Delay for 500ms
    }
}

void task2_handle(void)
{
    while (1)
    {
        lpuart1_send_string("Task 2 is running\n"); // Send message via LPUART1
        task_delay(500); // Delay for 500ms
    }
}

void task3_handle(void)
{
    while (1)
    {
        lpuart1_send_string("Task 3 is running\n"); // Send message via LPUART1
        printf("Task 3\n");
        task_delay(250); // Delay for 200ms
    }
}

void task4_handle(void)
{
    while (1)
    {
        lpuart1_send_string("Task 4 is running\n"); // Send message via LPUART1
        printf("Task 4\n");
        task_delay(2000); // Delay for 200ms
    }
}

// -----------------------------
// SysTick Timer Initialization
// -----------------------------
void init_systick_timer(uint32_t tick_hz)
{
    uint32_t *pSRVR = (uint32_t *)0xE000E014; // SysTick Reload Value Register
    uint32_t *pSCSR = (uint32_t *)0xE000E010; // SysTick Control and Status Register

    // Configure SysTick timer for 1ms tick
    uint32_t reload_value = (SYSTICK_TIMER_CLK / tick_hz) - 1;
    *pSCSR &= ~0x00FFFFFF; // Clear current value register
    *pSRVR = reload_value; // Set reload value
    *pSCSR = 0x00000007;   // Enable SysTick timer, use processor clock, enable interrupt

    // Enable SysTick interrupt in NVIC
    uint32_t *pNVIC_ISER = (uint32_t *)0xE000E100; // NVIC Interrupt Set-Enable Register
    *pNVIC_ISER |= (1 << 15); // Enable SysTick interrupt (interrupt number 15)
}

// -----------------------------
// Scheduler Initialization
// -----------------------------
__attribute__((naked)) void init_scheduler_task(uint32_t sched_top_of_stack)
{
    __asm volatile("MSR MSP, %0" : : "r" (sched_top_of_stack)); // Set MSP to scheduler stack
    __asm volatile("BX LR"); // Return
}

// -----------------------------
// Task Stack Initialization
// -----------------------------
void init_task_stack(void)
{
    user_task[0].current_state = TASK_RUNNING_STATE; // Set task 1 state to running
    user_task[1].current_state = TASK_BLOCKED_STATE; // Set task 2 state to blocked
    user_task[2].current_state = TASK_BLOCKED_STATE; // Set task 3 state to blocked
    user_task[3].current_state = TASK_BLOCKED_STATE; // Set task 4 state to blocked
    user_task[4].current_state = TASK_BLOCKED_STATE; // Set idle task state to blocked

    user_task[0].psp_value = IDLE_STACK_START; // Set task 1 stack pointer
    user_task[1].psp_value = T1_STACK_START; // Set task 2 stack pointer
    user_task[2].psp_value = T2_STACK_START; // Set task 3 stack pointer
    user_task[3].psp_value = T3_STACK_START; // Set task 4 stack pointer
    user_task[4].psp_value = T4_STACK_START; // Set idle task stack pointer

    user_task[0].task_handler = idle_task; // Set idle task handler
    user_task[1].task_handler = task1_handle; // Set task 1 handler
    user_task[2].task_handler = task2_handle; // Set task 2 handler
    user_task[3].task_handler = task3_handle; // Set task 3 handler
    user_task[4].task_handler = task4_handle; // Set task 4 handler

    uint32_t *pPSP;
    for (int i = 0; i < MAX_TASKS; i++)
    {
        pPSP = (uint32_t *)user_task[i].psp_value; // Get stack pointer for each task
        *(--pPSP) = 0x01000000;            // xPSR: Thumb bit set
        *(--pPSP) = (uint32_t)user_task[i].task_handler; // PC: Task handler address
        *(--pPSP) = 0xFFFFFFFD;            // LR: Return to thread mode with PSP
        for (int j = 0; j < 13; j++)
        {
            *(--pPSP) = 0; // Initialize registers R0-R12 to 0
        }
        user_task[i].psp_value = (uint32_t)pPSP; // Update task stack pointer
    }
}

// -----------------------------
// Exception Handlers
// -----------------------------
void enable_rprocessor_exceptions(void)
{
    uint32_t *pSHCSR = (uint32_t *)0xE000ED24; // System Handler Control and State Register
    *pSHCSR |= (1 << 16) | (1 << 17) | (1 << 18); // Enable usage fault, bus fault, memory management fault
}

void HardFault_Handler(void)
{
    printf("Hard Fault occurred\n");
    while (1);
}

void MemManage_Handler(void)
{
    printf("Memory Management Fault occurred\n");
    while (1);
}

void BusFault_Handler(void)
{
    printf("Bus Fault occurred\n");
    while (1);
}

void UsageFault_Handler(void)
{
    printf("Usage Fault occurred\n");
    while (1);
}

// -----------------------------
// PSP Management
// -----------------------------
uint32_t get_psp(void)
{
    return user_task[current_task].psp_value; // Return PSP value for current task
}

void save_psp_value(uint32_t psp_value)
{
    user_task[current_task].psp_value = psp_value; // Save PSP value for current task
}

void update_next_task(void)
{
    int state = TASK_BLOCKED_STATE; // Initialize state to blocked
    for (int i = 0; i < MAX_TASKS; i++)
    {
        current_task++; // Increment task index
        current_task %= MAX_TASKS; // Wrap around if it exceeds the number of tasks
        state = user_task[current_task].current_state; // Get the state of the next task
        if ((state == TASK_RUNNING_STATE) && (current_task != 0)) // Check if the next task is running
        {
            break; // Exit loop if a running task is found
        }
    }
    if (state == TASK_BLOCKED_STATE) // If all tasks are blocked
    {
        current_task = 0; // Set current task to idle task
    }
}

// -----------------------------
// Stack Pointer Switching
// -----------------------------
__attribute__((naked)) void switch_sp_to_psp(void)
{
    __asm volatile("PUSH {LR}"); // Save LR
    __asm volatile("BL get_psp"); // Get PSP for current task
    __asm volatile("MSR PSP, R0"); // Set PSP
    __asm volatile("POP {LR}"); // Restore LR
    __asm volatile("MOV R0, #0x02"); // Set CONTROL register to use PSP
    __asm volatile("MSR CONTROL, R0");
    __asm volatile("BX LR"); // Return
}

__attribute__((naked)) void PendSV_Handler(void)
{
    // Save the current task's Process Stack Pointer (PSP)
    // Lưu giá trị PSP (Process Stack Pointer) của task hiện tại
    __asm volatile("MRS R0, PSP"); // Move PSP to R0 (current task's stack pointer)

    // Save the context of the current task (R4-R11 registers)
    // Lưu trạng thái của task hiện tại (các thanh ghi R4-R11)
    __asm volatile("STMDB R0!, {R4-R11}"); // Store multiple registers (R4-R11) and decrement PSP

    // Save the Link Register (LR) to the stack
    // Lưu giá trị của thanh ghi LR vào stack
    __asm volatile("PUSH {LR}"); // Push LR onto the stack

    // Call save_psp_value to save the updated PSP value for the current task
    // Gọi hàm save_psp_value để lưu giá trị PSP đã cập nhật của task hiện tại
    __asm volatile("BL save_psp_value"); // Branch to save_psp_value

    // Call update_next_task to determine the next task to run
    // Gọi hàm update_next_task để xác định task tiếp theo cần chạy
    __asm volatile("BL update_next_task"); // Branch to update_next_task

    // Get the PSP value for the next task
    // Lấy giá trị PSP của task tiếp theo
    __asm volatile("BL get_psp"); // Branch to get_psp (returns PSP of the next task)

    // Restore the context of the next task (R4-R11 registers)
    // Khôi phục trạng thái của task tiếp theo (các thanh ghi R4-R11)
    __asm volatile("LDMIA R0!, {R4-R11}"); // Load multiple registers (R4-R11) and increment PSP

    // Set the PSP to the value of the next task
    // Thiết lập PSP với giá trị của task tiếp theo
    __asm volatile("MSR PSP, R0"); // Move R0 (next task's PSP) to PSP

    // Restore the Link Register (LR) from the stack
    // Khôi phục giá trị của thanh ghi LR từ stack
    __asm volatile("POP {LR}"); // Pop LR from the stack

    // Return to the next task
    // Trả quyền điều khiển cho task tiếp theo
    __asm volatile("BX LR"); // Branch to the address in LR
}


void update_global_tick_count(void) {
    g_tick_count++; // Increment global tick count
}

void unblock_tasks(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (user_task[i].current_state == TASK_BLOCKED_STATE) {
            if (user_task[i].block_count <= g_tick_count) {
                user_task[i].current_state = TASK_RUNNING_STATE; // Unblock the task
            }
        }
    }
}

// -----------------------------
// SysTick Handler
// -----------------------------
// This function is the SysTick interrupt handler. It is responsible for saving
// the current task's context, switching to the next task, and restoring the
// context of the next task.
// Hàm này là trình xử lý ngắt SysTick. Nó chịu trách nhiệm lưu trạng thái
// của task hiện tại, chuyển sang task tiếp theo và khôi phục trạng thái
// của task tiếp theo.
void SysTick_Handler(void)
{
    uint32_t *pICSR = (uint32_t *)0xE000ED04; // Address of the PendSV register
    
    update_global_tick_count(); // Update global tick count

    unblock_tasks(); // Unblock tasks if needed

    *pICSR = 0x10000000; // Trigger PendSV interrupt (set bit 28)
}

void schedule(void)
{
    uint32_t *pICSR = (uint32_t *)0xE000ED04; // Address of the PendSV register
    *pICSR = 0x10000000; // Trigger PendSV interrupt (set bit 28)
}

void task_delay(uint32_t delay)
{
    INTERRUPT_DISABLE(); // Disable interrupts
    if(current_task)
    {
        user_task[current_task].block_count = delay + g_tick_count; // Set block count for the current task    
        user_task[current_task].current_state = TASK_BLOCKED_STATE; // Set task state to blocked
        schedule(); // Call scheduler to switch tasks
    }
    INTERRUPT_ENABLE(); // Enable interrupts
}

void init_lpuart1(void)
{
    // Enable clock for LPUART1 and GPIOA
    RCC_APB1ENR2 |= (1U << 0); // Enable LPUART1 clock
    RCC_AHB1ENR |= (1U << 0);  // Enable GPIOA clock

    // Configure PA2 (TX) as alternate function mode
    GPIOA_MODER &= ~(3U << (2 * 2)); // Clear mode bits for PA2
    GPIOA_MODER |= (2U << (2 * 2));  // Set PA2 to alternate function mode

    // Set PA2 alternate function to AF8 (LPUART1_TX)
    uint32_t *GPIOA_AFRL = (uint32_t *)(GPIOA_BASE + 0x20U); // GPIO alternate function low register
    *GPIOA_AFRL &= ~(0xF << (4 * 2)); // Clear alternate function bits for PA2
    *GPIOA_AFRL |= (8U << (4 * 2));   // Set alternate function to AF8

    // Configure LPUART1
    LPUART1_CR1 = 0;                  // Disable LPUART1 and reset settings
    LPUART1_BRR = (HSI_FREQ / 9600);  // Set baud rate to 9600 (assuming HSI clock)
    LPUART1_CR1 |= (1U << 3);         // Enable transmitter
    LPUART1_CR1 |= (1U << 0);         // Enable LPUART1
}

void lpuart1_send_char(char c)
{
    while (!(LPUART1_ISR & (1U << 7))); // Wait until TXE (Transmit Data Register Empty) is set
    LPUART1_TDR = c;                   // Write data to transmit data register
}

void lpuart1_send_string(const char *str)
{
    while (*str)
    {
        lpuart1_send_char(*str++); // Send each character in the string
    }
}