bảng cheat sheet PDF đẹp

Dưới đây là  **tổng hợp các lệnh ASM (Assembly) cơ bản trong kiến trúc ARM** , chủ yếu áp dụng cho dòng  **ARM Cortex-M (ví dụ STM32)** . Mình sẽ chia theo nhóm chức năng cho dễ hiểu:

---

## 🧠 1. **Lệnh dữ liệu (Data-processing instructions)**

| Lệnh                           | Mô tả                         | Ví dụ            |
| ------------------------------- | ------------------------------- | ------------------ |
| `MOV`                         | Gán giá trị                  | `MOV R0, #1`     |
| `MVN`                         | Đảo bit                       | `MVN R1, R0`     |
| `ADD`                         | Cộng                           | `ADD R0, R0, #1` |
| `SUB`                         | Trừ                            | `SUB R1, R1, #1` |
| `RSB`                         | Trừ ngược (Reverse Subtract) | `RSB R2, R1, #0` |
| `ADC`                         | Cộng có nhớ                  | `ADC R0, R1, R2` |
| `SBC`                         | Trừ có nhớ                   | `SBC R0, R1, R2` |
| `MUL`                         | Nhân                           | `MUL R0, R1, R2` |
| `AND`,`ORR`,`EOR`,`BIC` | AND, OR, XOR, Bit clear         | `AND R0, R1, R2` |

---

## 🧮 2. **Lệnh dịch bit & xoay bit (Shift/Rotate)**

| Lệnh   | Mô tả                | Ví dụ            |
| ------- | ---------------------- | ------------------ |
| `LSL` | Dịch trái            | `LSL R0, R1, #2` |
| `LSR` | Dịch phải (logic)    | `LSR R0, R1, #1` |
| `ASR` | Dịch phải (số học) | `ASR R0, R1, #1` |
| `ROR` | Xoay phải             | `ROR R0, R1, #8` |

---

## 🧩 3. **Lệnh load/store (truy xuất bộ nhớ)**

| Lệnh             | Mô tả                           | Ví dụ                 |
| ----------------- | --------------------------------- | ----------------------- |
| `LDR`           | Load từ bộ nhớ vào thanh ghi  | `LDR R0, =0x20000000` |
| `STR`           | Store từ thanh ghi vào bộ nhớ | `STR R1, [R0]`        |
| `LDRB`/`STRB` | Load/Store 1 byte                 | `LDRB R2, [R1]`       |

---

## 🔁 4. **Lệnh nhảy (Branching)**

| Lệnh                                 | Mô tả                                          | Ví dụ        |
| ------------------------------------- | ------------------------------------------------ | -------------- |
| `B label`                           | Nhảy không điều kiện                        | `B loop`     |
| `BL label`                          | Nhảy có lưu địa chỉ quay lại (gọi hàm)  | `BL my_func` |
| `BX Rn`                             | Nhảy tới địa chỉ trong thanh ghi (trả về) | `BX LR`      |
| `BEQ`,`BNE`,`BGT`,`BLT`, v.v. | Nhảy có điều kiện                           | `BEQ done`   |

---

## 🔄 5. **Lệnh so sánh và logic**

| Lệnh   | Mô tả        | Ví dụ        |
| ------- | -------------- | -------------- |
| `CMP` | So sánh       | `CMP R0, #0` |
| `CMN` | So sánh cộng | `CMN R1, #1` |
| `TST` | Test AND       | `TST R0, #1` |
| `TEQ` | Test XOR       | `TEQ R1, R2` |

---

## 🛠️ 6. **Lệnh quản lý ngữ cảnh (Context)**

| Lệnh        | Mô tả                        | Ví dụ              |
| ------------ | ------------------------------ | -------------------- |
| `PUSH {}`  | Đẩy dữ liệu vào stack     | `PUSH {R4-R7, LR}` |
| `POP {}`   | Kéo dữ liệu ra khỏi stack  | `POP {R4-R7, PC}`  |
| `NOP`      | Không làm gì (No Operation) | `NOP`              |
| `SVC #imm` | Gọi service (hệ thống)      | `SVC #0`           |

---

## ⏳ 7. **Lệnh đặc biệt**

| Lệnh         | Mô tả                       | Ví dụ     |
| ------------- | ----------------------------- | ----------- |
| `CPSID i`   | Tắt ngắt                    | `CPSID i` |
| `CPSIE i`   | Bật ngắt                    | `CPSIE i` |
| `WFI`       | Chờ ngắt                    | `WFI`     |
| `BKPT #imm` | Breakpoint (dừng để debug) | `BKPT #0` |

---

## 🔢 Thanh ghi phổ biến:
