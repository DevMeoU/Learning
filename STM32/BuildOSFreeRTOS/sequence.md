Để minh họa luồng hoạt động của đoạn mã trên, chúng ta sẽ xem xét quá trình khởi tạo và chuyển đổi giữa các tác vụ trong hệ thống. Dưới đây là sơ đồ mô tả luồng hoạt động:

<pre class="overflow-visible!" data-start="173" data-end="956"><div class="contain-inline-size rounded-md border-[0.5px] border-token-border-medium relative bg-token-sidebar-surface-primary"><div class="flex items-center text-token-text-secondary px-4 py-2 text-xs font-sans justify-between h-9 bg-token-sidebar-surface-primary dark:bg-token-main-surface-secondary select-none rounded-t-[5px]">mermaid</div><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-sidebar-surface-primary text-token-text-secondary dark:bg-token-main-surface-secondary flex items-center rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"><button class="flex gap-1 items-center select-none px-4 py-1" aria-label="Sao chép"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path fill-rule="evenodd" clip-rule="evenodd" d="M7 5C7 3.34315 8.34315 2 10 2H19C20.6569 2 22 3.34315 22 5V14C22 15.6569 20.6569 17 19 17H17V19C17 20.6569 15.6569 22 14 22H5C3.34315 22 2 20.6569 2 19V10C2 8.34315 3.34315 7 5 7H7V5ZM9 7H14C15.6569 7 17 8.34315 17 10V15H19C19.5523 15 20 14.5523 20 14V5C20 4.44772 19.5523 4 19 4H10C9.44772 4 9 4.44772 9 5V7ZM5 9C4.44772 9 4 9.44772 4 10V19C4 19.5523 4.44772 20 5 20H14C14.5523 20 15 19.5523 15 19V10C15 9.44772 14.5523 9 14 9H5Z" fill="currentColor"></path></svg>Sao chép</button></span><span class="" data-state="closed"><button class="flex items-center gap-1 px-4 py-1 select-none"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path d="M2.5 5.5C4.3 5.2 5.2 4 5.5 2.5C5.8 4 6.7 5.2 8.5 5.5C6.7 5.8 5.8 7 5.5 8.5C5.2 7 4.3 5.8 2.5 5.5Z" fill="currentColor" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"></path><path d="M5.66282 16.5231L5.18413 19.3952C5.12203 19.7678 5.09098 19.9541 5.14876 20.0888C5.19933 20.2067 5.29328 20.3007 5.41118 20.3512C5.54589 20.409 5.73218 20.378 6.10476 20.3159L8.97693 19.8372C9.72813 19.712 10.1037 19.6494 10.4542 19.521C10.7652 19.407 11.0608 19.2549 11.3343 19.068C11.6425 18.8575 11.9118 18.5882 12.4503 18.0497L20 10.5C21.3807 9.11929 21.3807 6.88071 20 5.5C18.6193 4.11929 16.3807 4.11929 15 5.5L7.45026 13.0497C6.91175 13.5882 6.6425 13.8575 6.43197 14.1657C6.24513 14.4392 6.09299 14.7348 5.97903 15.0458C5.85062 15.3963 5.78802 15.7719 5.66282 16.5231Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path><path d="M14.5 7L18.5 11" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path></svg>Chỉnh sửa</button></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-mermaid"><span>sequenceDiagram
    participant Main
    participant Scheduler
    participant Task1
    participant SysTick_Handler

    Main->>Main: enable_rprocessor_exceptions()
    Main->>Scheduler: init_scheduler_task(SCHED_STACK_START)
    Main->>Main: Gán task_handlers cho các hàm task
    Main->>Main: init_task_stack()
    Main->>Main: init_systick_timer(TICK_HZ)
    Main->>Main: switch_sp_to_psp()
    Main->>Task1: task1_handle()
    loop Liên tục
        Task1->>Task1: Thực thi nhiệm vụ
        SysTick_Handler->>Task1: Ngắt SysTick xảy ra
        SysTick_Handler->>SysTick_Handler: Lưu trạng thái Task1
        SysTick_Handler->>Scheduler: update_next_task()
        SysTick_Handler->>Task2: Khôi phục trạng thái Task2
        Task2->>Task2: Thực thi nhiệm vụ
    end
</span></code></div></div></pre>

**Giải thích luồng hoạt động:**

1. **Khởi tạo hệ thống:**
   * `enable_rprocessor_exceptions()`: **Kích hoạt các ngoại lệ của bộ xử lý như Usage Fault, Bus Fault, và Memory Management Fault.**
   * `init_scheduler_task(SCHED_STACK_START)`: **Thiết lập con trỏ stack chính (MSP) cho bộ lập lịch tại địa chỉ **`SCHED_STACK_START`.
   * **Gán các hàm xử lý nhiệm vụ (**`task1_handle`, `task2_handle`, ...) vào mảng `task_handlers`.
   * `init_task_stack()`: **Khởi tạo stack cho từng tác vụ với trạng thái ban đầu.**
   * `init_systick_timer(TICK_HZ)`: **Cấu hình bộ định thời SysTick để tạo ngắt mỗi mili giây, phục vụ cho việc chuyển đổi tác vụ định kỳ.**
2. **Chuyển đổi sang sử dụng Process Stack Pointer (PSP):**
   * `switch_sp_to_psp()`: **Chuyển đổi từ sử dụng MSP sang PSP bằng cách:**
     * Lưu giá trị của thanh ghi liên kết (LR).
     * Gọi `get_psp()` để lấy địa chỉ stack của tác vụ hiện tại.
     * Thiết lập PSP với giá trị vừa lấy được.
     * Khôi phục giá trị của LR.
     * Thiết lập thanh ghi CONTROL để sử dụng PSP trong chế độ Thread.
3. **Bắt đầu thực thi tác vụ đầu tiên:**
   * **Gọi **`task1_handle()`, bắt đầu vòng lặp vô hạn thực thi nhiệm vụ của Task 1.
4. **Chuyển đổi tác vụ khi ngắt SysTick xảy ra:**
   * **Mỗi mili giây, ngắt SysTick xảy ra:**
     * Lưu trạng thái hiện tại của tác vụ đang chạy (Task 1) vào stack.
     * Gọi `save_psp_value()` để lưu giá trị PSP hiện tại.
     * Gọi `update_next_task()` để xác định tác vụ tiếp theo sẽ chạy.
     * Gọi `get_psp()` để lấy địa chỉ stack của tác vụ tiếp theo.
     * Khôi phục trạng thái của tác vụ tiếp theo từ stack.
     * Tiếp tục thực thi tác vụ mới.

**Lưu ý:** **Quá trình chuyển đổi giữa các tác vụ được thực hiện thông qua ngắt SysTick và việc quản lý stack pointer (PSP) cho từng tác vụ, đảm bảo rằng mỗi tác vụ có không gian stack riêng biệt và trạng thái được bảo toàn khi chuyển đổi.**
