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



### Cách **truy xuất ngữ cảnh** (Context Restore) trong hệ thống nhúng sử dụng ARM Cortex-M (ví dụ STM32):

---

## 🎯 Mục tiêu của truy xuất ngữ cảnh:

Khôi phục **trạng thái của một task** (ví dụ Task 1) từ nơi nó đã bị dừng lại trước đó → để task có thể  **tiếp tục thực hiện như chưa từng bị gián đoạn** .

---

## 🧠 Các bước phục hồi ngữ cảnh:

### 🧩 1. **Tải lại giá trị PSP của task cần phục hồi**

* Mỗi task có một vùng stack riêng, con trỏ PSP của task đã được lưu trước đó trong biến `tcb->stackPointer`.

<pre class="overflow-visible!" data-start="516" data-end="694"><div class="contain-inline-size rounded-md border-[0.5px] border-token-border-medium relative bg-token-sidebar-surface-primary"><div class="flex items-center text-token-text-secondary px-4 py-2 text-xs font-sans justify-between h-9 bg-token-sidebar-surface-primary dark:bg-token-main-surface-secondary select-none rounded-t-[5px]">c</div><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-sidebar-surface-primary text-token-text-secondary dark:bg-token-main-surface-secondary flex items-center rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"><button class="flex gap-1 items-center select-none px-4 py-1" aria-label="Copy"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path fill-rule="evenodd" clip-rule="evenodd" d="M7 5C7 3.34315 8.34315 2 10 2H19C20.6569 2 22 3.34315 22 5V14C22 15.6569 20.6569 17 19 17H17V19C17 20.6569 15.6569 22 14 22H5C3.34315 22 2 20.6569 2 19V10C2 8.34315 3.34315 7 5 7H7V5ZM9 7H14C15.6569 7 17 8.34315 17 10V15H19C19.5523 15 20 14.5523 20 14V5C20 4.44772 19.5523 4 19 4H10C9.44772 4 9 4.44772 9 5V7ZM5 9C4.44772 9 4 9.44772 4 10V19C4 19.5523 4.44772 20 5 20H14C14.5523 20 15 19.5523 15 19V10C15 9.44772 14.5523 9 14 9H5Z" fill="currentColor"></path></svg>Copy</button></span><span class="" data-state="closed"><button class="flex items-center gap-1 px-4 py-1 select-none"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path d="M2.5 5.5C4.3 5.2 5.2 4 5.5 2.5C5.8 4 6.7 5.2 8.5 5.5C6.7 5.8 5.8 7 5.5 8.5C5.2 7 4.3 5.8 2.5 5.5Z" fill="currentColor" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"></path><path d="M5.66282 16.5231L5.18413 19.3952C5.12203 19.7678 5.09098 19.9541 5.14876 20.0888C5.19933 20.2067 5.29328 20.3007 5.41118 20.3512C5.54589 20.409 5.73218 20.378 6.10476 20.3159L8.97693 19.8372C9.72813 19.712 10.1037 19.6494 10.4542 19.521C10.7652 19.407 11.0608 19.2549 11.3343 19.068C11.6425 18.8575 11.9118 18.5882 12.4503 18.0497L20 10.5C21.3807 9.11929 21.3807 6.88071 20 5.5C18.6193 4.11929 16.3807 4.11929 15 5.5L7.45026 13.0497C6.91175 13.5882 6.6425 13.8575 6.43197 14.1657C6.24513 14.4392 6.09299 14.7348 5.97903 15.0458C5.85062 15.3963 5.78802 15.7719 5.66282 16.5231Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path><path d="M14.5 7L18.5 11" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path></svg>Edit</button></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-c"><span><span>LDR R0, =nextTCB     </span><span>// Địa chỉ của task tiếp theo</span><span>
LDR R0, [R0]         </span><span>// Lấy địa chỉ PSP từ tcb->stackPointer</span><span>
MSR PSP, R0          </span><span>// Gán giá trị PSP vào thanh ghi PSP</span><span>
</span></span></code></div></div></pre>

---

### 🧩 2. **Phục hồi các thanh ghi R4–R11 từ stack của task**

* Những thanh ghi này  **không được CPU tự động phục hồi** , nên ta cần  **POP thủ công** .

<pre class="overflow-visible!" data-start="851" data-end="933"><div class="contain-inline-size rounded-md border-[0.5px] border-token-border-medium relative bg-token-sidebar-surface-primary"><div class="flex items-center text-token-text-secondary px-4 py-2 text-xs font-sans justify-between h-9 bg-token-sidebar-surface-primary dark:bg-token-main-surface-secondary select-none rounded-t-[5px]">c</div><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-sidebar-surface-primary text-token-text-secondary dark:bg-token-main-surface-secondary flex items-center rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"><button class="flex gap-1 items-center select-none px-4 py-1" aria-label="Copy"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path fill-rule="evenodd" clip-rule="evenodd" d="M7 5C7 3.34315 8.34315 2 10 2H19C20.6569 2 22 3.34315 22 5V14C22 15.6569 20.6569 17 19 17H17V19C17 20.6569 15.6569 22 14 22H5C3.34315 22 2 20.6569 2 19V10C2 8.34315 3.34315 7 5 7H7V5ZM9 7H14C15.6569 7 17 8.34315 17 10V15H19C19.5523 15 20 14.5523 20 14V5C20 4.44772 19.5523 4 19 4H10C9.44772 4 9 4.44772 9 5V7ZM5 9C4.44772 9 4 9.44772 4 10V19C4 19.5523 4.44772 20 5 20H14C14.5523 20 15 19.5523 15 19V10C15 9.44772 14.5523 9 14 9H5Z" fill="currentColor"></path></svg>Copy</button></span><span class="" data-state="closed"><button class="flex items-center gap-1 px-4 py-1 select-none"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path d="M2.5 5.5C4.3 5.2 5.2 4 5.5 2.5C5.8 4 6.7 5.2 8.5 5.5C6.7 5.8 5.8 7 5.5 8.5C5.2 7 4.3 5.8 2.5 5.5Z" fill="currentColor" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"></path><path d="M5.66282 16.5231L5.18413 19.3952C5.12203 19.7678 5.09098 19.9541 5.14876 20.0888C5.19933 20.2067 5.29328 20.3007 5.41118 20.3512C5.54589 20.409 5.73218 20.378 6.10476 20.3159L8.97693 19.8372C9.72813 19.712 10.1037 19.6494 10.4542 19.521C10.7652 19.407 11.0608 19.2549 11.3343 19.068C11.6425 18.8575 11.9118 18.5882 12.4503 18.0497L20 10.5C21.3807 9.11929 21.3807 6.88071 20 5.5C18.6193 4.11929 16.3807 4.11929 15 5.5L7.45026 13.0497C6.91175 13.5882 6.6425 13.8575 6.43197 14.1657C6.24513 14.4392 6.09299 14.7348 5.97903 15.0458C5.85062 15.3963 5.78802 15.7719 5.66282 16.5231Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path><path d="M14.5 7L18.5 11" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path></svg>Edit</button></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-c"><span><span>LDMIA R0!, {R4-R11}  </span><span>// POP R4–R11 từ stack (sau đó PSP đã được cập nhật)</span><span>
</span></span></code></div></div></pre>

---

### 🧩 3. **Thoát khỏi exception handler**

* Khi `BX LR` được gọi trong `PendSV_Handler`, CPU sẽ **tự động phục hồi các thanh ghi còn lại** (R0–R3, R12, LR, PC, xPSR) từ stack của task (PSP).

<pre class="overflow-visible!" data-start="1132" data-end="1191"><div class="contain-inline-size rounded-md border-[0.5px] border-token-border-medium relative bg-token-sidebar-surface-primary"><div class="flex items-center text-token-text-secondary px-4 py-2 text-xs font-sans justify-between h-9 bg-token-sidebar-surface-primary dark:bg-token-main-surface-secondary select-none rounded-t-[5px]">asm</div><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-sidebar-surface-primary text-token-text-secondary dark:bg-token-main-surface-secondary flex items-center rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"><button class="flex gap-1 items-center select-none px-4 py-1" aria-label="Copy"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path fill-rule="evenodd" clip-rule="evenodd" d="M7 5C7 3.34315 8.34315 2 10 2H19C20.6569 2 22 3.34315 22 5V14C22 15.6569 20.6569 17 19 17H17V19C17 20.6569 15.6569 22 14 22H5C3.34315 22 2 20.6569 2 19V10C2 8.34315 3.34315 7 5 7H7V5ZM9 7H14C15.6569 7 17 8.34315 17 10V15H19C19.5523 15 20 14.5523 20 14V5C20 4.44772 19.5523 4 19 4H10C9.44772 4 9 4.44772 9 5V7ZM5 9C4.44772 9 4 9.44772 4 10V19C4 19.5523 4.44772 20 5 20H14C14.5523 20 15 19.5523 15 19V10C15 9.44772 14.5523 9 14 9H5Z" fill="currentColor"></path></svg>Copy</button></span><span class="" data-state="closed"><button class="flex items-center gap-1 px-4 py-1 select-none"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path d="M2.5 5.5C4.3 5.2 5.2 4 5.5 2.5C5.8 4 6.7 5.2 8.5 5.5C6.7 5.8 5.8 7 5.5 8.5C5.2 7 4.3 5.8 2.5 5.5Z" fill="currentColor" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"></path><path d="M5.66282 16.5231L5.18413 19.3952C5.12203 19.7678 5.09098 19.9541 5.14876 20.0888C5.19933 20.2067 5.29328 20.3007 5.41118 20.3512C5.54589 20.409 5.73218 20.378 6.10476 20.3159L8.97693 19.8372C9.72813 19.712 10.1037 19.6494 10.4542 19.521C10.7652 19.407 11.0608 19.2549 11.3343 19.068C11.6425 18.8575 11.9118 18.5882 12.4503 18.0497L20 10.5C21.3807 9.11929 21.3807 6.88071 20 5.5C18.6193 4.11929 16.3807 4.11929 15 5.5L7.45026 13.0497C6.91175 13.5882 6.6425 13.8575 6.43197 14.1657C6.24513 14.4392 6.09299 14.7348 5.97903 15.0458C5.85062 15.3963 5.78802 15.7719 5.66282 16.5231Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path><path d="M14.5 7L18.5 11" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path></svg>Edit</button></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-asm"><span>BX LR   // Quay lại thread mode và tiếp tục task
</span></code></div></div></pre>

---

## 🔁 Tổng quan sơ đồ quá trình khôi phục ngữ cảnh:

<pre class="overflow-visible!" data-start="1251" data-end="1641"><div class="contain-inline-size rounded-md border-[0.5px] border-token-border-medium relative bg-token-sidebar-surface-primary"><div class="flex items-center text-token-text-secondary px-4 py-2 text-xs font-sans justify-between h-9 bg-token-sidebar-surface-primary dark:bg-token-main-surface-secondary select-none rounded-t-[5px]">r</div><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-sidebar-surface-primary text-token-text-secondary dark:bg-token-main-surface-secondary flex items-center rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"><button class="flex gap-1 items-center select-none px-4 py-1" aria-label="Copy"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path fill-rule="evenodd" clip-rule="evenodd" d="M7 5C7 3.34315 8.34315 2 10 2H19C20.6569 2 22 3.34315 22 5V14C22 15.6569 20.6569 17 19 17H17V19C17 20.6569 15.6569 22 14 22H5C3.34315 22 2 20.6569 2 19V10C2 8.34315 3.34315 7 5 7H7V5ZM9 7H14C15.6569 7 17 8.34315 17 10V15H19C19.5523 15 20 14.5523 20 14V5C20 4.44772 19.5523 4 19 4H10C9.44772 4 9 4.44772 9 5V7ZM5 9C4.44772 9 4 9.44772 4 10V19C4 19.5523 4.44772 20 5 20H14C14.5523 20 15 19.5523 15 19V10C15 9.44772 14.5523 9 14 9H5Z" fill="currentColor"></path></svg>Copy</button></span><span class="" data-state="closed"><button class="flex items-center gap-1 px-4 py-1 select-none"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path d="M2.5 5.5C4.3 5.2 5.2 4 5.5 2.5C5.8 4 6.7 5.2 8.5 5.5C6.7 5.8 5.8 7 5.5 8.5C5.2 7 4.3 5.8 2.5 5.5Z" fill="currentColor" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"></path><path d="M5.66282 16.5231L5.18413 19.3952C5.12203 19.7678 5.09098 19.9541 5.14876 20.0888C5.19933 20.2067 5.29328 20.3007 5.41118 20.3512C5.54589 20.409 5.73218 20.378 6.10476 20.3159L8.97693 19.8372C9.72813 19.712 10.1037 19.6494 10.4542 19.521C10.7652 19.407 11.0608 19.2549 11.3343 19.068C11.6425 18.8575 11.9118 18.5882 12.4503 18.0497L20 10.5C21.3807 9.11929 21.3807 6.88071 20 5.5C18.6193 4.11929 16.3807 4.11929 15 5.5L7.45026 13.0497C6.91175 13.5882 6.6425 13.8575 6.43197 14.1657C6.24513 14.4392 6.09299 14.7348 5.97903 15.0458C5.85062 15.3963 5.78802 15.7719 5.66282 16.5231Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path><path d="M14.5 7L18.5 11" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path></svg>Edit</button></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre!"><span><span>+</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>+</span><span>
</span><span>|</span><span>        Stack </span><span>c</span><span>ủa Task            </span><span>|</span><span>
</span><span>|</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>|</span><span>
</span><span>|</span><span> R0 </span><span>-</span><span> R3</span><span>,</span><span> R12</span><span>,</span><span> LR</span><span>,</span><span> PC</span><span>,</span><span> xPSR       </span><span>|</span><span></span><span><-</span><span> đượ</span><span>c</span><span> CPU tự động POP
</span><span>|</span><span> R4 </span><span>-</span><span> R11                         </span><span>|</span><span></span><span><-</span><span> đượ</span><span>c</span><span> LDMIA R0</span><span>!</span><span>,</span><span></span><span>{</span><span>R4</span><span>-</span><span>R11</span><span>}</span><span>
</span><span>+</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>-</span><span>+</span><span>

</span><span>1.</span><span> MSR PSP</span><span>,</span><span></span><span><</span><span>giá trị lưu trong TCB</span><span>></span><span>
</span><span>2.</span><span> LDMIA R0</span><span>!</span><span>,</span><span></span><span>{</span><span>R4</span><span>-</span><span>R11</span><span>}</span><span>
</span><span>3.</span><span> BX LR → CPU tự POP phần </span><span>c</span><span>òn lại và nhảy về PC
</span></span></code></div></div></pre>

---

## 💬 Kết luận:

* Quá trình **truy xuất ngữ cảnh** được thực hiện trong **PendSV Handler** sau khi chọn xong `nextTCB`.
* Nó là phần  **bắt buộc trong context switch** , để CPU có thể tiếp tục thực hiện task đã được lưu lại trước đó.
