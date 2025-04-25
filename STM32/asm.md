**Dưới đây là tổng hợp các lệnh Assembly (ASM) phổ biến trong ARM, tập trung vào ARM 32-bit (ARMv7 và các phiên bản trước đó như ARMv6), vì đây là kiến trúc thường được sử dụng trong hệ thống nhúng. Tôi sẽ phân loại các lệnh theo chức năng, giải thích ngắn gọn và cung cấp ví dụ. Danh sách này không bao gồm tất cả các lệnh ARM (vì có hàng trăm lệnh), nhưng sẽ bao gồm những lệnh quan trọng và thường dùng nhất, đặc biệt trong bối cảnh lập trình nhúng và RTOS.**

---

**1. Tổng quan về ARM Assembly**

* **ARM sử dụng kiến trúc RISC (Reduced Instruction Set Computer), với khoảng 30 lệnh cơ bản [Web ID: 6].**
* **ARM có hai trạng thái chính: **ARM** (lệnh 32-bit) và **Thumb** (lệnh 16-bit, mở rộng thành 32-bit trong Thumb-2) [Web ID: 9].**
* **Lệnh ARM thường có cú pháp: **MNEMONIC{S}{condition} Rd, Operand1, Operand2
  * **MNEMONIC**: Tên lệnh (ví dụ: ADD, MOV).
  * **{S}**: Tùy chọn, nếu có thì cập nhật cờ trạng thái (flags).
  * **{condition}**: Điều kiện thực thi (ví dụ: EQ, NE).
  * **Rd**: Thanh ghi đích.
  * **Operand1, Operand2**: Toán hạng nguồn (thanh ghi hoặc giá trị tức thời).

---

**2. Phân loại các lệnh ARM Assembly**

**2.1. Lệnh xử lý dữ liệu (Data Processing Instructions)**

**Những lệnh này thực hiện các phép toán số học, logic và dịch chuyển trên thanh ghi.**

| **Lệnh** | **Mô tả**                                                                        | **Ví dụ**                           |
| --------------- | ---------------------------------------------------------------------------------------- | ------------------------------------------- |
| **ADD**   | **Cộng hai toán hạng, lưu kết quả vào thanh ghi đích.**                   | **ADD R0, R1, R2**(R0 = R1 + R2)      |
| **ADC**   | **Cộng với cờ Carry, thường dùng cho phép cộng số lớn.**                 | **ADC R0, R1, R2**(R0 = R1 + R2 + C)  |
| **SUB**   | **Trừ hai toán hạng.**                                                          | **SUB R0, R1, R2**(R0 = R1 - R2)      |
| **SBC**   | **Trừ với cờ Carry.**                                                           | **SBC R0, R1, R2**(R0 = R1 - R2 - !C) |
| **MUL**   | **Nhân hai thanh ghi, lưu kết quả vào thanh ghi đích.**                     | **MUL R0, R1, R2**(R0 = R1 * R2)      |
| **MOV**   | **Di chuyển giá trị từ toán hạng vào thanh ghi đích.**                    | **MOV R0, #5**(R0 = 5)                |
| **MOVLE** | **Di chuyển có điều kiện (Less Than or Equal).**                              | **MOVLE R0, #5**(Nếu LE thì R0 = 5) |
| **MVN**   | **Di chuyển giá trị phủ định (NOT) của toán hạng vào thanh ghi đích.** | **MVN R0, R1**(R0 = ~R1)              |
| **AND**   | **Phép AND logic giữa hai toán hạng.**                                         | **AND R0, R1, R2**(R0 = R1 AND R2)    |
| **ORR**   | **Phép OR logic.**                                                                | **ORR R0, R1, R2**(R0 = R1 OR R2)     |
| **EOR**   | **Phép XOR logic.**                                                               | **EOR R0, R1, R2**(R0 = R1 XOR R2)    |
| **BIC**   | **Bit Clear (xóa bit): R0 = R1 AND NOT(R2).**                                     | **BIC R0, R1, R2**                    |
| **LSL**   | **Dịch trái logic (Logical Shift Left).**                                        | **MOV R0, R1, LSL #1**(R0 = R1 << 1)  |
| **LSR**   | **Dịch phải logic (Logical Shift Right).**                                       | **MOV R0, R1, LSR #1**(R0 = R1 >> 1)  |

**Ghi chú**:

* **Các lệnh như ADD, SUB có thể thêm hậu tố **S** (ví dụ: **ADDS**) để cập nhật cờ trạng thái (Zero, Negative, Carry, Overflow).**
* **ARM hỗ trợ phép dịch bit (barrel shifter) ngay trong lệnh, ví dụ: **MOV R0, R1, LSL #1** [Web ID: 9].**

**2.2. Lệnh tải/lưu (Load/Store Instructions)**

**ARM sử dụng kiến trúc load/store, nghĩa là dữ liệu phải được tải từ bộ nhớ vào thanh ghi trước khi xử lý, và lưu lại sau khi xử lý.**

| **Lệnh** | **Mô tả**                                               | **Ví dụ**                                                |
| --------------- | --------------------------------------------------------------- | ---------------------------------------------------------------- |
| **LDR**   | **Tải dữ liệu từ bộ nhớ vào thanh ghi.**           | **LDR R0, [R1]**(R0 = giá trị tại địa chỉ trong R1)  |
| **STR**   | **Lưu dữ liệu từ thanh ghi vào bộ nhớ.**           | **STR R0, [R1]**(Lưu R0 vào địa chỉ trong R1)         |
| **LDRB**  | **Tải 1 byte từ bộ nhớ.**                             | **LDRB R0, [R1]**                                          |
| **STRB**  | **Lưu 1 byte vào bộ nhớ.**                            | **STRB R0, [R1]**                                          |
| **LDRH**  | **Tải 1 half-word (16-bit) từ bộ nhớ.**               | **LDRH R0, [R1]**                                          |
| **STRH**  | **Lưu 1 half-word vào bộ nhớ.**                       | **STRH R0, [R1]**                                          |
| **LDM**   | **Tải nhiều thanh ghi từ bộ nhớ (Load Multiple).**   | **LDMIA R0!, {R1-R3}**(Tải R1, R2, R3 từ địa chỉ R0)  |
| **STM**   | **Lưu nhiều thanh ghi vào bộ nhớ (Store Multiple).** | **STMIA R0!, {R1-R3}**(Lưu R1, R2, R3 vào địa chỉ R0) |

**Ghi chú**:

* **IA** (Increment After): Tăng địa chỉ sau khi truy cập.
* **!** tự động cập nhật địa chỉ cơ sở (R0 trong ví dụ).

**2.3. Lệnh nhảy và điều kiện (Branch and Conditional Instructions)**

**Dùng để thay đổi luồng điều khiển chương trình.**

| **Lệnh** | **Mô tả**                                                                            | **Ví dụ**                        |
| --------------- | -------------------------------------------------------------------------------------------- | ---------------------------------------- |
| **B**     | **Nhảy đến một nhãn (label).**                                                    | **B loop**(Nhảy đến nhãn loop) |
| **BL**    | **Nhảy đến nhãn và lưu địa chỉ trả về vào thanh ghi R14 (Link Register).** | **BL subroutine**                  |
| **BX**    | **Nhảy đến địa chỉ trong thanh ghi, có thể chuyển trạng thái ARM/Thumb.**   | **BX LR**(Trả về từ hàm)       |
| **CMP**   | **So sánh hai toán hạng, cập nhật cờ trạng thái.**                             | **CMP R1, R2**                     |
| **TST**   | **Kiểm tra bit (AND logic), cập nhật cờ trạng thái.**                            | **TST R1, R2**                     |
| **BEQ**   | **Nhảy nếu bằng (Equal).**                                                          | **BEQ label**                      |
| **BNE**   | **Nhảy nếu không bằng (Not Equal).**                                               | **BNE label**                      |
| **BGT**   | **Nhảy nếu lớn hơn (Greater Than).**                                               | **BGT label**                      |
| **BLT**   | **Nhảy nếu nhỏ hơn (Less Than).**                                                  | **BLT label**                      |
| **IT**    | **If-Then (dùng trong Thumb, điều kiện cho các lệnh tiếp theo).**               | **IT EQ**                          |

**Ghi chú**:

* **ARM hỗ trợ thực thi có điều kiện cho hầu hết các lệnh (ví dụ: **ADDGT R0, R1, R2** - chỉ cộng nếu lớn hơn) [Web ID: 9].**
* **Trong Thumb-2, lệnh **IT** cho phép thực thi có điều kiện [Web ID: 16].**

**2.4. Lệnh quản lý ngăn xếp (Stack Instructions)**

**ARM sử dụng lệnh **PUSH** và **POP** để quản lý ngăn xếp.**

| **Lệnh** | **Mô tả**                                     | **Ví dụ**       |
| --------------- | ----------------------------------------------------- | ----------------------- |
| **PUSH**  | **Đẩy danh sách thanh ghi vào ngăn xếp.** | **PUSH {R4, LR}** |
| **POP**   | **Lấy danh sách thanh ghi từ ngăn xếp.**   | **POP {R4, PC}**  |

**Ghi chú**:

* **ARM tự động đẩy/lấy các thanh ghi theo thứ tự ngược nhau để đảm bảo tính nhất quán [Web ID: 18].**

**2.5. Lệnh điều khiển hệ thống**

**Dùng để tương tác với trạng thái hệ thống hoặc thanh ghi đặc biệt.**

| **Lệnh** | **Mô tả**                                                             | **Ví dụ**                 |
| --------------- | ----------------------------------------------------------------------------- | --------------------------------- |
| **MRS**   | **Đọc trạng thái từ thanh ghi trạng thái (PSR) vào thanh ghi.** | **MRS R0, PSR**             |
| **MSR**   | **Ghi giá trị từ thanh ghi vào thanh ghi trạng thái (PSR).**      | **MSR PSR, R0**             |
| **CPSIE** | **Bật ngắt (Enable Interrupts).**                                     | **CPSIE i**(Bật ngắt IRQ) |
| **CPSID** | **Tắt ngắt (Disable Interrupts).**                                    | **CPSID i**(Tắt ngắt IRQ) |

---

**3. Một số đặc điểm quan trọng của ARM Assembly**

* **Thanh ghi**: ARM có 16 thanh ghi (R0-R15), trong đó:
  * **R13: SP (Stack Pointer).**
  * **R14: LR (Link Register).**
  * **R15: PC (Program Counter).**
* **Điều kiện thực thi**: ARM hỗ trợ các hậu tố điều kiện như EQ (Equal), NE (Not Equal), GT (Greater Than), LT (Less Than) [Web ID: 18].
* **Thumb và Thumb-2**: Thumb sử dụng lệnh 16-bit để tiết kiệm không gian, Thumb-2 mở rộng thêm lệnh 32-bit và hỗ trợ điều kiện [Web ID: 9].

---

**4. Ví dụ minh họa**

**Dưới đây là một chương trình nhỏ sử dụng ARM Assembly để cộng các số từ 1 đến 10:**

**asm**

```text
    PUSH {R4, LR}       @ Lưu R4 và LR vào ngăn xếp
    MOV R4, #0          @ Khởi tạo R4 = 0 (biến đếm)
    MOV R0, #0          @ Khởi tạo R0 = 0 (tổng)
loop:
    ADD R0, R0, R4      @ R0 = R0 + R4
    ADD R4, R4, #1      @ Tăng R4 lên 1
    CMP R4, #10         @ So sánh R4 với 10
    BLE loop            @ Nhảy về loop nếu R4 <= 10
    POP {R4, PC}        @ Khôi phục R4 và quay lại
```

**Giải thích**:

* **PUSH** và **POP** quản lý ngăn xếp.
* **MOV** khởi tạo biến.
* **ADD** thực hiện phép cộng.
* **CMP** và **BLE** tạo vòng lặp có điều kiện.

---

**5. Tài liệu tham khảo**

**Để biết thêm chi tiết, bạn có thể tham khảo:**

* **ARM Architecture Reference Manual (có sẵn trên trang Arm Developer).**
* **Azeria Labs ARM Assembly Tutorials [Web ID: 9].**
* **ARM Assembly By Example [Web ID: 6].**


**Dưới đây là phần so sánh các lệnh trong ARM Assembly, tập trung vào các nhóm lệnh chính như xử lý dữ liệu, tải/lưu, nhảy, và quản lý ngăn xếp. Tôi sẽ giải thích sự khác biệt giữa các lệnh tương tự và cách sử dụng chúng.**

---

**1. Các lệnh xử lý dữ liệu**

**ADD vs ADC**

* **ADD**: Lệnh cộng hai toán hạng mà không sử dụng cờ Carry.
  * **Ví dụ: **ADD R0, R1, R2** → R0 = R1 + R2.**
* **ADC**: Lệnh cộng hai toán hạng và thêm giá trị của cờ Carry.
  * **Ví dụ: **ADC R0, R1, R2** → R0 = R1 + R2 + Carry.**
* **Khác biệt**: ADC sử dụng cờ Carry, hữu ích khi cộng các số lớn (ví dụ: 64-bit), còn ADD thì không.

**SUB vs SBC**

* **SUB**: Lệnh trừ hai toán hạng mà không sử dụng cờ Carry.
  * **Ví dụ: **SUB R0, R1, R2** → R0 = R1 - R2.**
* **SBC**: Lệnh trừ hai toán hạng và trừ thêm giá trị liên quan đến cờ Carry.
  * **Ví dụ: **SBC R0, R1, R2** → R0 = R1 - R2 - (1 - Carry).**
* **Khác biệt**: SBC dùng cho phép trừ số lớn, trong khi SUB là phép trừ cơ bản.

**MOV vs MVN**

* **MOV**: Gán giá trị từ toán hạng vào thanh ghi đích.
  * **Ví dụ: **MOV R0, #5** → R0 = 5.**
* **MVN**: Gán giá trị phủ định (bitwise NOT) của toán hạng vào thanh ghi đích.
  * **Ví dụ: **MVN R0, #5** → R0 = ~5.**
* **Khác biệt**: MVN tạo giá trị phủ định, còn MOV chỉ sao chép giá trị.

**AND vs ORR vs EOR vs BIC**

* **AND**: Thực hiện phép AND bitwise.
  * **Ví dụ: **AND R0, R1, R2** → R0 = R1 & R2.**
* **ORR**: Thực hiện phép OR bitwise.
  * **Ví dụ: **ORR R0, R1, R2** → R0 = R1 | R2.**
* **EOR**: Thực hiện phép XOR bitwise.
  * **Ví dụ: **EOR R0, R1, R2** → R0 = R1 ^ R2.**
* **BIC**: Xóa các bit được thiết lập trong toán hạng thứ hai.
  * **Ví dụ: **BIC R0, R1, R2** → R0 = R1 & ~R2.**
* **Khác biệt**: AND kiểm tra bit, ORR thiết lập bit, EOR đảo bit, BIC xóa bit.

---

**2. Các lệnh tải/lưu**

**LDR vs STR**

* **LDR**: Tải dữ liệu từ bộ nhớ vào thanh ghi.
  * **Ví dụ: **LDR R0, [R1]** → R0 = giá trị tại địa chỉ R1.**
* **STR**: Lưu dữ liệu từ thanh ghi vào bộ nhớ.
  * **Ví dụ: **STR R0, [R1]** → Ghi R0 vào địa chỉ R1.**
* **Khác biệt**: LDR đọc từ bộ nhớ, STR ghi vào bộ nhớ.

**LDRB vs LDRH vs LDR**

* **LDRB**: Tải 1 byte (8 bit).
  * **Ví dụ: **LDRB R0, [R1]** → R0 = byte tại R1.**
* **LDRH**: Tải 1 half-word (16 bit).
  * **Ví dụ: **LDRH R0, [R1]** → R0 = half-word tại R1.**
* **LDR**: Tải 1 word (32 bit).
  * **Ví dụ: **LDR R0, [R1]** → R0 = word tại R1.**
* **Khác biệt**: Kích thước dữ liệu tải về khác nhau (byte, half-word, word).

**LDM vs STM**

* **LDM**: Tải nhiều thanh ghi từ bộ nhớ.
  * **Ví dụ: **LDMIA R0!, {R1-R3}** → Tải R1, R2, R3 từ R0.**
* **STM**: Lưu nhiều thanh ghi vào bộ nhớ.
  * **Ví dụ: **STMIA R0!, {R1-R3}** → Lưu R1, R2, R3 vào R0.**
* **Khác biệt**: LDM tải khối dữ liệu, STM lưu khối dữ liệu.

---

**3. Các lệnh nhảy**

**B vs BL vs BX**

* **B**: Nhảy đến nhãn.
  * **Ví dụ: **B loop** → Nhảy đến nhãn loop.**
* **BL**: Nhảy đến nhãn và lưu địa chỉ trả về vào R14.
  * **Ví dụ: **BL subroutine** → Gọi hàm subroutine.**
* **BX**: Nhảy đến địa chỉ trong thanh ghi.
  * **Ví dụ: **BX LR** → Trả về từ hàm.**
* **Khác biệt**: B nhảy đơn giản, BL gọi hàm, BX nhảy động hoặc trả về.

**CMP vs TST**

* **CMP**: So sánh hai toán hạng, cập nhật cờ trạng thái.
  * **Ví dụ: **CMP R1, R2** → So sánh R1 và R2.**
* **TST**: Kiểm tra bit bằng phép AND, cập nhật cờ.
  * **Ví dụ: **TST R1, #1** → Kiểm tra bit thấp nhất.**
* **Khác biệt**: CMP so sánh giá trị, TST kiểm tra bit.

**BEQ vs BNE vs BGT vs BLT**

* **BEQ**: Nhảy nếu bằng (Z = 1).
* **BNE**: Nhảy nếu không bằng (Z = 0).
* **BGT**: Nhảy nếu lớn hơn (Z = 0 và N = V).
* **BLT**: Nhảy nếu nhỏ hơn (N ≠ V).
* **Khác biệt**: Dựa trên cờ trạng thái sau CMP để điều khiển luồng.

---

**4. Các lệnh quản lý ngăn xếp**

**PUSH vs POP**

* : Đẩy thanh ghi vào ngăn xếp.
  * **Ví dụ: **PUSH {R4, LR}** → Lưu R4 và LR.**
* **POP**: Lấy thanh ghi từ ngăn xếp.
  * **Ví dụ: **POP {R4, PC}** → Khôi phục R4 và PC.**
* **Khác biệt**: PUSH lưu trạng thái, POP khôi phục trạng thái.

Viết chương trình thực hiện các yêu cầu sau:

Câu 1 (3 điểm):

Khai báo lớp

phân số

với các thuộc tính: tử số, mẫu số.

Xây dựng hàm tạo không đối, hàm tạo có đối, hàm hủy một đối tượng phân

số.

Câu 2 (3 điểm):

Xây dựng các phương thức: nhập, xuất, rút gọn một đối tượng phân số.

Câu 3 (4 điểm):

Định nghĩa các toán tử

+,

, *, /

hai phân số. Thực hiện cộng, trừ, nhân, chia

hai phân số (kết quả phải được tối giản) và in kết quả ra màn hình.

(

Y

êu c

ầu:

B

ài l

àm c

ó gi

ải th

ích t

ừng

đo

ạn code (ghi dư

ới d

ạng ch

ú th

ích)

)

---

**Kết luận**

**Các lệnh trong ARM Assembly khác nhau về chức năng và cách sử dụng, từ xử lý dữ liệu (ADD, SUB), tải/lưu (LDR, STR), nhảy (B, BL), đến quản lý ngăn xếp (PUSH, POP). Hiểu rõ sự khác biệt giúp lập trình viên tối ưu mã lệnh cho từng tình huống cụ thể.**
