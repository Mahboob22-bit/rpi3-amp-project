# RPi3 AMP Portierung - Week 1 Action Plan

## 🎯 Ziel Week 1
**Erste funktionierende Basis:** Linux bootet mit 3 Cores, Core 3 führt simplen Bare-Metal Code aus

---

## Tag 1: Setup & Analyse

### Morning Session (3-4h)

#### 1.1 Entwicklungsumgebung einrichten

```bash
# Verzeichnisstruktur
mkdir -p ~/rpi3_amp_project
cd ~/rpi3_amp_project

# Cross-Compiler installieren
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf.tar.xz
tar xf arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf.tar.xz
export PATH=$PATH:$PWD/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf/bin

# Test
aarch64-none-elf-gcc --version
```

#### 1.2 Repositories clonen

```bash
# Referenz-Implementierungen
git clone https://github.com/TImada/raspi4_freertos.git rpi4_ref
git clone --recursive https://github.com/TImada/raspi4_freertos_rpmsg.git rpi4_rpmsg_ref

# Bare-Metal Tutorials als Referenz
git clone https://github.com/bztsrc/raspi3-tutorial.git rpi3_tutorial_ref

# Eigenes Projekt
mkdir rpi3_amp
cd rpi3_amp
git init
```

#### 1.3 Dokumentation herunterladen

```bash
mkdir docs
cd docs

# BCM2835/2836 Peripherals (auch für BCM2837 gültig!)
wget https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf

# BCM2836 ARM Local (QA7) - KRITISCH!
# Manual download von Raspberry Pi Forum/Docs erforderlich

# Notizen machen während Lesen!
```

### Afternoon Session (3-4h)

#### 1.4 Code-Analyse: TImada's Mailbox-Implementierung

```bash
cd ~/rpi3_amp_project/rpi4_rpmsg_ref

# Kritische Dateien identifizieren
find . -name "*mailbox*" -o -name "*platform*" | grep -E "\.(c|h)$"

# Wichtige Dateien:
# libmetal/lib/system/freertos/raspi4/sys.c
# open-amp/apps/machine/raspi4/platform_info.c
```

**Analyse-Fragen beantworten:**
- Wo werden Mailbox-Adressen definiert?
- Wie funktioniert die IRQ-Registrierung?
- Welche GIC-spezifischen Funktionen werden verwendet?
- Wie ist die Memory-Map aufgebaut?

**Notizen machen in:** `analysis_notes.md`

---

## Tag 2: Erster Bare-Metal Test (ohne OpenAMP)

### Morning Session: Simple Bare-Metal auf Core 3

#### 2.1 Projekt-Setup

```bash
cd ~/rpi3_amp_project/rpi3_amp
mkdir -p test_bare_metal
cd test_bare_metal
```

#### 2.2 Simple Boot-Code erstellen

**Datei: `boot.S`**
```asm
.section ".text.boot"

.global _start

_start:
    // Get CPU ID
    mrs     x0, mpidr_el1
    and     x0, x0, #0x3        // Core ID in x0
    
    // Core 0,1,2: halt
    cbz     x0, core0_start
    cmp     x0, #1
    beq     core_halt
    cmp     x0, #2
    beq     core_halt
    
    // Core 3: continue
    cmp     x0, #3
    beq     core3_start
    b       core_halt

core0_start:
    // Core 0 startet Linux später
    // Für jetzt: halt
    b       core_halt

core3_start:
    // Stack für Core 3 setzen
    ldr     x1, =_stack_core3
    mov     sp, x1
    
    // BSS löschen
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b
    
4:  // Jump zu C main
    bl      main
    
core_halt:
    wfe
    b       core_halt

.section ".bss"
.align 16
_stack_core3_bottom:
    .space 4096
_stack_core3:
```

**Datei: `main.c`**
```c
#define PERIPHERAL_BASE   0x3F000000
#define GPIO_BASE         (PERIPHERAL_BASE + 0x200000)

#define GPFSEL1           ((volatile unsigned int*)(GPIO_BASE + 0x04))
#define GPSET0            ((volatile unsigned int*)(GPIO_BASE + 0x1C))
#define GPCLR0            ((volatile unsigned int*)(GPIO_BASE + 0x28))

#define ACT_LED_PIN       47  // RPi3 ACT LED

// Simple delay
void delay(unsigned int count) {
    for (volatile unsigned int i = 0; i < count; i++) {
        asm volatile("nop");
    }
}

void main(void) {
    // Configure ACT LED as output
    // GPIO 47 = FSEL4[21:23]
    unsigned int ra = *GPFSEL1;
    ra &= ~(7 << 21);
    ra |= (1 << 21);  // Output
    *GPFSEL1 = ra;
    
    // Blink ACT LED
    while(1) {
        *GPSET0 = (1 << (ACT_LED_PIN - 32));  // LED ON (Active HIGH on Pi3!)
        delay(1000000);
        *GPCLR0 = (1 << (ACT_LED_PIN - 32));  // LED OFF
        delay(1000000);
    }
}
```

**Datei: `link.ld`**
```ld
SECTIONS
{
    . = 0x80000;  /* Standard Kernel Load Address */
    
    .text : {
        KEEP(*(.text.boot))
        *(.text*)
    }
    
    .rodata : {
        *(.rodata*)
    }
    
    .data : {
        *(.data*)
    }
    
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
    
    __bss_size = (__bss_end - __bss_start) >> 3;
    
    /DISCARD/ : {
        *(.comment)
        *(.gnu*)
        *(.note*)
        *(.eh_frame*)
    }
}
```

**Datei: `Makefile`**
```makefile
PREFIX = aarch64-none-elf-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy

CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a53
ASFLAGS = -mcpu=cortex-a53
LDFLAGS = -nostdlib

all: kernel8.img

boot.o: boot.S
	$(AS) $(ASFLAGS) -c boot.S -o boot.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

kernel8.elf: boot.o main.o link.ld
	$(LD) $(LDFLAGS) -T link.ld boot.o main.o -o kernel8.elf

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary kernel8.elf kernel8.img

clean:
	rm -f *.o *.elf *.img

.PHONY: all clean
```

#### 2.3 Bauen & Testen

```bash
make

# SD-Karte vorbereiten (auf deinem PC)
# - Raspbian Lite Image flashen
# - Boot-Partition mounten
# - kernel8.img ersetzen (Original sichern!)

sudo cp kernel8.img /media/$USER/boot/
sudo sync
```

**Erwartetes Ergebnis:** ACT LED blinkt!

### Afternoon Session: Core-Isolation testen

#### 2.4 Core 3 isolieren mit cmdline.txt

```bash
# Auf der SD-Karte Boot-Partition
sudo nano /media/$USER/boot/cmdline.txt

# Füge hinzu:
maxcpus=3 isolcpus=3
```

#### 2.5 Modified Boot mit U-Boot (besser für AMP!)

**Vorbereitung:**
```bash
# U-Boot für RPi3 kompilieren
git clone https://github.com/u-boot/u-boot.git
cd u-boot
make rpi_3_defconfig
make CROSS_COMPILE=aarch64-linux-gnu-

# u-boot.bin auf SD-Karte
sudo cp u-boot.bin /media/$USER/boot/

# config.txt anpassen
sudo nano /media/$USER/boot/config.txt
# Füge hinzu:
# kernel=u-boot.bin
# arm_64bit=1
```

---

## Tag 3: Linux + Bare-Metal Koexistenz

### Morning Session: Device Tree Overlay

#### 3.1 Eigenes Overlay erstellen

**Datei: `rpi3-amp-test.dts`**
```dts
/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2837";
    
    fragment@0 {
        target-path = "/";
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <1>;
            
            reserved-memory {
                #address-cells = <1>;
                #size-cells = <1>;
                ranges;
                
                /* Reserve memory for Bare-Metal on Core 3 */
                amp_core3: amp_core3@20000000 {
                    compatible = "shared-dma-pool";
                    reg = <0x20000000 0x1000000>;  /* 16 MB */
                    no-map;
                };
            };
        };
    };
    
    fragment@1 {
        target-path = "/cpus";
        __overlay__ {
            cpu@3 {
                status = "disabled";  /* Core 3 für Bare-Metal */
            };
        };
    };
};
```

#### 3.2 Kompilieren & Installieren

```bash
# Device Tree Compiler
sudo apt-get install device-tree-compiler

# Kompilieren
dtc -O dtb -o rpi3-amp-test.dtbo rpi3-amp-test.dts

# Auf SD-Karte
sudo cp rpi3-amp-test.dtbo /media/$USER/boot/overlays/

# In config.txt aktivieren
echo "dtoverlay=rpi3-amp-test" | sudo tee -a /media/$USER/boot/config.txt
```

### Afternoon Session: Mailbox-Test

#### 3.3 Core 3 per Mailbox aufwecken

**Konzept:** Linux (Core 0) lädt Code in 0x20000000 und weckt Core 3 auf

**Linux Userspace Tool: `wake_core3.c`**
```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

#define ARM_LOCAL_BASE  0x40000000
#define CORE3_MBOX3_SET 0xB0
#define CORE3_MBOX3_CLR 0xF0

#define PAGE_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <address>\n", argv[0]);
        return 1;
    }
    
    uint32_t jump_addr = strtoul(argv[1], NULL, 0);
    
    // Open /dev/mem
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }
    
    // Map ARM Local
    void *arm_local = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, ARM_LOCAL_BASE);
    if (arm_local == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    
    volatile uint32_t *mbox_set = (volatile uint32_t*)(arm_local + CORE3_MBOX3_SET);
    volatile uint32_t *mbox_clr = (volatile uint32_t*)(arm_local + CORE3_MBOX3_CLR);
    
    // Clear mailbox
    *mbox_clr = 0xFFFFFFFF;
    
    printf("Writing jump address 0x%08X to Core 3 Mailbox...\n", jump_addr);
    
    // Write jump address
    *mbox_set = jump_addr;
    
    // Send event
    asm volatile("sev");
    
    printf("Core 3 wakeup signal sent!\n");
    
    munmap(arm_local, PAGE_SIZE);
    close(fd);
    
    return 0;
}
```

**Kompilieren:**
```bash
gcc -o wake_core3 wake_core3.c
```

---

## Tag 4-5: libmetal Basis-Portierung

### Tag 4 Morning: Directory-Struktur

```bash
cd ~/rpi3_amp_project/rpi3_amp
mkdir -p libmetal_rpi3/lib/system/freertos/raspi3
cd libmetal_rpi3
```

### Dateien von RPi4 kopieren & anpassen

```bash
# Basis-Files von TImada's RPi4 kopieren
cp -r ~/rpi3_amp_project/rpi4_rpmsg_ref/libmetal/lib/system/freertos/raspi4/* \
      lib/system/freertos/raspi3/

# Jetzt Anpassungen vornehmen!
```

### Tag 4 Afternoon: sys.c anpassen

**Key Changes in `lib/system/freertos/raspi3/sys.c`:**

```c
// CHANGE 1: Peripheral Base
#define PERIPHERAL_BASE     0x3F000000  // RPi3!
#define ARM_LOCAL_BASE      0x40000000  // RPi3!

// CHANGE 2: Mailbox Offsets
#define CORE0_MBOX3_SET     0x80
#define CORE0_MBOX3_CLR     0xC0
#define CORE1_MBOX3_SET     0x90
#define CORE1_MBOX3_CLR     0xD0
#define CORE2_MBOX3_SET     0xA0
#define CORE2_MBOX3_CLR     0xE0
#define CORE3_MBOX3_SET     0xB0
#define CORE3_MBOX3_CLR     0xF0

// CHANGE 3: NO GIC!
// Remove all GIC-related code
// Use ARM Local Interrupts instead

// CHANGE 4: Interrupt Sources
#define CORE0_IRQ_SOURCE    0x60
#define CORE1_IRQ_SOURCE    0x64
#define CORE2_IRQ_SOURCE    0x68
#define CORE3_IRQ_SOURCE    0x6C
```

### Tag 5: irq.c Portierung

**Hauptaufgabe:** GIC-Code durch ARM Local Interrupts ersetzen

**Konzept:**
- RPi4 hat GIC-400 (Advanced Interrupt Controller)
- RPi3 hat simples ARM Local Interrupt System
- Mailbox-Interrupts sind "einfacher" auf RPi3

---

## Tag 6-7: OpenAMP platform_info.c

### Tag 6: Resource Table & Memory Map

**Datei: `apps/machine/raspi3/rsc_table.c`**

```c
#include <openamp/open_amp.h>
#include "rsc_table.h"

#define RPMSG_VRING_ADDR_0      0x20800000
#define RPMSG_VRING_ADDR_1      0x20804000
#define RPMSG_VRING_SIZE        0x4000
#define RPMSG_VRING_ALIGN       0x1000

struct remote_resource_table __attribute__((section(".resource_table"))) resources = {
    .version = 1,
    .num = 2,
    .reserved = {0, 0},
    .offset = {
        offsetof(struct remote_resource_table, rpmsg_vdev),
        offsetof(struct remote_resource_table, rpmsg_vdev_vring0),
    },
    
    .rpmsg_vdev = {
        RSC_VDEV, VIRTIO_ID_RPMSG, 0,
        RSC_VDEV_FEATURE_NS, 0, 0, 0, 2, {0, 0},
    },
    
    .rpmsg_vdev_vring0 = {RPMSG_VRING_ADDR_0, RPMSG_VRING_ALIGN,
                          NUM_RPMSG_BUFF, 0, 0},
    .rpmsg_vdev_vring1 = {RPMSG_VRING_ADDR_1, RPMSG_VRING_ALIGN,
                          NUM_RPMSG_BUFF, 1, 0},
};
```

### Tag 7: platform_info.c Mailbox-Funktionen

```c
#include <metal/sys.h>
#include <metal/device.h>
#include <openamp/remoteproc.h>

#define ARM_LOCAL_BASE      0x40000000
#define CORE0_MBOX3_SET     (ARM_LOCAL_BASE + 0x80)
#define CORE3_IRQ_SOURCE    (ARM_LOCAL_BASE + 0x6C)

// Mailbox "kick" - Signal an anderen Core senden
static int rpi3_proc_kick(struct remoteproc *rproc, uint32_t id) {
    (void)rproc;
    (void)id;
    
    volatile uint32_t *mbox = (volatile uint32_t*)CORE0_MBOX3_SET;
    *mbox = 1;  // Signal an Core 0
    
    asm volatile("sev");  // Send Event
    
    return 0;
}

// Mailbox-Interrupt empfangen
static void rpi3_mbox_irq_handler(void) {
    // Clear Mailbox
    volatile uint32_t *mbox_clr = (volatile uint32_t*)(ARM_LOCAL_BASE + 0xF0);
    *mbox_clr = 0xFFFFFFFF;
    
    // Notify OpenAMP
    rproc_virtio_notified(g_rproc_vdev, RPMSG_VRING_NOTIFY_ID);
}
```

---

## Week 1 Ziele - Checkliste

### Must Have ✅
- [ ] Development Environment aufgesetzt
- [ ] Dokumentation gelesen & verstanden
- [ ] Simple Bare-Metal auf RPi3 läuft (LED blinkt)
- [ ] Linux bootet mit `maxcpus=3`
- [ ] Device Tree Overlay erstellt
- [ ] Core 3 per Mailbox aufweckbar

### Should Have ⭐
- [ ] libmetal sys.c für RPi3 portiert
- [ ] Mailbox read/write funktioniert
- [ ] Memory Map definiert & reserviert

### Nice to Have 🎯
- [ ] Resource Table erstellt
- [ ] OpenAMP platform_info.c angefangen
- [ ] Erste IPC-Tests geplant

---

## Debugging-Tips

### Common Issues

**Problem 1: RPi3 bootet nicht**
```
Check:
- config.txt korrekt? (arm_64bit=1)
- kernel8.img an richtiger Stelle?
- UART-Kabel angeschlossen? (Debug-Output!)
```

**Problem 2: ACT LED blinkt nicht**
```
Check:
- GPIO 47 korrekt? (RPi3 vs RPi3+!)
- Active HIGH/LOW? (RPi3 ist Active HIGH!)
- Linker Script: Code an 0x80000?
```

**Problem 3: Core 3 wacht nicht auf**
```
Check:
- Mailbox-Adresse korrekt? (0x400000B0)
- "sev" instruction ausgeführt?
- Core 3 in WFE loop?
```

### UART Debug Setup

**Hardware:**
- USB-zu-UART Adapter (FTDI, CP2102)
- Connections:
  - GPIO 14 (TXD0) → RX
  - GPIO 15 (RXD0) → TX
  - GND → GND

**Software:**
```bash
# Linux
screen /dev/ttyUSB0 115200

# oder
minicom -D /dev/ttyUSB0 -b 115200
```

---

## Next Week Preview

### Week 2 Focus
- OpenAMP vollständig integrieren
- RPMsg Channels aufbauen
- FreeRTOS oder Bare-Metal IPC Sample
- Linux Userspace RPMSG Tool
- Erste Ping-Pong Messages!

---

## Resources Quick Links

```
BCM2835/36 Peripherals:
https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf

TImada RPi4 Reference:
https://github.com/TImada/raspi4_freertos_rpmsg

OpenAMP Docs:
https://openamp.readthedocs.io/

RPi3 Bare-Metal Tutorial:
https://github.com/bztsrc/raspi3-tutorial
```

---

## Daily Log Template

```markdown
# Day X - [Date]

## Goals
- [ ] Goal 1
- [ ] Goal 2

## Achievements
- ✅ What worked
- ❌ What failed
- 📝 Learnings

## Issues Encountered
1. Issue description
   - Tried solution X
   - Result: ...

## Tomorrow
- Next steps
- Questions to research
```

---

**VIEL ERFOLG! Du packst das! 🚀**

Bei Problemen: Dokumentiere alles & frag mich!
