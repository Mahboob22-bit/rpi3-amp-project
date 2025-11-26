# rpi3_amp - Core 3 Bare-Metal Code

This directory contains the working Core 3 bare-metal code for the RPi3 AMP project.

## Directory Structure

```
rpi3_amp/
├── rpi3_amp_core3/              # Core 3 bare-metal code (Phase 3B)
│   ├── boot.S                   # Assembly startup code
│   ├── main.c                   # C code (UART, GPIO, main loop)
│   ├── link.ld                  # Linker script
│   ├── Makefile                 # Build system
│   ├── core3_amp.bin            # Compiled binary (ready to deploy!)
│   └── README.md                # Build instructions
│
├── dts/                         # Device Tree Overlays
│   ├── rpi3-amp-reserved-memory.dtso       # Memory reservation v1
│   ├── rpi3-amp-reserved-memory-v2.dtso    # Memory reservation v2
│   ├── rpi3-amp-reserved-memory-v3.dtso    # Memory reservation v3 (used)
│   └── README.md                            # DT documentation
│
├── AMP_CONFIGURATION_GUIDE.md   # Configuration guide
├── PHASE1_COMPLETE.md           # Phase 1 documentation
└── README.md                    # This file
```

## Quick Build

```bash
cd rpi3_amp_core3
make clean && make
# Produces: core3_amp.bin
```

## Deployment

Copy `core3_amp.bin` to RPi boot partition:

```bash
# Mount SD card
sudo mount -t drvfs D: /mnt/d

# Copy binary
sudo cp rpi3_amp_core3/core3_amp.bin /mnt/d/

# Unmount
sudo umount /mnt/d
```

## What It Does

The Core 3 code:
- Runs at address `0x20000000` (AMP reserved memory)
- Initializes UART0 for debug output
- Outputs startup message and counter
- Controls GPIO (LED blinking possible)
- Runs in infinite loop sending messages

## Memory Map

```
0x00000000 - 0x1FFFFFFF  | Linux (512 MB)
0x20000000 - 0x209FFFFF  | Core 3 Code/Data (10 MB)
0x20A00000 - 0x20BFFFFF  | Shared Memory (2 MB, future IPC)
```

## Boot Flow

1. GPU loads U-Boot (`kernel8.img`)
2. U-Boot loads `core3_amp.bin` to `0x20000000`
3. U-Boot writes ARM Spin Table (`0xF0 = 0x20000000`)
4. Core 3 starts executing
5. U-Boot loads Linux kernel
6. Linux boots on Cores 0-2

## Phase Status

- ✅ Phase 1: Memory Reservation (Device Tree)
- ✅ Phase 2: Basic Bare-Metal Code
- ✅ Phase 3B: U-Boot Boot Method (WORKING!)
- 🎯 Phase 4: Simple IPC (Next)

## Documentation

See main project documentation:
- `../PHASE3B_SUCCESS.md` - Complete Phase 3B report
- `../CURRENT_STATUS.md` - Current status and next steps
- `../README.md` - Main project README

## Toolchain

ARM GNU Toolchain 13.2.rel1 (aarch64-none-elf-gcc)
- Download: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
