echo off

REM Build the project
echo Building...

REM Xóa các file cũ
del main.o syscalls.o main.exe main.map 2>nul

REM Biên dịch syscalls.c thành syscalls.o
C:\arm-none-eabi\bin\arm-none-eabi-gcc.exe -c syscalls.c -o syscalls.o -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=gnu11

REM Biên dịch main.c thành main.o
C:\arm-none-eabi\bin\arm-none-eabi-gcc.exe -c main.c -o main.o -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=gnu11

REM Liên kết và tạo map file
C:\arm-none-eabi\bin\arm-none-eabi-gcc.exe main.o syscalls.o -o main.exe -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,-Map=main.map

REM Hiển thị thông tin về các section
C:\arm-none-eabi\bin\arm-none-eabi-objdump.exe -h main.o > log.txt

REM Hiển thị bảng ký hiệu (symbol table)
C:\arm-none-eabi\bin\arm-none-eabi-nm.exe main.o >> log.txt