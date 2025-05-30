# Bộ câu hỏi và trả lời phỏng vấn Embedded Software Engineer

## 1. Kiến thức về vi điều khiển
### Câu hỏi:
- Vi điều khiển là gì? Nêu các thành phần chính của một vi điều khiển.
- So sánh vi điều khiển và vi xử lý.
- Bạn đã từng làm việc với loại vi điều khiển nào? Hãy mô tả một project thực tế.

### Trả lời mẫu:
- Vi điều khiển (Microcontroller) là một hệ thống tích hợp gồm CPU, bộ nhớ (RAM, ROM/Flash), các ngoại vi (GPIO, Timer, ADC, UART, SPI, I2C...) trên một chip duy nhất, dùng để điều khiển các thiết bị nhúng.
- Vi điều khiển tích hợp nhiều ngoại vi, phù hợp cho các ứng dụng nhúng nhỏ gọn, trong khi vi xử lý thường mạnh hơn, dùng cho máy tính, yêu cầu thêm chip ngoại vi.
- Ví dụ: "Tôi từng lập trình STM32 để điều khiển động cơ DC, đọc cảm biến nhiệt độ qua ADC, giao tiếp UART với module Bluetooth."

## 2. Lập trình nhúng (Embedded Programming)
### Câu hỏi:
- Điểm khác biệt giữa con trỏ và mảng trong C là gì?
- Làm thế nào để quản lý bộ nhớ hiệu quả trên hệ thống nhúng?
- Hãy giải thích về stack và heap, ưu nhược điểm khi sử dụng.
- Bạn xử lý lỗi tràn bộ nhớ như thế nào?
- Hãy mô tả cách debug một chương trình nhúng khi gặp lỗi hard fault.
- RTOS là gì? Nêu các khái niệm task, scheduler, semaphore, mutex, queue.

### Trả lời mẫu:
- Con trỏ là biến lưu địa chỉ, mảng là tập hợp các phần tử cùng kiểu. Con trỏ có thể trỏ tới bất kỳ vùng nhớ nào, mảng có kích thước cố định.
- Quản lý bộ nhớ hiệu quả bằng cách hạn chế cấp phát động, ưu tiên dùng biến cục bộ, kiểm soát kỹ khi dùng malloc/free, tránh memory leak.
- Stack dùng cho biến cục bộ, tốc độ truy xuất nhanh, tự động thu hồi; heap dùng cho cấp phát động, linh hoạt nhưng dễ gây rò rỉ bộ nhớ nếu không giải phóng đúng cách.
- Khi gặp lỗi tràn bộ nhớ, cần kiểm tra lại kích thước stack, tối ưu code, tránh đệ quy sâu, kiểm soát cấp phát động.
- Debug hard fault bằng cách sử dụng debugger, kiểm tra giá trị thanh ghi, stack trace, xác định vị trí lỗi, kiểm tra truy cập bộ nhớ sai.
- RTOS (Real-Time Operating System) là hệ điều hành thời gian thực, quản lý đa nhiệm. Task là tiến trình con, scheduler là bộ lập lịch, semaphore/mutex dùng đồng bộ, queue dùng truyền dữ liệu giữa các task.

## 3. Công cụ phát triển
### Câu hỏi:
- Bạn sử dụng IDE, toolchain, debugger nào cho phát triển nhúng?
- Hãy mô tả quy trình build và flash firmware lên vi điều khiển.
- Bạn sử dụng Git như thế nào trong quản lý mã nguồn?

### Trả lời mẫu:
- Tôi thường dùng STM32CubeIDE/Keil/IAR để lập trình, sử dụng OpenOCD/J-Link để debug, toolchain GCC ARM để build.
- Quy trình: viết code, build ra file .hex/.bin, dùng ST-Link Utility hoặc OpenOCD để nạp firmware vào chip.
- Git giúp quản lý version, làm việc nhóm, tôi thường tạo branch cho từng tính năng, commit rõ ràng, sử dụng pull request để review code.

## 4. AUTOSAR
### Câu hỏi:
- AUTOSAR là gì? Nêu các layer chính trong AUTOSAR.
- Bạn đã từng cấu hình module AUTOSAR nào chưa? Quy trình cấu hình và sinh code tự động ra sao?
- So sánh AUTOSAR Classic và Adaptive.

### Trả lời mẫu:
- AUTOSAR (AUTomotive Open System ARchitecture) là chuẩn kiến trúc phần mềm cho ngành ô tô, giúp chuẩn hóa, tái sử dụng code.
- Các layer: Application, RTE (Runtime Environment), Basic Software (BSW), Microcontroller Abstraction Layer (MCAL).
- Tôi từng cấu hình module CAN, LIN bằng EB Tresos/DaVinci Configurator, quy trình: chọn module, cấu hình tham số, sinh code tự động, tích hợp vào project.
- Classic phù hợp ECU truyền thống, Adaptive cho hệ thống phức tạp, hỗ trợ Linux, cập nhật OTA.

## 5. Quy trình phát triển phần mềm nhúng
### Câu hỏi:
- Mô tả các bước phát triển một sản phẩm phần mềm nhúng từ ý tưởng đến sản xuất.
- Bạn kiểm thử phần mềm nhúng như thế nào?
- Việc viết tài liệu kỹ thuật có quan trọng không? Bạn thường viết những loại tài liệu nào?

### Trả lời mẫu:
- Các bước: phân tích yêu cầu, thiết kế phần mềm, lập trình, kiểm thử đơn vị, kiểm thử tích hợp, kiểm thử hệ thống, viết tài liệu, bàn giao sản phẩm.
- Kiểm thử bằng unit test, test trên board thật, mô phỏng tín hiệu ngoại vi, kiểm tra tương thích phần cứng.
- Viết tài liệu giúp bảo trì, chuyển giao, tôi thường viết tài liệu thiết kế, hướng dẫn sử dụng, tài liệu API.

## 6. Kỹ năng mềm
### Câu hỏi:
- Làm việc nhóm trong dự án nhúng có gì đặc biệt?
- Bạn xử lý xung đột trong nhóm như thế nào?
- Kể về một lần bạn gặp khó khăn và cách vượt qua.

### Trả lời mẫu:
- Làm việc nhóm cần phối hợp giữa phần cứng và phần mềm, giao tiếp rõ ràng, chia sẻ tiến độ thường xuyên.
- Khi có xung đột, tôi lắng nghe ý kiến các bên, trao đổi thẳng thắn, tìm giải pháp chung.
- Ví dụ: "Khi debug lỗi giao tiếp CAN kéo dài, tôi chủ động trao đổi với team hardware, cùng kiểm tra tín hiệu, cuối cùng phát hiện lỗi do cấu hình baudrate."

## Gợi ý ôn tập
- Làm project nhỏ với vi điều khiển (STM32, AVR, PIC...)
- Thực hành cấu hình module AUTOSAR với EB Tresos
- Viết code C/C++ xử lý ngoại vi, ngắt, giao tiếp
- Đọc tài liệu, datasheet, chuẩn AUTOSAR
- Tham gia các diễn đàn, nhóm kỹ thuật nhúng

---

## 7. RTOS nâng cao
### Câu hỏi:
- Sự khác biệt giữa RTOS và hệ điều hành thông thường?
- Ưu nhược điểm khi sử dụng RTOS trong hệ thống nhúng?
- Giải thích khái niệm priority inversion và cách xử lý?
- Cách debug lỗi deadlock, starvation trong RTOS?
- Bạn đã từng sử dụng RTOS nào? So sánh FreeRTOS, uC/OS, ThreadX...
- Làm thế nào để tối ưu hiệu năng khi thiết kế task trong RTOS?

### Trả lời mẫu:
- RTOS đảm bảo đáp ứng thời gian thực, có scheduler ưu tiên, hỗ trợ đồng bộ hóa, còn OS thông thường không đảm bảo deadline.
- Ưu điểm: quản lý đa nhiệm, đáp ứng nhanh, dễ mở rộng; nhược điểm: tăng độ phức tạp, cần tối ưu tài nguyên.
- Priority inversion là hiện tượng task ưu tiên thấp giữ tài nguyên khiến task ưu tiên cao bị chặn; xử lý bằng priority inheritance hoặc ceiling protocol.
- Debug bằng trace, kiểm tra thứ tự cấp phát tài nguyên, sử dụng timeout, log trạng thái task.
- Đã dùng FreeRTOS (nhẹ, phổ biến), uC/OS (thương mại, bảo mật tốt), ThreadX (hỗ trợ IoT mạnh).
- Tối ưu bằng cách phân chia task hợp lý, tránh task ưu tiên thấp chiếm CPU lâu, sử dụng semaphore/mutex đúng cách.

---

## 8. ISR (Interrupt Service Routine) và xử lý ngắt
### Câu hỏi:
- ISR là gì? Quy trình xử lý ngắt trong vi điều khiển?
- ISR nên và không nên làm gì?
- Làm thế nào để truyền dữ liệu giữa ISR và main code an toàn?
- Bạn đã từng gặp lỗi gì khi lập trình ISR? Cách khắc phục?
- Ưu nhược điểm của polling và interrupt?

### Trả lời mẫu:
- ISR là hàm xử lý khi có ngắt xảy ra, thường được gọi tự động bởi phần cứng.
- ISR nên xử lý nhanh, tránh gọi hàm blocking, không dùng cấp phát động, chỉ set flag hoặc lưu dữ liệu tạm.
- Truyền dữ liệu bằng biến volatile, sử dụng queue/ring buffer, bảo vệ bằng disable interrupt hoặc critical section.
- Lỗi thường gặp: ISR quá dài gây mất ngắt, truy cập biến không volatile, tranh chấp tài nguyên; khắc phục bằng tối ưu code, dùng volatile, đồng bộ hóa.
- Polling đơn giản nhưng tốn CPU, interrupt tiết kiệm tài nguyên, đáp ứng nhanh hơn.

---

## 9. Thanh ghi CORE (Core Registers)
### Câu hỏi:
- Thanh ghi core là gì? Kể tên một số thanh ghi quan trọng trên ARM Cortex-M?
- Làm thế nào để đọc/ghi giá trị thanh ghi core trong code C?
- Ứng dụng thực tế của việc thao tác trực tiếp với thanh ghi core?
- Bạn đã từng debug lỗi liên quan đến thanh ghi core chưa? Kinh nghiệm xử lý?

### Trả lời mẫu:
- Thanh ghi core là các thanh ghi đặc biệt của CPU như PC (Program Counter), SP (Stack Pointer), LR (Link Register), PSR (Program Status Register), CONTROL...
- Có thể truy cập bằng inline assembly hoặc sử dụng CMSIS: ví dụ đọc MSP bằng __get_MSP().
- Ứng dụng: chuyển đổi chế độ stack, kiểm tra trạng thái CPU, xử lý context switch trong RTOS.
- Debug lỗi stack overflow, hard fault bằng cách kiểm tra giá trị SP, PC, phân tích stack trace, sử dụng debugger để xem nội dung thanh ghi.