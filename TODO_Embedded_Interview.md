# TODO Chuẩn bị phỏng vấn Embedded Software Engineer

## 1. Kiến thức nền tảng về vi điều khiển (Microcontroller)
- Hiểu cấu trúc cơ bản của vi điều khiển (CPU, RAM, Flash, Peripheral...)
- Biết cách đọc datasheet, sơ đồ chân, sơ đồ khối
- Nắm vững các giao tiếp cơ bản: UART, SPI, I2C, CAN, LIN
- Thực hành lập trình điều khiển GPIO, Timer, Interrupt

## 2. Lập trình nhúng (Embedded Programming)
- Thành thạo C/C++ cho hệ thống nhúng
- Quản lý bộ nhớ, stack/heap, con trỏ, struct, union
- Kỹ thuật debug, trace, sử dụng breakpoint, watch
- Hiểu về RTOS: task, scheduler, semaphore, mutex, queue
- Biết cách tối ưu code, xử lý lỗi, quản lý tài nguyên

## 3. Công cụ phát triển & môi trường
- Sử dụng thành thạo IDE (IAR, Keil, Eclipse, VSCode...)
- Làm việc với toolchain, makefile, linker script
- Sử dụng oscilloscope, logic analyzer, debugger (JTAG/SWD)
- Quản lý version với Git

## 4. AUTOSAR & phần mềm ô tô
- Hiểu kiến trúc AUTOSAR: BSW, RTE, SWC
- Biết về các module BSW: Com, Can, Dcm, PduR, MemIf...
- Cách cấu hình và sinh code với EB Tresos, DaVinci, Vector GENy
- Làm việc với file ARXML, ECUC
- Quy trình tích hợp, build, test phần mềm trên ECU

## 5. Quy trình phát triển phần mềm nhúng
- Hiểu V-Model, quy trình phát triển phần mềm ô tô
- Kỹ năng viết tài liệu: SRS, SDD, Test Case
- Quy trình kiểm thử: Unit Test, Integration Test, System Test
- Công cụ quản lý lỗi: Jira, Redmine...

## 6. Kỹ năng mềm
- Kỹ năng giao tiếp, teamwork, trình bày ý tưởng
- Quản lý thời gian, chủ động học hỏi
- Kỹ năng giải quyết vấn đề, tư duy logic

---

## Giải thích chi tiết từng mục
### 1. Vi điều khiển
Bạn cần nắm rõ cấu trúc phần cứng, cách các peripheral hoạt động, thực hành lập trình điều khiển ngoại vi cơ bản. Đọc hiểu datasheet là kỹ năng bắt buộc.

### 2. Lập trình nhúng
C/C++ là ngôn ngữ chủ đạo. Bạn cần hiểu sâu về quản lý bộ nhớ, xử lý ngắt, thao tác bit, tối ưu hiệu năng. RTOS là điểm cộng lớn.

### 3. Công cụ phát triển
Biết sử dụng các IDE, toolchain, debugger giúp bạn làm việc hiệu quả. Thành thạo Git là yêu cầu cơ bản.

### 4. AUTOSAR
Kiến thức về AUTOSAR rất quan trọng với ngành ô tô. Bạn cần hiểu các layer, module, quy trình cấu hình và sinh code tự động.

### 5. Quy trình phát triển
Nắm được các bước phát triển, kiểm thử, viết tài liệu giúp bạn hòa nhập nhanh với môi trường doanh nghiệp.

### 6. Kỹ năng mềm
Không chỉ giỏi kỹ thuật, bạn cần giao tiếp tốt, làm việc nhóm, chủ động học hỏi và giải quyết vấn đề.

---

## Gợi ý ôn tập
- Làm project nhỏ với vi điều khiển (STM32, AVR, PIC...)
- Thực hành cấu hình module AUTOSAR với EB Tresos
- Viết code C/C++ xử lý ngoại vi, ngắt, giao tiếp
- Đọc tài liệu, datasheet, chuẩn AUTOSAR
- Tham gia các diễn đàn, nhóm kỹ thuật nhúng