echo off

REM Build the project
echo Building...
@REM C:\arm-none-eabi\bin\arm-none-eabi-gcc.exe -c startup.c -o startup.o -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=gnu11
C:\arm-none-eabi\bin\arm-none-eabi-gcc.exe -c main.c -o main.o -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=gnu11
@REM C:\arm-none-eabi\bin\arm-none-eabi-gcc.exe startup.o main.o -o program.elf -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

echo Dummy
C:\arm-none-eabi\bin\arm-none-eabi-objdump.exe -h main.o >> log.txt