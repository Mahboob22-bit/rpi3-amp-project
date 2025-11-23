# Raspberry Pi 3 AMP Portierung - Dokumentation

## Ziel
Portierung von TImada's Raspberry Pi 4 OpenAMP/FreeRTOS/RPMsg Projekt auf Raspberry Pi 3

---

## Hardware-Spezifikationen

### Raspberry Pi 3 Model B
- **SoC:** Broadcom BCM2837
- **CPU:** Quad-core ARM Cortex-A53 @ 1.2 GHz (ARMv8-A)
- **RAM:** 1 GB LPDDR2
- **GPU:** VideoCore IV @ 400 MHz
- **Architecture:** ARMv8 (64-bit capable)

### Raspberry Pi 4 Model B (Referenz)
- **SoC:** Broadcom BCM2711
- **CPU:** Quad-core ARM Cortex-A72 @ 1.5 GHz (ARMv8-A)
- **RAM:** 1/2/4/8 GB LPDDR4
- **GPU:** VideoCore VI
- **GIC:** GIC-400

---

## Kritische Adress-Unterschiede

### Memory Map Comparison

| Region | RPi3 (BCM2837) | RPi4 (BCM2711) | Notes |
|--------|----------------|----------------|-------|
| **Peripheral Base** | `0x3F000000` | `0xFE000000` | ⚠️ MUSS geändert werden |
| **ARM Local Base** | `0x40000000` | `0xFF800000` | ⚠️ Mailboxes & Interrupts |
| **GPIO Base** | `0x3F200000` | `0xFE200000` | Peripheral + 0x200000 |
| **UART0 Base** | `0x3F201000` | `0xFE201000` | Peripheral + 0x201000 |
| **Mailbox Base** | `0x3F00B880` | `0xFE00B880` | Property Mailbox |

### ARM Local Peripherals (RPi3)

```
Base Address: 0x40000000

Core 0 Mailbox 3 SET:  0x40000080
Core 0 Mailbox 3 CLR:  0x400000C0
Core 1 Mailbox 3 SET:  0x40000090
Core 1 Mailbox 3 CLR:  0x400000D0
Core 2 Mailbox 3 SET:  0x400000A0
Core 2 Mailbox 3 CLR:  0x400000E0
Core 3 Mailbox 3 SET:  0x400000B0
Core 3 Mailbox 3 CLR:  0x400000F0

Core 0-3 IRQ Source:   0x40000060-0x4000006C
Core 0-3 FIQ Source:   0x40000070-0x4000007C

ARM Local Timer:       0x40000034
ARM Core Timer:        0x40000040
```

### Interrupt System Comparison

| Feature | RPi3 (BCM2837) | RPi4 (BCM2711) |
|---------|----------------|----------------|
| **Type** | ARM Local + VC | GIC-400 |
| **IRQ Routing** | Simple | Advanced |
| **SGI Support** | Limited | Full GIC |
| **IPI Mechanism** | Mailboxes | GIC SGI + Mailboxes |

---

## Erforderliche Dokumentationen

### 📚 Primäre Dokumentation (MUST READ)

1. **BCM2835/BCM2836 ARM Peripherals**
   - URL: https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
   - Enthält: GPIO, UART, SPI, I2C, Timers
   - Note: BCM2837 ist identisch zu BCM2836 (nur CPU gewechselt)

2. **BCM2836 QA7 (ARM Local Peripherals)**
   - Enthält: Mailboxes, Local Interrupts, Local Timers
   - ⚠️ CRITICAL für AMP!

3. **ARM Cortex-A53 Technical Reference Manual**
   - URL: ARM Developer Documentation
   - Enthält: MMU, Caches, Exception Levels, GIC Interface

4. **ARM Cortex-A53 MPCore Processor**
   - Multicore-spezifische Features
   - Cache coherency (SCU)

### 📑 Sekundäre Dokumentation

5. **ARMv8-A Architecture Reference Manual**
   - Exception Levels (EL0-EL3)
   - Memory Management
   - AARCH64 ISA

6. **OpenAMP Documentation**
   - URL: https://openamp.readthedocs.io/
   - remoteproc Framework
   - RPMsg Protocol
   - virtio Transport

7. **libmetal Documentation**
   - Hardware Abstraction Layer
   - System-spezifische Implementierung

---

## TImada's Code-Struktur (RPi4)

```
raspi4_freertos_rpmsg/
├── libmetal/
│   └── lib/system/freertos/raspi4/    ← MUSS portiert werden!
│       ├── sys.c                       ← Platform init
│       ├── sys.h
│       ├── io.c                        ← MMIO operations
│       └── irq.c                       ← Interrupt handling
│
├── open-amp/
│   ├── apps/machine/raspi4/           ← MUSS portiert werden!
│   │   ├── platform_info.c            ← Platform-spezifisch
│   │   └── rsc_table.c                ← Resource Table
│   └── cmake/platforms/
│       └── raspi4_a72_generic.cmake   ← A53 für RPi3!
│
└── samples/
    ├── freertos/rpmsg_ping/           ← FreeRTOS Seite
    │   └── src/
    │       ├── main.c
    │       ├── rpmsg_config.h
    │       └── platform_config.h      ← Adressen!
    └── linux/rpmsg_echo/              ← Linux Seite
        └── src/platform_info.c        ← Linux Platform
```

### Kritische Dateien für Portierung

#### 1. libmetal Platform Layer
```c
// lib/system/freertos/raspi3/sys.c (NEU!)
#define PERIPHERAL_BASE   0x3F000000
#define ARM_LOCAL_BASE    0x40000000
#define CORE3_MBOX3_SET   (ARM_LOCAL_BASE + 0xB0)
#define CORE3_MBOX3_CLR   (ARM_LOCAL_BASE + 0xF0)
```

#### 2. OpenAMP Platform Info
```c
// apps/machine/raspi3/platform_info.c (NEU!)
#define RPMSG_MEM_BASE    0x20600000    // Shared Memory
#define RPMSG_MEM_SIZE    0x200000      // 2 MB
#define RSC_TABLE_BASE    0x20000000    // Resource Table
```

#### 3. Device Tree Overlay
```dts
// dts/raspi3-rpmsg.dtso (NEU!)
/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2837";
    
    fragment@0 {
        target-path="/reserved-memory";
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <1>;
            ranges;
            
            rtos@20000000 {
                compatible = "shared-dma-pool";
                no-map;
                reg = <0x20000000 0xA00000>;  // 10 MB
            };
            
            shm@20600000 {
                compatible = "shared-dma-pool";
                no-map;
                reg = <0x20600000 0x200000>;  // 2 MB
            };
        };
    };
    
    fragment@1 {
        target-path="/";
        __overlay__ {
            shm_uio {
                compatible = "generic-uio";
                status = "okay";
                reg = <0x20600000 0x200000>;
            };
            
            // ARM Local für Mailbox-Zugriff
            arm_local_uio {
                compatible = "generic-uio";
                status = "okay";
                reg = <0x40000000 0x1000>;
            };
        };
    };
};
```

---

## Portierungs-Checkliste

### ✅ Phase 1: Vorbereitung
- [ ] BCM2835/2836 Peripherals PDF lesen
- [ ] BCM2836 QA7 (ARM Local) studieren
- [ ] Cortex-A53 TRM überblicken
- [ ] TImada's Code-Struktur verstehen
- [ ] Entwicklungsumgebung aufsetzen
  - [ ] Cross-Compiler: aarch64-none-elf-gcc
  - [ ] Device Tree Compiler: dtc
  - [ ] CMake >= 3.10
  - [ ] Git

### ⏳ Phase 2: libmetal Portierung
- [ ] Directory erstellen: `lib/system/freertos/raspi3/`
- [ ] sys.c implementieren
  - [ ] metal_sys_init()
  - [ ] metal_sys_cleanup()
  - [ ] Peripheral Base auf 0x3F000000
- [ ] io.c anpassen
  - [ ] MMIO read/write
  - [ ] Memory barriers
- [ ] irq.c implementieren
  - [ ] ARM Local Interrupts
  - [ ] Mailbox IRQs
- [ ] CMake Toolchain: `cmake/platforms/raspi3-freertos.cmake`

### ⏳ Phase 3: OpenAMP Anpassung
- [ ] Directory erstellen: `apps/machine/raspi3/`
- [ ] platform_info.c schreiben
  - [ ] Mailbox-Funktionen
  - [ ] Shared Memory Mapping
  - [ ] Resource Table Location
- [ ] rsc_table.c kopieren & anpassen
- [ ] remoteproc_ops implementieren
  - [ ] rproc_kick() - Mailbox trigger
  - [ ] rproc_notify() - Interrupt empfangen

### ⏳ Phase 4: Device Tree & Linux
- [ ] raspi3-rpmsg.dtso erstellen
- [ ] Reserved Memory Regions
- [ ] UIO Device Nodes
- [ ] Kernel Config prüfen
  - [ ] CONFIG_UIO=y
  - [ ] CONFIG_UIO_PDRV_GENIRQ=y
- [ ] Device Tree kompilieren & testen

### ⏳ Phase 5: FreeRTOS/Bare-Metal
- [ ] Cortex-A53 spezifische Anpassungen
- [ ] MMU Config für A53
- [ ] Cache Management
- [ ] Boot-Code (entry.S)
  - [ ] Core 3 isolieren
  - [ ] Stack Setup
  - [ ] Jump to main()
- [ ] Linker Script anpassen
  - [ ] Code @ 0x20000000
  - [ ] Data @ 0x20200000

### ⏳ Phase 6: Integration & Testing
- [ ] Linux Userspace RPMSG Sample bauen
- [ ] FreeRTOS/Bare-Metal RPMSG Sample bauen
- [ ] Boot-Test
- [ ] IPC-Test (Ping-Pong)
- [ ] Performance-Test
- [ ] Stability-Test

---

## Memory Layout Plan (RPi3)

```
Physical Memory Map (1 GB total):

0x00000000 - 0x0FFFFFFF  |  256 MB  | Linux Kernel & DMA
0x10000000 - 0x1FFFFFFF  |  256 MB  | Linux User Space
0x20000000 - 0x209FFFFF  |   10 MB  | FreeRTOS/Bare-Metal
  ├─ 0x20000000 - 0x201FFFFF  | 2 MB | Code
  ├─ 0x20200000 - 0x205FFFFF  | 4 MB | Data/Stack
  └─ 0x20600000 - 0x207FFFFF  | 2 MB | Reserved
0x20800000 - 0x209FFFFF  |    2 MB  | Shared Memory (IPC)
0x20A00000 - 0x3EFFFFFF  |  340 MB  | Linux Rest
0x3F000000 - 0x3FFFFFFF  |   16 MB  | Peripherals
0x40000000 - 0x40000FFF  |    4 KB  | ARM Local
```

---

## Entwicklungsumgebung Setup

### Cross-Compiler Installation

```bash
# ARM AArch64 Bare-Metal Toolchain
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/\
arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf.tar.xz

tar xf arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf.tar.xz
export PATH=$PATH:$PWD/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf/bin

# Test
aarch64-none-elf-gcc --version
```

### Repository Setup

```bash
# Basis-Repositories clonen
mkdir ~/rpi3_amp
cd ~/rpi3_amp

# TImada's Repos als Referenz
git clone https://github.com/TImada/raspi4_freertos.git
git clone --recursive https://github.com/TImada/raspi4_freertos_rpmsg.git

# Neue Branches für RPi3 Portierung
cd raspi4_freertos_rpmsg
git checkout -b rpi3-port

cd ../raspi4_freertos
git checkout -b rpi3-port
```

---

## Testing Strategy

### Unit Tests
1. **libmetal Tests**
   - Mailbox Write/Read
   - MMIO Operations
   - Interrupt Registration

2. **OpenAMP Tests**
   - Resource Table Parsing
   - VirtIO Ring Setup
   - RPMsg Channel Creation

### Integration Tests
3. **Boot Test**
   - Linux bootet auf Core 0-2
   - Core 3 bleibt verfügbar
   - Memory-Mapping korrekt

4. **IPC Test**
   - FreeRTOS → Linux Message
   - Linux → FreeRTOS Message
   - Bidirektionaler Echo-Test

5. **Stress Test**
   - 1000 Messages/sec
   - Variable Message Sizes
   - Concurrent Operations

---

## Debugging Tools

### Hardware Debugging
- **UART2 (GPIO 0/1):** FreeRTOS Debug Output
- **UART1 (GPIO 14/15):** Linux Console
- **JTAG (optional):** OpenOCD für Core 3

### Software Debugging
```bash
# Linux Seite
dmesg | grep -i uio
ls -la /sys/class/uio/
cat /proc/iomem | grep rtos
cat /proc/iomem | grep shm

# FreeRTOS/Bare-Metal Debug Output via UART2
screen /dev/ttyUSB0 115200
```

---

## Bekannte Herausforderungen

### 1. Cache Coherency
- **Problem:** A53 hat keinen Hardware Cache Coherency ohne SCU
- **Lösung:** Software Cache Management mit Clean/Invalidate

### 2. Interrupt Routing
- **Problem:** RPi3 hat kein GIC wie RPi4
- **Lösung:** ARM Local Interrupts nutzen

### 3. Mailbox Semantik
- **Problem:** BCM2837 Mailboxes unterscheiden sich von BCM2711
- **Lösung:** Genaue Dokumentation QA7 lesen

### 4. Device Tree Support
- **Problem:** Kernel muss UIO für ARM Local unterstützen
- **Lösung:** Custom UIO Driver oder generic-uio nutzen

---

## Erfolgs-Kriterien

### Minimum Viable Product (MVP)
- ✅ Linux bootet auf Core 0-2
- ✅ Bare-Metal Code läuft auf Core 3
- ✅ Ping-Pong Message funktioniert

### Full Implementation
- ✅ OpenAMP remoteproc funktioniert
- ✅ RPMsg Channels stabil
- ✅ Bidirektionale Kommunikation
- ✅ Performance: >1000 msg/sec
- ✅ Stabil über Stunden

---

## Nächste Schritte

1. **Dokumentation studieren** (Phase 1)
   - BCM2835/2836 Peripherals
   - BCM2836 QA7
   - Cortex-A53 TRM

2. **Code-Analyse** (Phase 1)
   - TImada's libmetal durchgehen
   - Mailbox-Implementierung verstehen
   - Resource Table Format

3. **Erste Portierung** (Phase 2)
   - libmetal/sys.c für RPi3
   - Basis-Adressen ändern
   - Kompilieren testen

---

## Ressourcen & Links

### Offizielle Dokumentation
- BCM2835 Peripherals: https://www.raspberrypi.org/documentation/
- ARM Cortex-A53: https://developer.arm.com/
- OpenAMP: https://openamp.readthedocs.io/

### Code-Referenzen
- TImada RPi4: https://github.com/TImada/raspi4_freertos_rpmsg
- Circle RPi3: https://github.com/rsta2/circle
- rpi4-osdev: https://www.rpi4os.com/

### Community
- Raspberry Pi Forums - Bare Metal: https://forums.raspberrypi.com/
- OpenAMP Mailing List: https://lists.openampproject.org/

---

## Lizenz & Credits

Dieses Projekt basiert auf:
- TImada's raspi4_freertos_rpmsg (Original RPi4 Implementation)
- OpenAMP Project (remoteproc & RPMsg)
- libmetal (Hardware Abstraction)

Portierung: Mahboob (2024/2025)
Ziel: RPi3 AMP mit OpenAMP Support
