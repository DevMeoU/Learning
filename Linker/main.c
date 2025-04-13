#include <stdio.h>

/* Thông tin về các section trong file object:
 * 
 * 1. .text (Size: 0x4c bytes)
 *    - VMA/LMA: 0x00000000 (VMA (Virtual Memory Address) và LMA (Load Memory Address
 *    - File offset: 0x34
 *    - Alignment: 2^2 = 4 bytes
 *    - Thuộc tính: CONTENTS, ALLOC, LOAD, RELOC, READONLY, CODE
 *    - Chứa mã máy của chương trình
 *
 * 2. .data (Size: 0xc bytes)
 *    - VMA/LMA: 0x00000000 
 *    - File offset: 0x80
 *    - Alignment: 2^2 = 4 bytes
 *    - Thuộc tính: CONTENTS, ALLOC, LOAD, DATA
 *    - Chứa biến toàn cục/static đã khởi tạo
 *
 * 3. .bss (Size: 0x4 bytes)
 *    - VMA/LMA: 0x00000000
 *    - File offset: 0x8c
 *    - Alignment: 2^2 = 4 bytes 
 *    - Thuộc tính: ALLOC
 *    - Chứa biến toàn cục/static chưa khởi tạo
 *
 * 4. .rodata (Size: 0x3c bytes)
 *    - VMA/LMA: 0x00000000
 *    - File offset: 0x8c
 *    - Alignment: 2^2 = 4 bytes
 *    - Thuộc tính: CONTENTS, ALLOC, LOAD, READONLY, DATA
 *    - Chứa hằng số và chuỗi string literals
 *
 * 5. .comment (Size: 0x46 bytes)
 *    - VMA/LMA: 0x00000000
 *    - File offset: 0xc8
 *    - Alignment: 2^0 = 1 byte
 *    - Thuộc tính: CONTENTS, READONLY
 *    - Chứa thông tin về phiên bản compiler
 *
 * 6. .ARM.attributes (Size: 0x34 bytes)
 *    - VMA/LMA: 0x00000000
 *    - File offset: 0x10e
 *    - Alignment: 2^0 = 1 byte
 *    - Thuộc tính: CONTENTS, READONLY
 *    - Chứa thông tin về target architecture
 */

/* Các biến toàn cục được phân bổ như sau:
 * - int a: Chưa khởi tạo nên nằm trong section .bss (SRAM)
 * - int b = 10: Đã khởi tạo nên nằm trong section .data (SRAM)
 * - const int c = 10: Hằng số nên nằm trong section .rodata (FLASH)
 * - static int d = 10: Biến static đã khởi tạo nên nằm trong section .data (SRAM)
 */
int a;
int b = 10;
const int c = 10;
static int d = 10;

int main(void) {
    /* Các biến cục bộ được phân bổ như sau:
     * - int e, f: Biến thường nằm trên stack (SRAM)
     * - const int g: Hằng số nằm trong section .rodata (FLASH)
     * - static int h: Biến static đã khởi tạo nằm trong section .data (SRAM)
     */
    int e = 10;
    int f = 10;
    const int g = 10;
    static int h = 10;

    /* Các mảng được phân bổ như sau:
     * - int i[10]: Mảng đã khởi tạo nên nằm trong section .data (SRAM)
     * - int j[10]: Mảng chưa khởi tạo ban đầu nằm trong section .bss (SRAM)
     *   Sau khi gán giá trị j[0] và j[1], các phần tử này sẽ được lưu trong SRAM
     */
    int i[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int j[10];
    j[0] = 1;
    j[1] = 2;

    /* Chuỗi "Hello, World!\n" là hằng nên nằm trong section .rodata (FLASH) */
    // printf("Hello, World!\n");
    return 0;
}