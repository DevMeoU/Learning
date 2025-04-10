# Create Task và Stack Pointer


### 1. **Task 1 đang chạy**

* CPU đang thực thi  **Task 1** . Tất cả các thanh ghi cần thiết (R0–R3, R12, LR, PC, xPSR) có thể đã được lưu tự động khi ngắt xảy ra, nhưng các thanh ghi R4–R11 (vì là thanh ghi không được lưu tự động) cần phải được lưu thủ công để bảo toàn trạng thái của Task 1.

---

### 2. **Xảy ra ngắt Systick**

* Khi ngắt **SysTick** xảy ra, CPU dừng Task 1 và chuyển sang chế độ Handler. Trong handler, trước khi thực hiện các công việc khác, cần lưu lại trạng thái (context) của Task 1.
* **Bước lưu trạng thái của Task 1:**
  * **PUSH các thanh ghi R4–R11** : Dùng lệnh PUSH (hoặc STMDB trên ARM) để lưu các thanh ghi R4 đến R11 lên stack riêng của Task 1 (stack private của Task 1).
  * Sau khi PUSH, giá trị hiện tại của Process Stack Pointer (PSP) của Task 1 được lưu vào bộ nhớ (TCB – Task Control Block) của Task 1. Điều này cho phép lưu trữ vị trí chính xác của stack khi Task 1 bị ngắt.

---

### 3. **Lấy trạng thái của Task 2**

* Scheduler (trong hàm SysTick hoặc được kích hoạt bởi PendSV) quyết định chuyển sang  **Task 2** .
* Từ TCB của Task 2, hệ thống **POP** (nạp ngược) các thanh ghi R4–R11 từ stack private của Task 2. Thao tác POP này khôi phục lại trạng thái của Task 2.
* Giá trị PSP được lưu sẵn trong TCB của Task 2 được nạp vào thanh ghi PSP của CPU.

---

### 4. **Thoát khỏi Exception Handler và chạy Task 2**

* Sau khi khôi phục đầy đủ context của Task 2 (bao gồm các thanh ghi R4–R11 và giá trị PSP mới), CPU ra khỏi exception handler (thông qua lệnh BX LR trong PendSV Handler).
* CPU sau đó tiếp tục thực thi Task 2 từ địa chỉ được lưu trước đó, tức là Task 2 được “đánh thức” và chạy tiếp.

---

### Tóm tắt trình tự chuyển ngữ cảnh

1. **Task 1 đang chạy.**
2. Ngắt SysTick xảy ra → CPU chuyển sang chế độ Handler.
3. Trong handler:
   * **Lưu context của Task 1** : PUSH các thanh ghi R4–R11 lên stack private của Task 1 và lưu PSP của Task 1 vào TCB.
4. Scheduler quyết định chuyển sang Task 2:
   * **Khôi phục context của Task 2** : Lấy PSP đã lưu trong TCB của Task 2, POP các thanh ghi R4–R11 từ stack private của Task 2.
5. CPU thoát khỏi handler → Task 2 tiếp tục chạy.

---

Ví dụ code của PendSV Handler (đã được minh họa ở trên) sử dụng inline assembly để thực hiện các bước lưu/khôi phục:

<pre class="overflow-visible!" data-start="2413" data-end="3289"><div class="contain-inline-size rounded-md border-[0.5px] border-token-border-medium relative bg-token-sidebar-surface-primary"><div class="flex items-center text-token-text-secondary px-4 py-2 text-xs font-sans justify-between h-9 bg-token-sidebar-surface-primary dark:bg-token-main-surface-secondary select-none rounded-t-[5px]">c</div><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-sidebar-surface-primary text-token-text-secondary dark:bg-token-main-surface-secondary flex items-center rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"><button class="flex gap-1 items-center select-none px-4 py-1" aria-label="Copy"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path fill-rule="evenodd" clip-rule="evenodd" d="M7 5C7 3.34315 8.34315 2 10 2H19C20.6569 2 22 3.34315 22 5V14C22 15.6569 20.6569 17 19 17H17V19C17 20.6569 15.6569 22 14 22H5C3.34315 22 2 20.6569 2 19V10C2 8.34315 3.34315 7 5 7H7V5ZM9 7H14C15.6569 7 17 8.34315 17 10V15H19C19.5523 15 20 14.5523 20 14V5C20 4.44772 19.5523 4 19 4H10C9.44772 4 9 4.44772 9 5V7ZM5 9C4.44772 9 4 9.44772 4 10V19C4 19.5523 4.44772 20 5 20H14C14.5523 20 15 19.5523 15 19V10C15 9.44772 14.5523 9 14 9H5Z" fill="currentColor"></path></svg>Copy</button></span><span class="" data-state="closed"><button class="flex items-center gap-1 px-4 py-1 select-none"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path d="M2.5 5.5C4.3 5.2 5.2 4 5.5 2.5C5.8 4 6.7 5.2 8.5 5.5C6.7 5.8 5.8 7 5.5 8.5C5.2 7 4.3 5.8 2.5 5.5Z" fill="currentColor" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"></path><path d="M5.66282 16.5231L5.18413 19.3952C5.12203 19.7678 5.09098 19.9541 5.14876 20.0888C5.19933 20.2067 5.29328 20.3007 5.41118 20.3512C5.54589 20.409 5.73218 20.378 6.10476 20.3159L8.97693 19.8372C9.72813 19.712 10.1037 19.6494 10.4542 19.521C10.7652 19.407 11.0608 19.2549 11.3343 19.068C11.6425 18.8575 11.9118 18.5882 12.4503 18.0497L20 10.5C21.3807 9.11929 21.3807 6.88071 20 5.5C18.6193 4.11929 16.3807 4.11929 15 5.5L7.45026 13.0497C6.91175 13.5882 6.6425 13.8575 6.43197 14.1657C6.24513 14.4392 6.09299 14.7348 5.97903 15.0458C5.85062 15.3963 5.78802 15.7719 5.66282 16.5231Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path><path d="M14.5 7L18.5 11" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path></svg>Edit</button></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-c"><span><span>__attribute__((naked)) </span><span>void</span><span></span><span>PendSV_Handler</span><span>(void</span><span>) {
    __asm </span><span>volatile</span><span> (
        </span><span>"MRS   R0, PSP            \n"</span><span></span><span>// Lấy PSP hiện tại (Task 1) vào R0</span><span>
        </span><span>"STMDB R0!, {R4-R11}       \n"</span><span></span><span>// Lưu R4-R11 (context của Task 1) lên stack</span><span>
        </span><span>"LDR   R1, =currentTCB    \n"</span><span></span><span>// Địa chỉ của biến currentTCB</span><span>
        </span><span>"LDR   R1, [R1]           \n"</span><span>
        </span><span>"STR   R0, [R1]           \n"</span><span></span><span>// Lưu giá trị PSP của Task 1 vào TCB của Task 1</span><span>

        </span><span>"LDR   R1, =nextTCB       \n"</span><span></span><span>// Địa chỉ của biến nextTCB</span><span>
        </span><span>"LDR   R1, [R1]           \n"</span><span>
        </span><span>"LDR   R0, [R1]           \n"</span><span></span><span>// Lấy giá trị PSP của Task 2 từ TCB của Task 2</span><span>
        </span><span>"LDMIA R0!, {R4-R11}       \n"</span><span></span><span>// Khôi phục R4-R11 từ stack của Task 2</span><span>
        </span><span>"MSR   PSP, R0            \n"</span><span></span><span>// Cập nhật PSP của CPU với giá trị PSP của Task 2</span><span>
        </span><span>"BX    LR                 \n"</span><span></span><span>// Thoát khỏi Handler, trở lại Task 2</span><span>
    );
}
</span></span></code></div></div></pre>

Qua đó, bạn đã chuyển ngữ cảnh từ Task 1 sang Task 2 bằng cách lưu và khôi phục giá trị PSP của từng task thông qua các lệnh PUSH/POP trên stack riêng của mỗi task.



## 💡 Giả định ban đầu:

* CPU  **đang thực hiện Task 2 (T2)** .
* Stack của Task 2 dùng **Process Stack Pointer (PSP)** và đang trỏ tới **cuối RAM** (ví dụ `RAM_END`).
* Task 2 đã được cấu hình để chạy trong  **Thread Mode sử dụng PSP** .

---

## 🔁 Khi xảy ra một ngắt (ví dụ ngắt SysTick):

1. **CPU tự động lưu Stack Frame phần cứng (Hardware stack frame)** :

* Các thanh ghi: **R0, R1, R2, R3, R12, LR, PC, xPSR**
* Những thanh ghi này được CPU **tự động PUSH** vào stack của Task 2 (PSP), gọi là  **frame stack 1** .
* 👉 Đây là điều **bắt buộc** theo chuẩn ARM Cortex-M: khi exception xảy ra, CPU  **luôn tự động đẩy các thanh ghi này lên stack** .

1. **Vào Handler của SysTick** :

* CPU chuyển sang chế độ **Handler Mode** và sử dụng  **Main Stack Pointer (MSP)** .

1. **Trong Handler (ví dụ trong PendSV_Handler)** :

* Ta sẽ  **lưu thêm các thanh ghi còn lại** : **R4–R11**
* Lúc này, dùng lệnh như `STMDB R0!, {R4-R11}` để **đẩy tiếp các thanh ghi này vào PSP** của Task 2.
* 👉 Việc này tạo thành  **frame stack 2** , giúp ta lưu toàn bộ trạng thái Task 2.

1. **Lưu giá trị hiện tại của PSP (tức là stack pointer sau khi đã lưu đầy đủ)** :

* Giá trị PSP hiện tại được lưu vào biến toàn cục, ví dụ `tcb2.stackPointer`.
* 👉 Nhờ vậy, sau này khi muốn  **chuyển ngược lại Task 2** , chỉ cần:
  * Nạp lại giá trị PSP từ `tcb2.stackPointer`
  * POP trở lại các thanh ghi R4–R11
  * CPU tự động POP phần còn lại (R0–R3, R12, LR, PC, xPSR)

---

## ✅ Tóm lại:

### Đây chính là **lưu ngữ cảnh** (context saving) của Task 2:

| Thành phần được lưu | Ai lưu                                   | Vị trí                                         |
| ------------------------- | ----------------------------------------- | ------------------------------------------------ |
| R0–R3, R12, LR, PC, xPSR | **CPU**tự động                   | Stack của T2 (PSP)                              |
| R4–R11                   | **Lập trình viên**dùng assembly | Stack của T2 (PSP)                              |
| Giá trị PSP             | **Lập trình viên**               | Lưu vào biến toàn cục `tcb2.stackPointer` |
