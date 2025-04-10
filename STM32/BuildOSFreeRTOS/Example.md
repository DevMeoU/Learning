Dưới đây là một ví dụ minh họa cụ thể với các giá trị giả định cho các thanh ghi và con trỏ ngăn xếp (PSP) trong quá trình chuyển đổi ngữ cảnh giữa Task hiện tại (ví dụ Task 1) và Task kế tiếp (ví dụ Task 2). Các giá trị này chỉ mang tính chất minh họa.

---

### Giả sử ban đầu:

**Task 1:**

* PSP ban đầu: **0x2000F000**
* Các thanh ghi được lưu (R4–R11) trong Task 1 có giá trị:
  * R4 = 0x11111111
  * R5 = 0x22222222
  * R6 = 0x33333333
  * R7 = 0x44444444
  * R8 = 0x55555555
  * R9 = 0x66666666
  * R10 = 0x77777777
  * R11 = 0x88888888

**Task 2:**

* PSP ban đầu (đã khởi tạo): **0x2000E000**
* Các thanh ghi R4–R11 ban đầu của Task 2 (trước khi được phục hồi) có giá trị:
  * R4 = 0xAAAAAAAA
  * R5 = 0xBBBBBBBB
  * R6 = 0xCCCCCCCC
  * R7 = 0xDDDDDDDD
  * R8 = 0xEEEEEEEE
  * R9 = 0xFFFFFFFF
  * R10 = 0x12345678
  * R11 = 0x87654321

---

### 1. **Lưu context của Task 1 (task đang chạy)**

**Bước 1:** Lấy PSP của Task 1

* PSP hiện tại được đọc từ thanh ghi PSP = **0x2000F000**

**Bước 2:** Lưu các thanh ghi R4–R11 của Task 1 lên stack

* Trước khi lưu, PSP = 0x2000F000
* Sau lệnh `STMDB R0!, {R4-R11}` (8 thanh ghi = 8 × 4 bytes = 32 bytes), các giá trị sau được lưu lên stack và PSP được cập nhật:
  * Stack tại địa chỉ từ **0x2000EFE0** đến **0x2000EFFC** sẽ chứa:
    * *(0x2000EFFC) = R4 = 0x11111111*
    * *(0x2000EFF8) = R5 = 0x22222222*
    * *(0x2000EFF4) = R6 = 0x33333333*
    * *(0x2000EFF0) = R7 = 0x44444444*
    * *(0x2000EFEc) = R8 = 0x55555555*
    * *(0x2000EFE8) = R9 = 0x66666666*
    * *(0x2000EFE4) = R10 = 0x77777777*
    * *(0x2000EFE0) = R11 = 0x88888888*
  * PSP sau khi lưu trở thành  **0x2000EFE0** .

**Bước 3:** Lưu giá trị PSP hiện tại của Task 1 vào TCB (qua hàm `save_psp_value`)

* Ta lưu PSP = **0x2000EFE0** cho Task 1 trong mảng hoặc biến toàn cục.

---

### 2. **Cập nhật Task kế tiếp (Task 2)**

**Bước 4:** Gọi hàm `update_next_task`

* Ví dụ, nếu chỉ có 2 task, ta sẽ cập nhật biến `current_task` từ 0 sang 1, tức chuyển sang Task 2.

**Bước 5:** Lấy PSP của Task 2 (qua hàm `get_psp`)

* PSP của Task 2 đã được khởi tạo ban đầu =  **0x2000E000** .

---

### 3. **Phục hồi context của Task 2**

**Bước 6:** Phục hồi các thanh ghi R4–R11 của Task 2 từ stack

* Với PSP Task 2 ban đầu =  **0x2000E000** , sau khi khôi phục context (lệnh `LDMIA R0!, {R4-R11}`), các giá trị sau được nạp:
  * Giả sử stack của Task 2 được khởi tạo sẵn với các giá trị:
    * R4 = 0xAAAAAAAA
    * R5 = 0xBBBBBBBB
    * R6 = 0xCCCCCCCC
    * R7 = 0xDDDDDDDD
    * R8 = 0xEEEEEEEE
    * R9 = 0xFFFFFFFF
    * R10 = 0x12345678
    * R11 = 0x87654321
* Sau lệnh `LDMIA R0!, {R4-R11}`, PSP Task 2 sẽ được tăng lên 32 bytes, tức từ **0x2000E000** trở thành **0x2000E020** và các thanh ghi R4–R11 được phục hồi với các giá trị trên.

**Bước 7:** Cập nhật PSP của Task 2 vào CPU

* Lệnh `MSR PSP, R0` ghi giá trị PSP (0x2000E020) vào thanh ghi PSP.

**Bước 8:** Khôi phục LR từ stack

* Lệnh `POP {LR}` lấy giá trị LR đã được lưu trước đó.

**Bước 9:** Trở về Task 2

* Lệnh `BX LR` trả về từ handler, và CPU sẽ tự động phục hồi các thanh ghi còn lại (như R0–R3, R12, PC, xPSR) từ stack của Task 2, giúp Task 2 tiếp tục chạy từ vị trí đã dừng.

---

### Sơ đồ minh họa (dưới dạng text):


Task 1 (trước ngắt):
   PSP = 0x2000F000
   R4 = 0x11111111, R5 = 0x22222222, ... R11 = 0x88888888

Sau lệnh STMDB (lưu context của Task 1):
   Stack Task 1 (PSP giảm):
   0x2000EFE0: [R11 = 0x88888888]
   0x2000EFE4: [R10 = 0x77777777]
   ...
   0x2000EFFC: [R4 = 0x11111111]
   PSP Task 1 = 0x2000EFE0

Lưu giá trị PSP (0x2000EFE0) vào TCB của Task 1

Chuyển sang Task 2:
   TCB Task 2 lưu PSP = 0x2000E000

Phục hồi context của Task 2:
   LDMIA từ PSP=0x2000E000:
      R4 = 0xAAAAAAAA, R5 = 0xBBBBBBBB, ... R11 = 0x87654321
   PSP Task 2 sau phục hồi = 0x2000E020

CPU chuyển về Task 2 và tiếp tục chạy.

---

### Kết luận

Qua ví dụ cụ thể này, bạn có thể thấy rằng:

* **Lưu context** của Task 1 thực hiện bằng cách PUSH các thanh ghi R4–R11 lên stack và lưu lại PSP hiện tại vào TCB của Task 1.
* **Cập nhật task** chuyển đổi chỉ số task (ví dụ từ Task 1 sang Task 2) và lấy PSP của Task 2.
* **Phục hồi context** của Task 2 thực hiện bằng cách POP các thanh ghi R4–R11 từ stack của Task 2 và cập nhật lại PSP vào CPU.

Các bước này đảm bảo rằng khi chuyển đổi giữa các task, toàn bộ trạng thái cần thiết được lưu và phục hồi một cách chính xác, cho phép mỗi task tiếp tục chạy từ đúng điểm đã dừng.
