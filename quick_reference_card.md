# RPi3 AMP - Quick Reference Card

## 🎯 Memory Map Übersicht

```
RPi3 Physical Address Space:

0x00000000 - 0x1FFFFFFF  |  512 MB  | Linux RAM
0x20000000 - 0x209FFFFF  |   10 MB  | Bare-Metal Code/Data
0x20A00000 - 0x20BFFFFF  |    2 MB  | Shared Memory (IPC)
0x20C00000 - 0x3EFFFFFF  |  403 MB  | Linux RAM (cont.)
0x3F000000 - 0x3FFFFFFF  |   16 MB  | BCM2837 Peripherals
0x40000000 - 0x40000FFF  |    4 KB  | ARM Local Peripherals
```

---

## 📍 Critical Hardware Addresses

### BCM2837 Peripherals (Base: 0x3F000000)

| Peripheral | Offset | Full Address | Description |
|------------|--------|--------------|-------------|
| **GPIO** | +0x200000 | 0x3F200000 | GPIO Control |
| GPFSEL0 | +0x200000 | 0x3F200000 | Function Select 0-9 |
| GPFSEL1 | +0x200004 | 0x3F200004 | Function Select 10-19 |
| GPFSEL4 | +0x200010 | 0x3F200010 | Function Select 40-49 |
| GPSET0 | +0x20001C | 0x3F20001C | Set GPIO 0-31 |
| GPSET1 | +0x200020 | 0x3F200020 | Set GPIO 32-53 |
| GPCLR0 | +0x200028 | 0x3F200028 | Clear GPIO 0-31 |
| GPCLR1 | +0x20002C | 0x3F20002C | Clear GPIO 32-53 |
| **UART0** | +0x201000 | 0x3F201000 | PL011 UART |
| **UART1** | +0x215000 | 0x3F215000 | Mini UART |
| **SPI0** | +0x204000 | 0x3F204000 | SPI Master |
| **I2C0** | +0x205000 | 0x3F205000 | I2C Master 0 |
| **I2C1** | +0x804000 | 0x3F804000 | I2C Master 1 |
| **Timer** | +0x003000 | 0x3F003000 | System Timer |
| **Mailbox** | +0x00B880 | 0x3F00B880 | Property Mailbox |

### ARM Local Peripherals (Base: 0x40000000)

| Register | Offset | Full Address | Description |
|----------|--------|--------------|-------------|
| **Control** | +0x00 | 0x40000000 | ARM Local Control |
| **Prescaler** | +0x08 | 0x40000008 | Timer Prescaler |
| **Timer IRQ** | +0x40 | 0x40000040 | Core Timer IRQ Control |
| **Mailboxes** | | | |
| Core 0 MB3 SET | +0x80 | 0x40000080 | Core 0 Mailbox 3 Set |
| Core 0 MB3 CLR | +0xC0 | 0x400000C0 | Core 0 Mailbox 3 Clear |
| Core 1 MB3 SET | +0x90 | 0x40000090 | Core 1 Mailbox 3 Set |
| Core 1 MB3 CLR | +0xD0 | 0x400000D0 | Core 1 Mailbox 3 Clear |
| Core 2 MB3 SET | +0xA0 | 0x400000A0 | Core 2 Mailbox 3 Set |
| Core 2 MB3 CLR | +0xE0 | 0x400000E0 | Core 2 Mailbox 3 Clear |
| Core 3 MB3 SET | +0xB0 | 0x400000B0 | Core 3 Mailbox 3 Set |
| Core 3 MB3 CLR | +0xF0 | 0x400000F0 | Core 3 Mailbox 3 Clear |
| **IRQ Sources** | | | |
| Core 0 IRQ | +0x60 | 0x40000060 | Core 0 IRQ Source |
| Core 1 IRQ | +0x64 | 0x40000064 | Core 1 IRQ Source |
| Core 2 IRQ | +0x68 | 0x40000068 | Core 2 IRQ Source |
| Core 3 IRQ | +0x6C | 0x4000006C | Core 3 IRQ Source |
| **FIQ Sources** | | | |
| Core 0 FIQ | +0x70 | 0x40000070 | Core 0 FIQ Source |
| Core 1 FIQ | +0x74 | 0x40000074 | Core 1 FIQ Source |
| Core 2 FIQ | +0x78 | 0x40000078 | Core 2 FIQ Source |
| Core 3 FIQ | +0x7C | 0x4000007C | Core 3 FIQ Source |

---

## 🔧 Register Bit Definitions

### GPIO Function Select (GPFSEL)

```
Each GPIO pin: 3 bits
000 = Input
001 = Output
100 = Alt Function 0
101 = Alt Function 1
110 = Alt Function 2
111 = Alt Function 3
011 = Alt Function 4
010 = Alt Function 5

Example: GPIO 14 (UART TX)
GPFSEL1 bits [12:14] = 100 (ALT0)
```

### Important GPIO Pins (RPi3)

| GPIO | Function | Alt Func | Notes |
|------|----------|----------|-------|
| 14 | UART0 TXD | ALT0 | Linux Console |
| 15 | UART0 RXD | ALT0 | Linux Console |
| 0 | UART2 TXD | ALT4 | FreeRTOS/Bare-Metal |
| 1 | UART2 RXD | ALT4 | FreeRTOS/Bare-Metal |
| 47 | ACT LED | Output | On-board LED |

### ARM Local IRQ Source Bits

```
Bit 0: CNTPS IRQ
Bit 1: CNTPNS IRQ
Bit 2: CNTHP IRQ
Bit 3: CNTV IRQ
Bit 4-7: Mailbox 0-3 IRQ
Bit 8: GPU IRQ (from VC)
Bit 9: PMU IRQ
Bit 10: AXI Outstanding IRQ
Bit 11: Local Timer IRQ
```

---

## 💾 Memory Regions for AMP

### Bare-Metal Memory (Core 3)

```c
// Code Region
#define BAREMETAL_CODE_BASE     0x20000000
#define BAREMETAL_CODE_SIZE     0x00200000  // 2 MB

// Data Region
#define BAREMETAL_DATA_BASE     0x20200000
#define BAREMETAL_DATA_SIZE     0x00400000  // 4 MB

// Stack Region
#define BAREMETAL_STACK_BASE    0x20600000
#define BAREMETAL_STACK_SIZE    0x00100000  // 1 MB

// Reserved
#define BAREMETAL_RESERVED_BASE 0x20700000
#define BAREMETAL_RESERVED_SIZE 0x00300000  // 3 MB
```

### Shared Memory (IPC)

```c
// Resource Table
#define RSC_TABLE_BASE          0x20A00000
#define RSC_TABLE_SIZE          0x00001000  // 4 KB

// VirtIO Vrings
#define VRING0_BASE             0x20A10000
#define VRING0_SIZE             0x00004000  // 16 KB

#define VRING1_BASE             0x20A20000
#define VRING1_SIZE             0x00004000  // 16 KB

// Shared Buffers
#define SHARED_BUF_BASE         0x20A30000
#define SHARED_BUF_SIZE         0x001D0000  // ~1.8 MB
```

---

## 🚀 Code Snippets

### Mailbox: Core 3 aufwecken (von Linux)

```c
#include <sys/mman.h>
#include <fcntl.h>

void wake_core3(uint32_t jump_addr) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    
    void *arm_local = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, 0x40000000);
    
    volatile uint32_t *mbox_set = 
        (volatile uint32_t*)((char*)arm_local + 0xB0);
    volatile uint32_t *mbox_clr = 
        (volatile uint32_t*)((char*)arm_local + 0xF0);
    
    *mbox_clr = 0xFFFFFFFF;  // Clear first
    *mbox_set = jump_addr;   // Set jump address
    
    asm volatile("sev");     // Send event
    
    munmap(arm_local, 4096);
    close(fd);
}
```

### Mailbox: Auf Nachricht warten (Bare-Metal)

```c
#define ARM_LOCAL_BASE  0x40000000
#define CORE3_MBOX3_CLR (ARM_LOCAL_BASE + 0xF0)
#define CORE3_IRQ_SRC   (ARM_LOCAL_BASE + 0x6C)

uint32_t wait_mailbox(void) {
    volatile uint32_t *mbox_clr = (volatile uint32_t*)CORE3_MBOX3_CLR;
    volatile uint32_t *irq_src = (volatile uint32_t*)CORE3_IRQ_SRC;
    
    // Wait for Mailbox IRQ (bit 4)
    while (!(*irq_src & (1 << 4))) {
        asm volatile("wfe");
    }
    
    // Read & clear mailbox
    uint32_t data = *mbox_clr;
    *mbox_clr = 0xFFFFFFFF;
    
    return data;
}
```

### GPIO: LED blinken

```c
#define GPIO_BASE   0x3F200000
#define GPFSEL4     ((volatile uint32_t*)(GPIO_BASE + 0x10))
#define GPSET1      ((volatile uint32_t*)(GPIO_BASE + 0x20))
#define GPCLR1      ((volatile uint32_t*)(GPIO_BASE + 0x2C))

void init_act_led(void) {
    // GPIO 47: FSEL4 bits [21:23] = 001 (Output)
    uint32_t ra = *GPFSEL4;
    ra &= ~(7 << 21);
    ra |= (1 << 21);
    *GPFSEL4 = ra;
}

void led_on(void)  { *GPSET1 = (1 << (47-32)); }
void led_off(void) { *GPCLR1 = (1 << (47-32)); }
```

### UART: Debug Output

```c
#define UART0_BASE  0x3F201000
#define UART0_DR    ((volatile uint32_t*)(UART0_BASE + 0x00))
#define UART0_FR    ((volatile uint32_t*)(UART0_BASE + 0x18))

void uart_putc(char c) {
    // Wait until TX FIFO not full
    while (*UART0_FR & (1 << 5));
    *UART0_DR = c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}
```

---

## 🔍 Debug Commands

### Linux-Seite Diagnostics

```bash
# CPU Info
cat /proc/cpuinfo | grep processor

# Memory Map
cat /proc/iomem | grep -E "System|reserved"

# Device Tree
dtc -I fs /sys/firmware/devicetree/base

# UIO Devices
ls -la /sys/class/uio/
cat /sys/class/uio/uio0/name
cat /sys/class/uio/uio0/maps/map0/{addr,size}

# Kernel Messages
dmesg | grep -iE "amp|rpmsg|remoteproc|core"

# Reserved Memory
cat /proc/device-tree/reserved-memory/*/reg | od -t x4
```

### Bare-Metal UART Debug

```bash
# Connect USB-UART Adapter:
# GPIO 14 (TX) -> RX
# GPIO 15 (RX) -> TX
# GND -> GND

# Monitor on PC:
screen /dev/ttyUSB0 115200

# or
minicom -D /dev/ttyUSB0 -b 115200

# or
python -m serial.tools.miniterm /dev/ttyUSB0 115200
```

---

## ⚙️ Build Commands

### Bare-Metal Build

```bash
# Simple Makefile-based
make CROSS=aarch64-none-elf- all

# CMake-based (for OpenAMP)
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-rpi3.cmake
make VERBOSE=1
```

### Device Tree Compile

```bash
# DTS -> DTB
dtc -O dtb -o output.dtb input.dts

# DTS -> DTBO (Overlay)
dtc -@ -O dtb -o output.dtbo input.dtso

# DTB -> DTS (Decompile)
dtc -I dtb -O dts -o output.dts input.dtb
```

### U-Boot Commands

```bash
# U-Boot Prompt Commands
=> printenv
=> setenv bootargs "maxcpus=3 mem=512M"
=> saveenv
=> fatload mmc 0:1 ${kernel_addr_r} kernel8.img
=> booti ${kernel_addr_r} - ${fdt_addr}
```

---

## 📊 Performance Expectations

### Mailbox Latency
- Write: ~100 ns
- Read: ~100 ns
- IRQ trigger: ~1-5 µs

### IPC Throughput (RPMsg)
- Small messages (<512 B): ~10,000 msg/sec
- Large messages (>4 KB): ~2,000 msg/sec
- Bandwidth: ~50-100 MB/sec

### Boot Times
- Linux (3 cores): ~15-20 sec
- Bare-Metal ready: ~100 ms after Linux
- First RPMsg: ~200 ms after boot

---

## 🐛 Common Errors & Solutions

### Error 1: "Kernel panic - not syncing"
**Symptom:** Linux doesn't boot
**Check:**
- `maxcpus=3` in cmdline.txt?
- Device tree reserves memory correctly?
- No conflicting overlays?

### Error 2: Core 3 not waking up
**Symptom:** Mailbox write but no response
**Check:**
- Correct mailbox address (0x400000B0)?
- SEV instruction executed?
- Core 3 in WFE loop?
- Jump address valid?

### Error 3: "Unable to handle kernel paging request"
**Symptom:** Kernel crash when accessing memory
**Check:**
- Memory region reserved in DT?
- `/dev/mem` opened with O_SYNC?
- Correct physical address (not virtual)?

### Error 4: GPIO doesn't work
**Symptom:** LED not blinking
**Check:**
- Correct GPIO pin (47 for ACT LED)?
- Function select set to output?
- RPi3 vs RPi3+ difference?
- Active HIGH vs LOW?

---

## 📚 Essential Documentation

```
BCM2835 ARM Peripherals (applies to BCM2837):
→ GPIO, UART, SPI, I2C, Timers, Mailbox

BCM2836 ARM-local peripherals (QA7):
→ Core Mailboxes, Local Interrupts, Local Timer
→ CRITICAL for multi-core!

ARM Cortex-A53 MPCore TRM:
→ Multiprocessor features, SCU, GIC interface

ARM Cortex-A53 TRM:
→ MMU, Caches, Exception handling

ARMv8-A Architecture Reference Manual:
→ Instruction Set, System Registers, EL0-EL3
```

---

## 🎯 Quick Test Checklist

### ✅ Bare-Metal Basics
- [ ] LED blinks
- [ ] UART outputs "Hello"
- [ ] Delay loop works

### ✅ Multi-Core
- [ ] Linux boots with 3 cores
- [ ] Core 3 reserved
- [ ] Memory reserved

### ✅ Mailbox
- [ ] Can write to mailbox
- [ ] Can read from mailbox
- [ ] IRQ triggers
- [ ] Core wakes up

### ✅ OpenAMP
- [ ] Resource table found
- [ ] VirtIO rings initialized
- [ ] RPMsg channel created
- [ ] Messages exchanged

---

## 🔗 Quick Links

```
TImada RPi4 Reference:
https://github.com/TImada/raspi4_freertos_rpmsg

OpenAMP Docs:
https://openamp.readthedocs.io/

RPi Documentation:
https://www.raspberrypi.org/documentation/

RPi Forums - Bare Metal:
https://forums.raspberrypi.com/viewforum.php?f=72

ARM Developer:
https://developer.arm.com/
```

---

**Print this and keep it handy while coding! 📄**
