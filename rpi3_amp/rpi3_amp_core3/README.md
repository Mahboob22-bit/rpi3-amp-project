# RPi3 AMP - Core 3 Bare-Metal Firmware

## Übersicht

Dies ist die **modulare Bare-Metal Firmware** für Core 3 in einer Asymmetric Multiprocessing (AMP) Konfiguration auf dem Raspberry Pi 3.

**Status:** ✅ **WORKING** - Shared Memory IPC funktioniert!

### Architektur

```
┌─────────────────────────────────────────────────────────────┐
│                    RPi3 AMP System                          │
├─────────────┬─────────────┬─────────────┬───────────────────┤
│   Core 0    │   Core 1    │   Core 2    │      Core 3       │
│   Linux     │   Linux     │   Linux     │   Bare-Metal FW   │
├─────────────┴─────────────┴─────────────┴───────────────────┤
│                  Shared Memory @ 0x20A00000                 │
│                    (Linux ↔ Core 3 IPC)                     │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 Dateistruktur (Modular)

```
rpi3_amp_core3/
├── boot.S              # Assembly Startup (Core 3 Filter)
├── link.ld             # Linker Script (Load @ 0x20000000)
├── common.h            # Hardware-Adressen, Typen, Makros
├── uart.h / uart.c     # UART0 Treiber mit printf()
├── timer.h / timer.c   # System Timer (echte Zeitstempel)
├── memory.h / memory.c # Shared Memory & Memory Tests
├── cpu_info.h / .c     # CPU Info (derzeit deaktiviert)
├── main.c              # Hauptprogramm mit Heartbeat
├── Makefile            # Build + SSH Deploy
└── core3_amp.bin       # Kompilierte Firmware (~12 KB)
```

### Module

| Modul | Beschreibung |
|-------|--------------|
| **common.h** | Alle Hardware-Adressen (0x3F000000), Typen (uint32_t, etc.), Memory Map |
| **uart** | UART0 auf GPIO 14/15, printf mit %d/%x/%s Support |
| **timer** | System Timer @ 1 MHz, Zeitstempel, Delays |
| **memory** | Shared Memory Status-Struktur, Memory Tests |
| **main** | Initialisierung, Heartbeat-Loop |

---

## 📍 Memory Layout

```
0x00000000 - 0x1FFFFFFF  |  512 MB  | Linux (Cores 0-2)
0x20000000 - 0x209FFFFF  |   10 MB  | ← CORE 3 FIRMWARE HIER!
0x20A00000 - 0x20BFFFFF  |    2 MB  | Shared Memory (IPC)
0x3F000000 - 0x3FFFFFFF  |   16 MB  | BCM2837 Peripherals
0x40000000 - 0x40000FFF  |    4 KB  | ARM Local (Mailboxes)
```

### Shared Memory Layout (0x20A00000)

```
Offset  | Größe  | Beschreibung
--------|--------|------------------
0x0000  | 4 KB   | Status-Struktur
0x1000  | 4 KB   | IPC Daten
0x2000  | 64 KB  | Memory Test Bereich
```

---

## 🔨 Build & Deploy

### Voraussetzungen

```bash
# Cross-Compiler in PATH:
export PATH=$PATH:~/rpi3_amp_project/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf/bin
```

### Build

```bash
make clean && make
```

**Output:**
```
╔════════════════════════════════════════════════════════════════╗
║           BUILD SUCCESSFUL                                     ║
╠════════════════════════════════════════════════════════════════╣
║ Output: core3_amp.bin
║ Size:   12K
╚════════════════════════════════════════════════════════════════╝
```

### Deploy via SSH

```bash
make deploy              # Upload to RPi3
make deploy-reboot       # Upload and reboot
```

### Alle Targets

```bash
make              # Build firmware
make clean        # Remove build files
make deploy       # Deploy via SSH
make deploy-reboot # Deploy and reboot
make disasm       # Create disassembly
make size         # Show section sizes
make help         # Show all targets
```

### Konfiguration

```bash
# Anderer Host:
make deploy RPI_HOST=pi@192.168.1.100

# Anderes Boot-Verzeichnis:
make deploy RPI_BOOT_DIR=/boot
```

---

## 🧪 Features

### 1. ASCII Banner
```
╔════════════════════════════════════════════════════════════╗
║   ██████╗ ██████╗ ██╗██████╗      █████╗ ███╗   ███╗██████╗║
║   ██╔══██╗██╔══██╗██║╚════██╗    ██╔══██╗████╗ ████║██╔══██║
║   ██████╔╝██████╔╝██║ █████╔╝    ███████║██╔████╔██║██████╔║
║   ...
╚════════════════════════════════════════════════════════════╝
```

### 2. Echte Zeitstempel
```
│ Time     : 00:05:23.456
│ Uptime   : 5m 23s
```

### 3. Shared Memory Status
Linux kann jederzeit den Core 3 Status lesen:
```bash
sudo read_shared_mem
# Magic         : 0x52503341 ✓
# State         : RUNNING
# Heartbeat     : 42
```

### 4. Periodischer Heartbeat
Alle 5 Sekunden wird Status auf UART ausgegeben und Shared Memory aktualisiert.

---

## 📋 Shared Memory Status Struktur

```c
typedef struct {
    uint32_t magic;              // 0x52503341 ("RP3A")
    uint32_t version;            // Firmware Version (1.0.0)
    uint32_t core3_state;        // RUNNING, ERROR, etc.
    uint32_t boot_count;         // Anzahl Boots
    uint64_t boot_time;          // Boot-Zeitstempel
    uint64_t uptime_ticks;       // Uptime in µs
    uint32_t heartbeat_counter;  // Heartbeat Zähler
    uint32_t heartbeat_interval_ms;
    uint32_t memtest_status;     // 0=N/A, 1=PASS, 2=FAIL
    uint32_t memtest_errors;
    uint32_t memtest_bytes;
    uint32_t messages_sent;      // IPC Statistik
    uint32_t messages_received;
    uint32_t reserved[8];
    char debug_message[128];     // Debug String
} shared_status_t;
```

---

## 🐛 Bekannte Issues

### 1. CPU Info deaktiviert
**Problem:** `cpu_info.c` verursacht Crash beim Zugriff auf EL1 Register (wir laufen in EL2).

**Workaround:** Modul deaktiviert im Makefile.

**Fix (TODO):** Register-Zugriffe für EL2 anpassen.

### 2. WFE verursacht Crash
**Problem:** `wfe` (Wait For Event) Instruction verursacht Absturz.

**Workaround:** Busy-wait Loop statt WFE:
```c
for (volatile int i = 0; i < 10000; i++) {
    asm volatile("nop");
}
```

**Fix (TODO):** Exception Handler implementieren oder Timer-basiertes Warten.

### 3. Memory Test deaktiviert
**Problem:** Memory Test verursachte Crash im ersten Boot.

**Workaround:** Test deaktiviert.

**Fix (TODO):** Cache-Kohärenz prüfen, Test reaktivieren.

---

## 🔧 Entwicklung

### Neues Modul hinzufügen

1. Header erstellen: `mymodule.h`
2. Implementation: `mymodule.c`
3. In `Makefile` zu `C_SRCS` hinzufügen
4. In `main.c` includieren und nutzen

### Printf Format Strings

```c
uart_printf("Dezimal: %d\n", 42);
uart_printf("Unsigned: %u\n", 42);
uart_printf("Hex: %x\n", 0xDEAD);    // Achtung: gibt 0x... aus
uart_printf("String: %s\n", "Hello");
```

### Hex ohne "0x" Prefix
```c
uart_put_hex32(0x12345678);  // Gibt "0x12345678" aus
uart_put_uint(42);           // Gibt "42" aus
```

---

## 📊 Memory Usage

```bash
make size
```

**Typische Ausgabe:**
```
   text    data     bss     dec     hex filename
  10240       0    4096   14336    3800 kernel8.elf
```

- **text:** ~10 KB (Code + Konstanten)
- **bss:** 4 KB (Stack)
- **Gesamt:** ~14 KB (haben 10 MB reserviert!)

---

## 📚 Referenzen

**Im Projekt:**
- `../../CLAUDE.md` - Projekt-Übersicht
- `../../CURRENT_STATUS.md` - Aktueller Status
- `../../quick_reference_card.md` - Hardware-Adressen
- `../linux_tools/` - Linux Reader Tool

**Hardware:**
- BCM2835 ARM Peripherals PDF (gilt auch für BCM2837)
- BCM2836 QA7 (ARM Local) - Mailboxes!
- ARM Cortex-A53 TRM

---

**Version:** 1.0.0
**Datum:** 2025-11-26
**Status:** ✅ Working - Shared Memory IPC funktioniert!
