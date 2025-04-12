+ Một chương trình:
    + Code
    + Data
+ Code sẽ được lưu trữ ở phần .text trong FLASH
+ Data sẽ được lưu trong FLASH hoặc RAM
    + Ví dụ nếu là data const thì lưu trữ trong FLASH vì lưu trữ trong RAM không có ý nghĩa
    + FLASH mặc định xem là bộ nhớ chỉ ghi
    + RAM là đọc ghi
+ Phần .rodata cũng sẽ không lưu trữ trong RAM, vì rodata lưu trữ hằng số của chương trình
+ Các biến bình thường sẽ lư trong bộ nhớ đọc ghi được vì nó có thể thay đổi trong lúc runtime

File object:
    .data: Lưu trữ các biến đã được khởi tạo    => SRAM
    .bss: Lưu trữ các biến chưa được khởi tạo    => SRAM
    .rodata: Lưu trữ các hằng số read only     => FLASH
    .text: Lưu trữ mã chương trình             => FLASH
    User define sections: phần do người dùng tạo ra, tùy mục đích => FLASH

