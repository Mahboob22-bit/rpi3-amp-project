# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **Raspberry Pi 3 AMP (Asymmetric Multiprocessing) project** - a port of TImada's Raspberry Pi 4 OpenAMP/FreeRTOS implementation to the Raspberry Pi 3 platform.

**Goal:** Run Linux on cores 0-2 and bare-metal/FreeRTOS code on core 3, with inter-processor communication via OpenAMP/RPMsg.

**Architecture:**
```
┌─────────────┬─────────────┬─────────────┬──────────────┐
│   Core 0    │   Core 1    │   Core 2    │    Core 3    │
├─────────────┼─────────────┼─────────────┼──────────────┤
│   Linux     │   Linux     │   Linux     │  Bare-Metal  │
│  (Master)   │             │             │   (Remote)   │
└─────────────┴─────────────┴─────────────┴──────────────┘
       │                                          │
       └──────────── OpenAMP/RPMsg ──────────────┘
```

## Current Status (Last Updated: 2025-11-26)

**✅ PHASE 4 IN PROGRESS - Shared Memory IPC Working!**

```
Status: WORKING ✅
├── Phase 1: Planning & Setup               [DONE]
├── Phase 2: Memory Reservation             [DONE]
├── Phase 3A: Userspace Launcher            [FAILED - Cache Issues]
├── Phase 3B: U-Boot Boot Method            [DONE - WORKING! 🎉]
├── Phase 4: Simple IPC (Shared Memory)     [IN PROGRESS ✅]
│   ├── Shared Memory Status Structure      [DONE]
│   ├── Linux Reader Tool                   [DONE]
│   └── Mailbox IPC                         [NEXT]
├── Phase 5: OpenAMP/RPMsg                  [PLANNED]
└── Phase 6: FreeRTOS Integration           [PLANNED]
```

**What Works Now:**
- Core 3 runs **modular bare-metal firmware** at 0x20000000
- **Shared Memory IPC** at 0x20A00000 (Linux can read Core 3 status!)
- **Real timestamps** via System Timer (1 MHz)
- **Periodic heartbeat** every 5 seconds
- Linux boots on Cores 0-2 (parallel execution)
- Memory reservation enforced (512 MB Linux, 12 MB AMP)
- UART output from Core 3 visible with ASCII banner
- SSH to Linux works
- **SSH deployment** workflow (no SD card swap needed!)

**Boot Flow:**
1. GPU loads U-Boot (kernel8.img)
2. U-Boot loads Core 3 binary to 0x20000000
3. U-Boot writes to ARM Spin Table (0xF0)
4. Core 3 starts executing bare-metal code
5. U-Boot loads Linux kernel (kernel8.img.backup)
6. Linux boots on Cores 0-2

**Key Files on SD Card:**
- `/boot/firmware/kernel8.img` - U-Boot (637 KB)
- `/boot/firmware/boot.scr` - U-Boot boot script (2.5 KB)
- `/boot/firmware/core3_amp.bin` - Core 3 firmware (~12 KB)
- `/boot/firmware/kernel8.img.backup` - Original Linux kernel (9.3 MB)

**Linux Tools (on RPi):**
- `/usr/local/bin/read_shared_mem` - Read Core 3 status from shared memory

**Documentation:**
- `PHASE3B_SUCCESS.md` - Complete Phase 3B report
- `CURRENT_STATUS.md` - Quick start for next steps
- This file (CLAUDE.md) - Project overview and guidelines

## Critical Hardware Differences: RPi3 vs RPi4

Understanding these differences is essential for the port:

| Component | RPi3 (BCM2837) | RPi4 (BCM2711) |
|-----------|----------------|----------------|
| CPU | Cortex-A53 @ 1.2 GHz | Cortex-A72 @ 1.5 GHz |
| Peripheral Base | `0x3F000000` | `0xFE000000` |
| ARM Local Base | `0x40000000` | `0xFF800000` |
| Interrupt Controller | ARM Local IRQ | GIC-400 |
| ACT LED Control | I2C GPIO Expander (GPU Mailbox) | Direct GPIO |

**CRITICAL:** The peripheral base addresses are different and must be updated throughout the codebase.

## Memory Map

```
0x00000000 - 0x1FFFFFFF  | 512 MB  | Linux (mem=512M in cmdline.txt)
0x20000000 - 0x209FFFFF  |  10 MB  | Bare-Metal Code/Data (RESERVED)
0x20A00000 - 0x20BFFFFF  |   2 MB  | Shared Memory IPC (UNCACHED)
0x3F000000 - 0x3FFFFFFF  |  16 MB  | BCM2837 Peripherals
0x40000000 - 0x40000FFF  |   4 KB  | ARM Local Peripherals (Mailboxes!)
```

**Note:** Memory reservation is enforced via device tree (`no-map`) and kernel cmdline (`mem=512M`).

## Repository Structure

```
rpi3_amp_project/
├── rpi4_ref/                  # TImada's RPi4 FreeRTOS (reference)
├── rpi4_rpmsg_ref/            # TImada's RPi4 OpenAMP/RPMsg (reference)
│   ├── libmetal/              # Hardware abstraction layer
│   ├── open-amp/              # OpenAMP framework
│   └── samples/               # Example applications
├── rpi3_tutorial_ref/         # Bare-metal RPi3 tutorials (reference)
├── rpi3_amp/                  # OUR MAIN PROJECT ✅
│   ├── rpi3_amp_core3/        # Core 3 bare-metal firmware (MODULAR!)
│   │   ├── boot.S             # Assembly startup
│   │   ├── link.ld            # Linker script (0x20000000)
│   │   ├── common.h           # Hardware addresses, types
│   │   ├── uart.h / uart.c    # UART0 driver with printf
│   │   ├── timer.h / timer.c  # System Timer (timestamps)
│   │   ├── memory.h / memory.c # Shared memory & memtest
│   │   ├── cpu_info.h / .c    # CPU info (currently disabled)
│   │   ├── main.c             # Main program with heartbeat
│   │   ├── Makefile           # Build + SSH deploy
│   │   └── core3_amp.bin      # Compiled binary (~12 KB)
│   ├── linux_tools/           # Linux-side tools
│   │   ├── read_shared_mem.c  # Shared memory reader
│   │   └── Makefile           # Build + deploy
│   ├── dts/                   # Device tree overlays
│   └── README.md              # Project documentation
├── u-boot/                    # U-Boot compiled files (ready to deploy)
│   ├── u-boot.bin             # Compiled U-Boot binary (637 KB)
│   ├── boot.scr               # Compiled boot script (2.5 KB)
│   ├── boot.scr.txt           # Boot script source
│   └── README.md              # Deployment guide
├── u-boot-rpi3/               # U-Boot source code (if rebuild needed)
│   ├── tools/mkimage          # Boot script compiler
│   └── ...                    # Full U-Boot source tree
└── arm-gnu-toolchain-.../     # Cross-compiler toolchain
```

## Build System

### Toolchain
- **Cross-Compiler:** `aarch64-none-elf-gcc` (ARM GNU Toolchain 13.2.rel1)
- Located in: `arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf/`
- Add to PATH: `export PATH=$PATH:/path/to/arm-gnu-toolchain-.../bin`

### Common Build Commands

**Core 3 Bare-Metal Firmware (Modular Build):**
```bash
cd rpi3_amp/rpi3_amp_core3
make clean && make        # Build firmware
make deploy               # Deploy via SSH to RPi3
make deploy-reboot        # Deploy and reboot RPi3
make help                 # Show all targets
# Produces: core3_amp.bin (~12 KB)
```

**Linux Tools (for reading shared memory):**
```bash
cd rpi3_amp/linux_tools
make deploy               # Build on RPi3 and install
# Then on RPi3: sudo read_shared_mem
# Or watch mode: sudo read_shared_mem -w
```

**U-Boot Boot Script (if modifications needed):**
```bash
cd u-boot-rpi3
# Edit boot.scr.txt first, then compile:
./tools/mkimage -A arm64 -O linux -T script -C none -d boot.scr.txt boot.scr
# Copy to u-boot/ directory for deployment
cp boot.scr ../u-boot/
```

**U-Boot Full Rebuild (rarely needed):**
```bash
cd u-boot-rpi3
make CROSS_COMPILE=aarch64-none-elf- rpi_3_defconfig
make CROSS_COMPILE=aarch64-none-elf- -j4
# Produces: u-boot.bin
# Copy to u-boot/ directory
cp u-boot.bin ../u-boot/
```

**Device Tree Compilation:**
```bash
# Compile device tree overlay
dtc -@ -O dtb -o output.dtbo input.dtso

# Decompile for inspection
dtc -I dtb -O dts -o output.dts input.dtb
```

## Key Porting Tasks

### 1. libmetal Platform Layer (`libmetal/lib/system/freertos/raspi3/`)

**Files to port from raspi4/:**
- `sys.c` - Platform initialization, peripheral base addresses
- `io.c` - MMIO operations
- `irq.c` - Interrupt handling (ARM Local IRQ, NOT GIC!)

**Critical changes:**
```c
// In sys.c
#define PERIPHERAL_BASE   0x3F000000  // Changed from 0xFE000000
#define ARM_LOCAL_BASE    0x40000000  // Changed from 0xFF800000

// Mailbox addresses (ARM Local)
#define CORE0_MBOX3_SET   (ARM_LOCAL_BASE + 0x80)
#define CORE3_MBOX3_SET   (ARM_LOCAL_BASE + 0xB0)
// etc.

// Remove all GIC-400 code - RPi3 uses simpler ARM Local interrupts
```

### 2. OpenAMP Platform Info (`open-amp/apps/machine/raspi3/`)

**Files to create:**
- `platform_info.c` - Mailbox functions, memory mapping
- `rsc_table.c` - Resource table for remoteproc

**Key implementations:**
- Mailbox-based IPC using ARM Local Mailboxes (not GIC SGI)
- Shared memory setup at 0x20A00000
- VirtIO ring buffer initialization

### 3. Cache Coherency Management

**CRITICAL:** Cortex-A53 does NOT have hardware cache coherency enabled by default.

**Solutions:**
1. Mark shared memory as uncached in MMU configuration:
```c
{
    .addr = 0x20A00000,  // Shared Memory
    .size = SIZE_2M,
    .sharable = OUTER_SHARABLE,
    .policy = TYPE_MEM_UNCACHED,  // Important!
}
```

2. Manual cache operations:
```c
void metal_cache_flush(void *addr, size_t size) {
    uintptr_t start = (uintptr_t)addr & ~63UL;
    uintptr_t end = ((uintptr_t)addr + size + 63) & ~63UL;

    for (uintptr_t va = start; va < end; va += 64) {
        asm volatile("dc cvac, %0" : : "r"(va));
    }
    asm volatile("dsb sy");
}
```

## Critical Issues & Solutions

### ACT LED Control (RPi3 Limitation)

**Problem:** On RPi3 Model B, the ACT LED is NOT controllable via GPIO 47 directly. It's connected through an I2C GPIO expander and requires GPU Mailbox Property Interface.

**Solution for Testing:** Use an external LED on GPIO 17 instead:
```c
#define TEST_LED_PIN  17  // Physical Pin 11
// Hardware: GPIO 17 → 330Ω resistor → LED → GND
```

See `ERRATA_CRITICAL_FIXES.md` for complete details and corrected code.

### Interrupt System Differences

**RPi4:** GIC-400 with sophisticated interrupt routing
**RPi3:** ARM Local interrupt controller (simpler)

When porting `irq.c`, remove all GIC-specific code and use ARM Local interrupt registers at 0x40000000.

### Memory Reservation

Both device tree AND kernel cmdline must reserve memory:

**Device Tree:**
```dts
reserved-memory {
    amp_reserved@20000000 {
        compatible = "shared-dma-pool";
        reg = <0x20000000 0x1000000>;
        no-map;  // Critical - prevents Linux from using this memory
    };
}
```

**cmdline.txt:**
```
maxcpus=3 mem=512M
```

## Mailbox System

**Two Different Mailbox Systems - Don't Confuse Them:**

1. **ARM Local Mailboxes (Core-to-Core IPC)** - Base: 0x40000000
   - Used for AMP/OpenAMP communication
   - Core 3 Mailbox 3 SET: 0x400000B0
   - Core 0 Mailbox 3 SET: 0x40000080

2. **GPU Property Mailbox (ARM ↔ VideoCore)** - Base: 0x3F00B880
   - Used for ACT LED control (if needed)
   - NOT required for basic AMP functionality

## Development Workflow

### Testing Strategy

**Phase 1: Bare-Metal Basics**
1. External LED blink test on Core 3 (GPIO 17)
2. UART debug output
3. Verify code runs at correct address

**Phase 2: Multi-Core**
1. Boot Linux with `maxcpus=3`
2. Verify memory reservation
3. Wake Core 3 from Linux via mailbox

**Phase 3: OpenAMP Integration**
1. Port libmetal
2. Port OpenAMP platform layer
3. Test RPMsg ping-pong

**Phase 4: Stability**
1. Cache coherency verification
2. Performance testing
3. Long-duration stability tests

### Debugging

**UART Setup (RPi3 has ONLY 2 UARTs!):**
- GPIO 14/15 (UART0 PL011) - Primary UART, used by Linux console by default
- GPIO 14/15 (UART1 Mini UART, ALT5) - Secondary UART (limited features)

**⚠️ IMPORTANT FOR RPi3:**
- **UART2-5 do NOT exist on BCM2837!** They only exist on RPi4 (BCM2711)
- For AMP setup: Disable Linux console on UART0, use UART0 exclusively for bare-metal debug
- Alternative: Use UART1 (mini UART) for bare-metal, but it's less stable

**Linux Diagnostics:**
```bash
# Check CPUs
cat /proc/cpuinfo | grep processor

# Check reserved memory
cat /proc/iomem | grep reserved

# Check UIO devices (for shared memory access)
ls -la /sys/class/uio/

# Kernel messages
dmesg | grep -iE "amp|rpmsg|remoteproc"
```

## Important Documentation References

Located in project root:
- `ERRATA_CRITICAL_FIXES.md` - **READ FIRST** - Known issues and corrections
- `rpi3_amp_documentation.md` - Complete technical documentation
- `quick_reference_card.md` - Hardware addresses and register definitions
- `week1_action_plan.md` - Step-by-step implementation guide

**External Documentation:**
- BCM2835 ARM Peripherals PDF (applies to BCM2837)
- BCM2836 QA7 - ARM Local Peripherals (CRITICAL for mailboxes!)
- ARM Cortex-A53 Technical Reference Manual
- OpenAMP Documentation: https://openamp.readthedocs.io/

## Code Style & Conventions

- Follow existing style in TImada's reference implementation
- Use explicit types: `uint32_t`, `uintptr_t`
- Volatile pointers for all MMIO: `volatile uint32_t *`
- Memory barriers after MMIO writes: `asm volatile("dsb sy")`
- Document all hardware address changes with comments

## Known Working Configuration

**Hardware:**
- Raspberry Pi 3 Model B
- USB-UART adapter for debugging
- External LED + resistor for visual feedback

**Software:**
- Raspberry Pi OS Lite (64-bit)
- Cross-compiler: ARM GNU Toolchain 13.2.rel1
- Device Tree Compiler (dtc)
- CMake >= 3.10

## Next Steps for Development

### Phase 4: Simple IPC (Current Goal)

**Recommended approach - Start Simple:**

1. **Shared Memory Communication (Easiest - Start Here!)**
   - Modify `rpi3_amp_core3/main.c` to write test pattern to 0x20A00000
   - Write Linux userspace program to read via `/dev/mem`
   - Verify data integrity without interrupts
   - Code example:
   ```c
   // Core 3: Write to shared memory
   volatile uint32_t *shared = (volatile uint32_t *)0x20A00000;
   shared[0] = 0xDEADBEEF;
   ```

2. **Mailbox-Based IPC (Medium Complexity)**
   - Use ARM Local Mailboxes at 0x40000000
   - Core 3 waits for messages in loop
   - Linux sends commands via mailbox write
   - Add interrupt handling on Core 3

3. **OpenAMP/RPMsg (Final Goal - Complex)**
   - Port libmetal platform layer for RPi3
   - Port OpenAMP platform info
   - Implement full remoteproc/rpmsg framework
   - Similar to TImada's RPi4 implementation

### General Development Guidelines

When starting work:
1. Ensure cross-compiler is in PATH
2. Review `ERRATA_CRITICAL_FIXES.md` for known issues
3. Review `CURRENT_STATUS.md` for current status and quick commands
4. Test incrementally - don't skip steps!
5. Use UART debugging liberally

When debugging:
1. Always check peripheral base addresses (0x3F000000 for RPi3)
2. Verify memory reservation in both DT and cmdline
3. Use UART output liberally for bare-metal debugging
4. Check mailbox addresses carefully (ARM Local vs GPU Property)
5. Remember: Cache coherency is CRITICAL on RPi3!
