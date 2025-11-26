# Raspberry Pi 3 AMP Project

**Asymmetric Multiprocessing (AMP) on Raspberry Pi 3 Model B**

Run Linux on cores 0-2 and bare-metal code on core 3 simultaneously!

[![Status](https://img.shields.io/badge/Status-Phase%203B%20Complete-success)]()
[![Hardware](https://img.shields.io/badge/Hardware-RPi3%20Model%20B-red)]()
[![License](https://img.shields.io/badge/License-MIT-blue)]()

---

## 🎯 Project Goal

Port [TImada's Raspberry Pi 4 OpenAMP/FreeRTOS implementation](https://github.com/TImada/raspi4_freertos_rpmsg) to the Raspberry Pi 3 platform.

**Architecture:**
```
┌─────────────┬─────────────┬─────────────┬──────────────┐
│   Core 0    │   Core 1    │   Core 2    │    Core 3    │
├─────────────┼─────────────┼─────────────┼──────────────┤
│   Linux     │   Linux     │   Linux     │  Bare-Metal  │
│  (Master)   │             │             │   (Remote)   │
└─────────────┴─────────────┴─────────────┴──────────────┘
       │                                          │
       └──────────── Future: OpenAMP/RPMsg ──────┘
```

---

## ✅ Current Status (Phase 3B Complete!)

**What Works:**
- ✅ Core 3 runs bare-metal code at 0x20000000
- ✅ Linux boots on Cores 0-2 (parallel execution)
- ✅ Memory reservation enforced (512 MB Linux, 12 MB AMP)
- ✅ UART output from Core 3 visible
- ✅ SSH to Linux works

**Boot Flow:**
1. GPU loads U-Boot (replaces kernel8.img)
2. U-Boot loads Core 3 binary to 0x20000000
3. U-Boot writes to ARM Spin Table (0xF0)
4. Core 3 starts executing bare-metal code
5. U-Boot loads Linux kernel (from kernel8.img.backup)
6. Linux boots on Cores 0-2

**UART Output:**
```
*** RPi3 AMP - Core 3 Bare-Metal ***
Running at: 0x20000000 (AMP Reserved)
Core 3 successfully started!
Message #0
Message #1
...
```

---

## 📁 Repository Structure

```
rpi3_amp_project/
├── docs/                          # Documentation (you're here!)
│   ├── PHASE3B_SUCCESS.md        # Complete Phase 3B report
│   ├── CURRENT_STATUS.md         # Quick start guide
│   ├── CLAUDE.md                 # Project overview & guidelines
│   └── quick_reference_card.md   # Hardware addresses & snippets
│
├── rpi3_amp/                      # Main project code
│   └── rpi3_amp_core3/           # Core 3 bare-metal code
│       ├── boot.S                # Assembly startup
│       ├── main.c                # C code (UART, GPIO, etc.)
│       ├── Makefile              # Build system
│       └── core3_amp.bin         # Compiled binary
│
├── u-boot/                        # U-Boot bootloader files
│   ├── u-boot.bin                # Compiled U-Boot (637 KB)
│   ├── boot.scr.txt              # Boot script source
│   ├── boot.scr                  # Compiled boot script
│   └── README.md                 # U-Boot installation guide
│
└── README.md                      # This file
```

---

## 🚀 Quick Start

### Prerequisites

- Raspberry Pi 3 Model B
- microSD card (16 GB+)
- Raspberry Pi OS Lite (64-bit)
- USB-UART adapter for debugging
- ARM GNU Toolchain 13.2.rel1 (`aarch64-none-elf-gcc`)
  - Download: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
  - Version: `arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf.tar.xz`

### Installation

1. **Prepare SD Card:**
   ```bash
   # Flash Raspberry Pi OS Lite (64-bit) to SD card
   # Boot RPi and configure (enable SSH, set hostname, etc.)
   ```

2. **Setup Memory Reservation:**
   ```bash
   # On RPi, edit cmdline.txt
   sudo nano /boot/firmware/cmdline.txt
   # Add: maxcpus=3 mem=512M iomem=relaxed
   ```

3. **Install U-Boot and Core 3 Code:**
   ```bash
   # Mount SD card on your PC
   sudo mount -t drvfs D: /mnt/d  # WSL example

   # Backup original kernel
   sudo cp /mnt/d/kernel8.img /mnt/d/kernel8.img.backup

   # Install U-Boot as kernel8.img
   sudo cp u-boot/u-boot.bin /mnt/d/kernel8.img

   # Install boot script
   sudo cp u-boot/boot.scr /mnt/d/

   # Install Core 3 binary
   sudo cp rpi3_amp/rpi3_amp_core3/core3_amp.bin /mnt/d/

   sudo umount /mnt/d
   ```

4. **Connect UART and Boot:**
   ```bash
   # Connect USB-UART adapter to GPIO 14/15
   screen /dev/ttyUSB0 115200

   # Insert SD card in RPi and power on
   # You should see Core 3 messages on UART!
   ```

---

## 📚 Documentation

| Document | Description |
|----------|-------------|
| [CLAUDE.md](CLAUDE.md) | Project overview, hardware differences, build system |
| [CURRENT_STATUS.md](CURRENT_STATUS.md) | Current status and next steps |
| [PHASE3B_SUCCESS.md](PHASE3B_SUCCESS.md) | Complete Phase 3B report |
| [quick_reference_card.md](quick_reference_card.md) | Hardware addresses, code snippets |
| [u-boot/README.md](u-boot/README.md) | U-Boot installation and rebuild guide |

---

## 🛠️ Building from Source

### Core 3 Code

```bash
cd rpi3_amp/rpi3_amp_core3
make clean && make
# Produces: core3_amp.bin
```

### U-Boot (if needed)

See [u-boot/README.md](u-boot/README.md) for detailed instructions.

---

## 📊 Project Phases

```
✅ Phase 1: Planning & Setup
✅ Phase 2: Memory Reservation (Device Tree)
✅ Phase 3A: Userspace Launcher (Failed - cache issues)
✅ Phase 3B: U-Boot Boot Method (SUCCESS! 🎉)
🎯 Phase 4: Simple IPC (Mailbox) ← NEXT
⏳ Phase 5: OpenAMP/RPMsg
⏳ Phase 6: FreeRTOS Integration
```

---

## 🔧 Hardware

### Raspberry Pi 3 Model B Specifications

| Component | Details |
|-----------|---------|
| SoC | Broadcom BCM2837 |
| CPU | 4× Cortex-A53 @ 1.2 GHz |
| RAM | 1 GB LPDDR2 |
| Peripheral Base | 0x3F000000 |
| ARM Local Base | 0x40000000 |

### Memory Map

```
0x00000000 - 0x1FFFFFFF  |  512 MB  | Linux
0x20000000 - 0x209FFFFF  |   10 MB  | Bare-Metal Code/Data
0x20A00000 - 0x20BFFFFF  |    2 MB  | Shared Memory (Future IPC)
0x3F000000 - 0x3FFFFFFF  |   16 MB  | BCM2837 Peripherals
0x40000000 - 0x40000FFF  |    4 KB  | ARM Local Peripherals
```

---

## 🤝 Credits

- **TImada** - Original RPi4 FreeRTOS/OpenAMP implementation
- **bztsrc** - RPi3 Bare-Metal Tutorials
- **Raspberry Pi Foundation** - Hardware documentation, armstub8
- **OpenAMP Community** - IPC framework

---

## 📝 License

MIT License - see individual files for details.

---

## 🙏 Acknowledgments

This project is a learning exercise in embedded systems, multi-core programming, and AMP architectures. It's based on and inspired by excellent work from the embedded Linux and bare-metal communities.

---

## 🔗 Related Links

- [TImada's RPi4 FreeRTOS](https://github.com/TImada/raspi4_freertos)
- [TImada's RPi4 OpenAMP/RPMsg](https://github.com/TImada/raspi4_freertos_rpmsg)
- [OpenAMP Documentation](https://openamp.readthedocs.io/)
- [U-Boot](https://www.denx.de/wiki/U-Boot)
- [Raspberry Pi Documentation](https://www.raspberrypi.org/documentation/)

---

**Status:** Phase 3B Complete ✅ | **Next:** Phase 4 - Simple IPC 🚀
