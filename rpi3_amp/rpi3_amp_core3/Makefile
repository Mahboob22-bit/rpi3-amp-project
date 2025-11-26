# =============================================================================
# RPi3 AMP Core 3 Firmware - Makefile
# =============================================================================
#
# Modularer Build für die Core 3 Bare-Metal Firmware
#
# Usage:
#   make              - Build firmware (core3_amp.bin)
#   make clean        - Remove build files
#   make disasm       - Create disassembly listing
#   make size         - Show binary size breakdown
#   make deploy       - Deploy via SSH to RPi3
#   make help         - Show this help
#
# =============================================================================

# Cross-Compiler Toolchain
PREFIX = aarch64-none-elf-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE = $(PREFIX)size

# Compiler Flags
CFLAGS  = -Wall -Wextra -Werror
CFLAGS += -O2
CFLAGS += -ffreestanding
CFLAGS += -nostdinc
CFLAGS += -nostdlib
CFLAGS += -mcpu=cortex-a53
CFLAGS += -std=gnu11

# Assembler Flags
ASFLAGS = -mcpu=cortex-a53

# Linker Flags
LDFLAGS = -nostdlib

# =============================================================================
# Source Files
# =============================================================================

# Assembly sources
ASM_SRCS = boot.S

# C sources (modulare Struktur)
# cpu_info.c deaktiviert - verursacht Crash bei Register-Zugriff
C_SRCS = \
    main.c \
    uart.c \
    timer.c \
    memory.c

# Object files
ASM_OBJS = $(ASM_SRCS:.S=.o)
C_OBJS = $(C_SRCS:.c=.o)
OBJS = $(ASM_OBJS) $(C_OBJS)

# =============================================================================
# Output Files
# =============================================================================

TARGET_ELF = kernel8.elf
TARGET_BIN = kernel8.img
DEPLOY_BIN = core3_amp.bin
DISASM     = kernel8.list
MAP_FILE   = kernel8.map

# =============================================================================
# Deploy Configuration (SSH)
# =============================================================================

RPI_HOST ?= admin@rpi3-amp
RPI_BOOT_DIR ?= /boot/firmware

# =============================================================================
# Build Rules
# =============================================================================

.PHONY: all clean disasm size deploy help info

all: $(DEPLOY_BIN)
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║           BUILD SUCCESSFUL                                     ║"
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║ Output: $(DEPLOY_BIN)"
	@ls -lh $(DEPLOY_BIN) | awk '{print "║ Size:   " $$5}'
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║ Next steps:"
	@echo "║   make deploy   - Deploy to RPi3 via SSH"
	@echo "║   make disasm   - View disassembly"
	@echo "╚════════════════════════════════════════════════════════════════╝"

# Link ELF
$(TARGET_ELF): $(OBJS) link.ld
	@echo "[LD]  $@"
	@$(LD) $(LDFLAGS) -T link.ld $(OBJS) -o $@ -Map=$(MAP_FILE)

# Create binary
$(TARGET_BIN): $(TARGET_ELF)
	@echo "[BIN] $@"
	@$(OBJCOPY) -O binary $< $@

# Copy to deploy name
$(DEPLOY_BIN): $(TARGET_BIN)
	@cp $< $@

# Compile C files
%.o: %.c
	@echo "[CC]  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Assemble S files
%.o: %.S
	@echo "[AS]  $<"
	@$(AS) $(ASFLAGS) -c $< -o $@

# =============================================================================
# Utility Targets
# =============================================================================

clean:
	@echo "Cleaning build files..."
	@rm -f $(OBJS) $(TARGET_ELF) $(TARGET_BIN) $(DEPLOY_BIN) $(DISASM) $(MAP_FILE)
	@echo "Done."

disasm: $(TARGET_ELF)
	@echo "Creating disassembly..."
	@$(OBJDUMP) -d -S $< > $(DISASM)
	@echo "Saved to $(DISASM)"
	@head -100 $(DISASM)

size: $(TARGET_ELF)
	@echo ""
	@echo "Section sizes:"
	@$(SIZE) $<
	@echo ""
	@echo "Detailed breakdown:"
	@$(SIZE) -A $<

info:
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║           BUILD CONFIGURATION                                  ║"
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║ Toolchain: $(PREFIX)gcc"
	@$(CC) --version | head -1 | sed 's/^/║ Version:   /'
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║ Source files:"
	@for f in $(ASM_SRCS) $(C_SRCS); do echo "║   $$f"; done
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║ Output: $(DEPLOY_BIN)"
	@echo "║ Deploy: $(RPI_HOST):$(RPI_BOOT_DIR)"
	@echo "╚════════════════════════════════════════════════════════════════╝"

# =============================================================================
# Deploy via SSH
# =============================================================================

deploy: $(DEPLOY_BIN)
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║           DEPLOYING TO RPi3                                    ║"
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║ Target: $(RPI_HOST):$(RPI_BOOT_DIR)/$(DEPLOY_BIN)"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "[1/3] Uploading binary..."
	scp $(DEPLOY_BIN) $(RPI_HOST):/tmp/$(DEPLOY_BIN)
	@echo "[2/3] Moving to boot partition (requires sudo)..."
	ssh $(RPI_HOST) "sudo cp /tmp/$(DEPLOY_BIN) $(RPI_BOOT_DIR)/$(DEPLOY_BIN)"
	@echo "[3/3] Verifying..."
	ssh $(RPI_HOST) "ls -la $(RPI_BOOT_DIR)/$(DEPLOY_BIN)"
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║           DEPLOY COMPLETE                                      ║"
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║ Reboot the RPi3 to load the new firmware:                      ║"
	@echo "║   ssh $(RPI_HOST) 'sudo reboot'"
	@echo "║                                                                 ║"
	@echo "║ Or connect UART and watch the output:                          ║"
	@echo "║   screen /dev/ttyUSB0 115200                                   ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"

# Convenience target: deploy and reboot
deploy-reboot: deploy
	@echo ""
	@echo "Rebooting RPi3..."
	ssh $(RPI_HOST) "sudo reboot" || true
	@echo "RPi3 is rebooting. Connect UART to see Core 3 output."

# =============================================================================
# Help
# =============================================================================

help:
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║      RPi3 AMP Core 3 Firmware - Build System                   ║"
	@echo "╠════════════════════════════════════════════════════════════════╣"
	@echo "║                                                                 ║"
	@echo "║  BUILD TARGETS:                                                 ║"
	@echo "║    make              Build firmware (core3_amp.bin)             ║"
	@echo "║    make clean        Remove all build artifacts                 ║"
	@echo "║    make disasm       Generate disassembly listing               ║"
	@echo "║    make size         Show section sizes                         ║"
	@echo "║    make info         Show build configuration                   ║"
	@echo "║                                                                 ║"
	@echo "║  DEPLOY TARGETS:                                                ║"
	@echo "║    make deploy       Deploy via SSH to RPi3                     ║"
	@echo "║    make deploy-reboot  Deploy and reboot RPi3                   ║"
	@echo "║                                                                 ║"
	@echo "║  CONFIGURATION:                                                 ║"
	@echo "║    RPI_HOST=user@host    SSH target (default: admin@rpi3-amp)   ║"
	@echo "║    RPI_BOOT_DIR=/path    Boot partition (default: /boot/firmware)║"
	@echo "║                                                                 ║"
	@echo "║  EXAMPLES:                                                      ║"
	@echo "║    make clean && make                                           ║"
	@echo "║    make deploy RPI_HOST=pi@192.168.1.100                        ║"
	@echo "║    make deploy-reboot                                           ║"
	@echo "║                                                                 ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"

# =============================================================================
# Dependencies (auto-generated would be better, but keep it simple)
# =============================================================================

main.o: main.c common.h uart.h timer.h cpu_info.h memory.h
uart.o: uart.c uart.h common.h
timer.o: timer.c timer.h common.h
cpu_info.o: cpu_info.c cpu_info.h common.h uart.h
memory.o: memory.c memory.h common.h uart.h timer.h
