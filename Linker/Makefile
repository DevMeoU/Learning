# Compiler and flags
CC = C:/arm-none-eabi/bin/arm-none-eabi-gcc.exe
OBJDUMP = C:/arm-none-eabi/bin/arm-none-eabi-objdump.exe
OBJCOPY = C:/arm-none-eabi/bin/arm-none-eabi-objcopy.exe
LINKER = C:/arm-none-eabi/bin/arm-none-eabi-ld.exe
SIZE = C:/arm-none-eabi/bin/arm-none-eabi-size.exe

# CPU flags
CPU_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -std=gnu11 -O0

# Compiler flags
# -fno-builtin Vô hiệu hóa optimization cho memory operations
CFLAGS = $(CPU_FLAGS) -c -std=gnu11 -Wall -Wextra -O2 -g -fno-builtin

# Linker flags
LDFLAGS = -T stm32l443rct6_startup.ld -Map=build/main.map

# Source files
SRCS = main.c stm32l443rct6_startup.c
OBJS = $(SRCS:.c=.o)

# Output directory
OUTDIR = build

# Output files
TARGET = $(OUTDIR)/main.elf
BIN = $(OUTDIR)/main.bin
HEX = $(OUTDIR)/main.hex
OBJS := $(addprefix $(OUTDIR)/,$(notdir $(OBJS)))

# Create output directory
$(OUTDIR):
	@echo Creating output directory $(OUTDIR)
	mkdir $(OUTDIR)

# Default target
all: $(OUTDIR) $(TARGET) $(BIN) $(HEX) size

# Compile source files
$(OUTDIR)/%.o: %.c | $(OUTDIR)
	@echo Compiling $< to $@
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files
$(TARGET): $(OBJS)
	@echo Linking $^ to $@
	$(LINKER) $(LDFLAGS) $^ -o $@

# Create binary file
$(BIN): $(TARGET)
	@echo Creating binary file $@ from $<
	$(OBJCOPY) -O binary $< $@

# Create hex file
$(HEX): $(TARGET)
	@echo Creating hex file $@ from $
	$(OBJCOPY) -O ihex $< $@

# Display size information
size: $(TARGET)
	@echo Size information:
	$(SIZE) $<

# Clean target
clean:
	rm -rf $(OUTDIR)
	rm -f log.txt

# Show section headers
dump:
	$(OBJDUMP) -h $(TARGET) > log.txt

.PHONY: all clean size dump