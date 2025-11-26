PREFIX = aarch64-none-elf-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a53
ASFLAGS = -mcpu=cortex-a53
LDFLAGS = -nostdlib

# Default: UART2 test
TARGET = kernel8.img
ELF = kernel8.elf
MAIN_SRC = main.c

# Alternative targets
TARGET_MIN = kernel8_minimal.img
TARGET_DEBUG = kernel8_debug.img

.PHONY: all clean disasm minimal debug help

all: $(TARGET)
	@echo ""
	@echo "==================================="
	@echo "Standard UART2 test build complete"
	@echo "==================================="
	@ls -lh $(TARGET)

minimal: MAIN_SRC = main_minimal.c
minimal: TARGET = $(TARGET_MIN)
minimal: ELF = kernel8_minimal.elf
minimal: boot.o main_minimal.o
	$(LD) $(LDFLAGS) -T link.ld boot.o main_minimal.o -o $(ELF)
	$(OBJCOPY) -O binary $(ELF) $(TARGET)
	@echo ""
	@echo "==================================="
	@echo "Minimal test build complete"
	@echo "This does NOTHING except loop"
	@echo "Use this to test if boot works!"
	@echo "==================================="
	@ls -lh $(TARGET)

debug: MAIN_SRC = main_debug.c
debug: TARGET = $(TARGET_DEBUG)
debug: ELF = kernel8_debug.elf
debug: boot.o main_debug.o
	$(LD) $(LDFLAGS) -T link.ld boot.o main_debug.o -o $(ELF)
	$(OBJCOPY) -O binary $(ELF) $(TARGET)
	@echo ""
	@echo "==================================="
	@echo "Debug build complete (LED + UART)"
	@echo "==================================="
	@ls -lh $(TARGET)

boot.o: boot.S
	$(AS) $(ASFLAGS) -c boot.S -o boot.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

main_minimal.o: main_minimal.c
	$(CC) $(CFLAGS) -c main_minimal.c -o main_minimal.o

main_debug.o: main_debug.c
	$(CC) $(CFLAGS) -c main_debug.c -o main_debug.o

$(ELF): boot.o main.o link.ld
	$(LD) $(LDFLAGS) -T link.ld boot.o main.o -o $(ELF)

$(TARGET): $(ELF)
	$(OBJCOPY) -O binary $(ELF) $(TARGET)

disasm: $(ELF)
	$(OBJDUMP) -D $(ELF) > kernel8.list
	@echo "Disassembly saved to kernel8.list"

clean:
	rm -f *.o *.elf *.img *.list

help:
	@echo "Raspberry Pi 3 Bare-Metal Test Suite"
	@echo ""
	@echo "Targets:"
	@echo "  make           - Build standard UART2 test"
	@echo "  make minimal   - Build minimal test (just loops)"
	@echo "  make debug     - Build debug version (LED + UART)"
	@echo "  make disasm    - Create disassembly listing"
	@echo "  make clean     - Remove build files"
	@echo ""
	@echo "Testing Strategy:"
	@echo "  1. Start with 'make minimal'"
	@echo "     → Tests if boot works at all"
	@echo "  2. Then 'make debug'"
	@echo "     → Tests GPIO + UART together"
	@echo "  3. Finally 'make' (standard)"
	@echo "     → Your actual UART2 test"
	@echo ""
	@echo "Hardware Setup for Debug/Standard:"
	@echo "  UART2 TX: GPIO 0  (Pin 27) -> USB Adapter RX"
	@echo "  UART2 RX: GPIO 1  (Pin 28) -> USB Adapter TX"
	@echo "  LED:      GPIO 17 (Pin 11) -> 330Ω -> LED -> GND"
	@echo "  GND:      Pin 39 -> USB Adapter GND"
	@echo ""
	@echo "Terminal: 115200 8N1"
