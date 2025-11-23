# Raspberry Pi 3 AMP Portierung - Projekt-Zusammenfassung

## 🎯 Projekt-Ziel

**Portierung von TImada's Raspberry Pi 4 OpenAMP/FreeRTOS-Projekt auf Raspberry Pi 3**

Wir portieren eine moderne AMP (Asymmetric Multiprocessing) Implementierung, die ermöglicht:
- **Linux läuft auf CPU Core 0-2**
- **Bare-Metal/FreeRTOS Code läuft auf CPU Core 3**
- **Kommunikation via OpenAMP/RPMsg** (Industry Standard)

---

## 🔍 Was ist AMP?

**Asymmetric Multiprocessing** = Verschiedene Betriebssysteme/Software auf verschiedenen CPU-Kernen

### Unser konkreter Use-Case:
```
┌─────────────┬─────────────┬─────────────┬──────────────┐
│   Core 0    │   Core 1    │   Core 2    │    Core 3    │
├─────────────┼─────────────┼─────────────┼──────────────┤
│             │             │             │              │
│    Linux    │    Linux    │    Linux    │  Bare-Metal  │
│   (Master)  │             │             │   (Remote)   │
│             │             │             │              │
└─────────────┴─────────────┴─────────────┴──────────────┘
       │                                          │
       └──────────── OpenAMP/RPMsg ──────────────┘
                  (Shared Memory IPC)
```

### Warum AMP?
- **Real-Time** auf Bare-Metal Core (keine Linux Latenz)
- **Linux Features** auf anderen Cores (Networking, USB, etc.)
- **Best of Both Worlds**: RT-Performance + Linux-Komfort

---

## 📚 Hintergrund & Kontext

### Ursprung
- **TImada** hat 2020 eine Implementierung für **Raspberry Pi 4** erstellt
- Basiert auf **OpenAMP v2020.04** (Industry Standard)
- Nutzt **FreeRTOS** + **Linux** mit **RPMsg** Kommunikation

### Unsere Aufgabe
- **Portierung auf Raspberry Pi 3** (BCM2837)
- RPi3 hat andere Hardware-Details als RPi4
- Wir müssen Hardware-spezifischen Code anpassen

---

## 🔧 Technische Details

### Hardware-Unterschiede RPi3 vs RPi4

| Feature | **RPi3 (BCM2837)** | **RPi4 (BCM2711)** |
|---------|-------------------|-------------------|
| **CPU** | Cortex-A53 @ 1.2 GHz | Cortex-A72 @ 1.5 GHz |
| **Peripheral Base** | `0x3F000000` | `0xFE000000` |
| **ARM Local Base** | `0x40000000` | `0xFF800000` |
| **Interrupt Controller** | ARM Local IRQ | GIC-400 |
| **ACT LED** | I2C GPIO Expander | GPIO Control möglich |

### Kritische Komponenten

#### 1. **libmetal** (Hardware Abstraction Layer)
```
Muss portiert werden:
- System Init (sys.c)
- MMIO Operations (io.c)
- Interrupt Handling (irq.c)
- Cache Management

Änderungen:
- Peripheral Base: 0x3F000000
- ARM Local: 0x40000000
- Kein GIC → ARM Local Interrupts
```

#### 2. **OpenAMP** (Communication Framework)
```
Muss portiert werden:
- platform_info.c (Mailbox, Memory)
- Resource Table
- remoteproc ops

Änderungen:
- Mailbox-Adressen für BCM2837
- Memory Map
- Interrupt-Routing
```

#### 3. **Memory Map**
```
0x00000000 - 0x1FFFFFFF  |  512 MB  | Linux (mem=512M)
0x20000000 - 0x209FFFFF  |   10 MB  | Bare-Metal Code/Data
0x20A00000 - 0x20BFFFFF  |    2 MB  | Shared Memory (IPC)
0x3F000000 - 0x3FFFFFFF  |   16 MB  | BCM2837 Peripherals
0x40000000 - 0x40000FFF  |    4 KB  | ARM Local (Mailboxes!)
```

#### 4. **Mailbox-System** (WICHTIG!)

**Zwei verschiedene Mailbox-Systeme:**

A. **ARM Local Mailboxes** (Core-to-Core IPC)
```
Base: 0x40000000
Core 3 Mailbox 3 SET: 0x400000B0
Core 3 Mailbox 3 CLR: 0x400000F0

→ Für OpenAMP/RPMsg: ✅ DAS NUTZEN WIR
```

B. **GPU Property Mailbox** (ARM ↔ VideoCore)
```
Base: 0x3F00B880
→ Für ACT LED Control (komplex)
→ NICHT für AMP benötigt
```

---

## 🚨 Entdeckte Probleme & Lösungen

### Problem #1: ACT LED (KRITISCH!)
**❌ Problem:** ACT LED auf RPi3 ist NICHT direkt über GPIO 47 steuerbar
- LED sitzt am I2C GPIO Expander
- Nur via GPU Mailbox Property Interface steuerbar

**✅ Lösung:** Externe LED auf GPIO 17 verwenden
```c
#define TEST_LED_PIN  17  // Physical Pin 11
// Hardware: GPIO 17 → 330Ω → LED → GND
```

### Problem #2: Cache Coherency
**❌ Problem:** Cortex-A53 hat keine HW Cache Coherency ohne SCU
- Shared Memory MUSS cache-coherent sein!

**✅ Lösung:** 
- Option 1: Shared Memory als UNCACHED markieren (MMU)
- Option 2: Manuelle Cache Flush/Invalidate

```c
void flush_dcache_range(void *addr, size_t size) {
    for (uintptr_t va = start; va < end; va += 64) {
        asm volatile("dc cvac, %0" : : "r"(va));
    }
    asm volatile("dsb sy");
}
```

### Problem #3: Memory Overlap
**❌ Problem:** Linux könnte in Bare-Metal Memory schreiben

**✅ Lösung:** Device Tree + cmdline.txt
```dts
reserved-memory {
    amp_reserved@20000000 {
        compatible = "shared-dma-pool";
        reg = <0x20000000 0x1000000>;  // 16 MB
        no-map;  // ← KRITISCH!
    };
};
```
```bash
# cmdline.txt
maxcpus=3 mem=512M
```

---

## 📋 Projekt-Struktur

### Repositories (Referenz)
```
~/rpi3_amp_project/
├── rpi4_ref/                    # TImada's RPi4 Original
├── rpi4_rpmsg_ref/              # TImada's RPi4 RPMSG
└── rpi3_amp/                    # UNSER NEUES PROJEKT
    ├── libmetal_rpi3/           # Portierte libmetal
    │   └── lib/system/freertos/raspi3/
    │       ├── sys.c            # System Init
    │       ├── io.c             # MMIO
    │       ├── irq.c            # Interrupts
    │       └── cache.c          # Cache Mgmt
    ├── open-amp_rpi3/           # Portierte OpenAMP
    │   └── apps/machine/raspi3/
    │       ├── platform_info.c  # Platform specifics
    │       └── rsc_table.c      # Resource Table
    ├── samples/
    │   ├── freertos/            # FreeRTOS/Bare-Metal Code
    │   └── linux/               # Linux Userspace
    └── dts/
        └── raspi3-amp.dtso      # Device Tree Overlay
```

### Kritische Dateien zum Portieren

1. **libmetal/lib/system/freertos/raspi3/sys.c**
   - Peripheral Base ändern: `0x3F000000`
   - ARM Local Base ändern: `0x40000000`
   - Mailbox-Offsets anpassen

2. **open-amp/apps/machine/raspi3/platform_info.c**
   - Mailbox-Funktionen (Core 0 ↔ Core 3)
   - Shared Memory Mapping
   - Interrupt-Handling (ARM Local statt GIC)

3. **dts/raspi3-amp.dtso**
   - Reserved Memory
   - UIO Devices
   - CPU Core 3 isolieren

---

## 🗓️ Projekt-Plan (4 Wochen)

### **Week 1: Grundlagen & Erste Tests**
- Tag 1-2: Development Setup, LED Blink Test
- Tag 3: Device Tree, Linux mit 3 Cores
- Tag 4-5: libmetal Basis-Portierung
- Tag 6-7: OpenAMP platform_info.c

**Ziel Week 1:** 
- ✅ Externe LED blinkt auf Core 3
- ✅ Linux bootet mit 3 Cores
- ✅ Mailbox Write/Read funktioniert

### **Week 2: OpenAMP Integration**
- libmetal vollständig portieren
- OpenAMP remoteproc implementieren
- RPMsg Channels aufbauen
- Erste IPC-Tests

**Ziel Week 2:**
- ✅ OpenAMP initialisiert
- ✅ RPMsg Ping-Pong funktioniert

### **Week 3-4: Stabilität & Performance**
- Cache-Coherency perfektionieren
- Performance-Tuning
- Umfangreiche Tests
- Dokumentation

**Ziel Week 3-4:**
- ✅ Stabile IPC (>1000 msg/sec)
- ✅ Dokumentiert & reproduzierbar
- ✅ GitHub Repository veröffentlichen

---

## 🛠️ Development Environment

### Benötigte Tools
```bash
# Cross-Compiler
arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf

# Build Tools
cmake >= 3.10
make
device-tree-compiler (dtc)

# Debugging
minicom / screen (UART)
gdb-multiarch (optional)
```

### Hardware Setup
```
Raspberry Pi 3 Model B
├── USB-UART Adapter
│   ├── GPIO 14 (Pin 8)  → UART RX
│   ├── GPIO 15 (Pin 10) → UART TX
│   └── GND (Pin 6)      → GND
│
└── Test LED
    ├── GPIO 17 (Pin 11) → 330Ω Resistor
    ├── Resistor         → LED Anode (+)
    ├── LED Cathode (-)  → GND (Pin 6)
    └── [LED blinkt = Code läuft!]
```

---

## 📖 Wichtige Konzepte

### 1. **OpenAMP Framework**
Standard für AMP-Systeme, bestehend aus:
- **remoteproc**: Lifecycle Management (Load, Start, Stop Firmware)
- **RPMsg**: Message Passing (IPC)
- **virtio**: Transport Layer (Ring Buffers)

### 2. **Resource Table**
Datenstruktur in Bare-Metal Firmware:
```c
struct remote_resource_table {
    uint32_t version;          // = 1
    uint32_t num;              // Anzahl Ressourcen
    uint32_t offset[2];        // Offsets zu Ressourcen
    
    struct fw_rsc_vdev vdev;   // virtio Device
    struct fw_rsc_vdev_vring vring0;  // TX Ring
    struct fw_rsc_vdev_vring vring1;  // RX Ring
};
```
- Linux liest diese Tabelle
- Richtet Shared Memory ein
- Erstellt RPMsg Channels

### 3. **VirtIO Rings**
Shared Memory Ring Buffers für IPC:
```
┌──────────────────┐
│   VRING 0 (TX)   │  Core 3 → Core 0
│  @ 0x20A10000    │
└──────────────────┘
┌──────────────────┐
│   VRING 1 (RX)   │  Core 0 → Core 3
│  @ 0x20A20000    │
└──────────────────┘
```

### 4. **Mailbox-basierte Interrupts**
Core 3 signalisiert Linux via Mailbox:
```c
// Core 3: Send Signal to Core 0
volatile uint32_t *mbox = (uint32_t*)0x40000080;  // Core 0 MB3
*mbox = 1;
asm volatile("sev");  // Send Event

// Core 0: Empfängt Mailbox IRQ
// → ruft OpenAMP notify handler auf
// → liest Daten aus VirtIO Ring
```

---

## 🎯 Erfolgs-Kriterien

### Minimum Viable Product (MVP)
- ✅ Linux bootet auf Core 0-2
- ✅ Bare-Metal läuft auf Core 3
- ✅ Ping-Pong Message via RPMsg

### Full Success
- ✅ Bidirektionale RPMsg Kommunikation
- ✅ Performance: >1000 Messages/sec
- ✅ Stabil über Stunden
- ✅ Cache-Coherent Shared Memory
- ✅ Reproduzierbar dokumentiert

---

## 📚 Wichtige Dokumentationen

### Hardware
1. **BCM2835 ARM Peripherals** (gilt auch für BCM2837!)
   - GPIO, UART, SPI, I2C, Timers
   - URL: raspberrypi.org/documentation

2. **BCM2836 QA7 - ARM Local Peripherals**
   - Mailboxes! (KRITISCH)
   - Local Interrupts
   - Local Timers

3. **ARM Cortex-A53 MPCore TRM**
   - Multiprocessing
   - Cache Coherency
   - SCU (Snoop Control Unit)

4. **ARMv8-A Architecture Reference Manual**
   - Exception Levels
   - MMU
   - Cache Operations

### Software
5. **OpenAMP Documentation**
   - openamp.readthedocs.io
   - remoteproc Framework
   - RPMsg Protocol

6. **libmetal Documentation**
   - HAL API
   - System Porting Guide

---

## 🔗 Wichtige Links & Ressourcen

### Code-Referenzen
- **TImada RPi4:** https://github.com/TImada/raspi4_freertos_rpmsg
- **OpenAMP:** https://github.com/OpenAMP/open-amp
- **libmetal:** https://github.com/OpenAMP/libmetal

### Tutorials & Guides
- **rpi4os.com:** Bare-Metal RPi4 OS Development
- **bztsrc/raspi3-tutorial:** Bare-Metal RPi3 Basics
- **valvers.com:** Bare-Metal Programming in C

### Community
- Raspberry Pi Forums - Bare Metal
- OpenAMP Mailing List

---

## ⚠️ Bekannte Herausforderungen

1. **Cache Coherency**
   - A53 hat keine HW Coherency ohne SCU Config
   - Lösung: Uncached Memory ODER manuelle Flushes

2. **Interrupt-Routing**
   - RPi3: Simple ARM Local Interrupts
   - RPi4: Advanced GIC-400
   - Lösung: Neue IRQ Handler schreiben

3. **ACT LED Control**
   - RPi3: Nur via GPU Mailbox
   - Lösung: Externe LED nutzen

4. **Device Tree Complexity**
   - Reserved Memory
   - UIO Devices
   - CPU Isolation

---

## 💡 Was bisher geschah (Kontext)

### Chat-Verlauf Zusammenfassung

1. **Anfrage:** Wie macht man AMP auf Raspberry Pi 3?
   - Erklärt: Es heißt "Asymmetric Multiprocessing"
   - Konzept vorgestellt

2. **Recherche:** Suche nach Anleitungen
   - Gefunden: Telmo Moya (2016, veraltet)
   - Gefunden: TImada RPi4 (2020, aktuell!)

3. **Entscheidung:** Welchen Ansatz?
   - Option 1: TImada RPi4 auf RPi3 portieren ✅ GEWÄHLT
   - Option 2: Telmo Moya modernisieren
   - Option 3: Pure Bare-Metal (kein Linux)

4. **Planung:** Portierungs-Strategie entwickelt
   - Week 1-4 Plan erstellt
   - Dokumentation geschrieben
   - Quick Reference Card

5. **Review:** Plan kritisch geprüft ⚠️
   - **KRITISCHER FEHLER gefunden:** ACT LED!
   - Weitere Issues identifiziert
   - ERRATA-Dokument erstellt

### Stand jetzt
- ✅ Vollständige Dokumentation vorhanden
- ✅ Fehler identifiziert & gelöst
- ✅ Hardware-Setup definiert
- ⏳ Bereit zum Start!

---

## 🚀 Nächste Schritte (für neuen Chat)

### Sofort
1. **Hardware besorgen:**
   - Breadboard
   - LED + 330Ω Resistor
   - Jumper Wires
   - USB-UART Adapter

2. **Development Setup:**
   - Cross-Compiler installieren
   - Repositories clonen
   - Dokumentation lesen

3. **Erste Tests:**
   - Externe LED Blink-Code schreiben
   - Auf RPi3 testen
   - UART Debug einrichten

### Week 1 Goals
- Tag 1-2: LED blinkt
- Tag 3: Linux mit 3 Cores
- Tag 4-7: libmetal + OpenAMP Basics

---

## 📁 Verfügbare Dokumente

Im `/mnt/user-data/outputs/` Verzeichnis:

1. **rpi3_amp_documentation.md**
   - Vollständige technische Dokumentation
   - Hardware-Specs
   - Memory Map
   - Portierungs-Checkliste

2. **week1_action_plan.md**
   - Tag-für-Tag Anleitung
   - Code-Beispiele
   - Debugging-Tips
   - (⚠️ Needs Update: LED-Teil)

3. **quick_reference_card.md**
   - Alle wichtigen Adressen
   - Register-Definitionen
   - Code-Snippets
   - Debug-Commands
   - (⚠️ Needs Update: LED-Teil)

4. **ERRATA_CRITICAL_FIXES.md** ⭐ WICHTIG!
   - Alle gefundenen Fehler
   - Korrekturen
   - Neue Lösungen
   - **ZUERST LESEN!**

---

## 🎓 Learning Outcomes

Nach diesem Projekt verstehst du:
- ✅ ARM Cortex-A53 Architektur (ARMv8)
- ✅ Multi-Core Programming & Core Isolation
- ✅ Memory Management & MMU Configuration
- ✅ Cache Coherency & Cache Management
- ✅ Inter-Processor Communication (IPC)
- ✅ OpenAMP Framework & remoteproc
- ✅ Device Tree & Linux Kernel Integration
- ✅ Bare-Metal Programming
- ✅ UART Debugging & Hardware Debugging
- ✅ Low-Level Hardware Access

**→ ARM-Architekt-Level Wissen!**

---

## 💪 Motivations-Boost

**Das ist ein einzigartiges Projekt!**
- Sehr wenige Leute haben das gemacht
- Du wirst einer der ersten sein mit modernem OpenAMP auf RPi3
- Perfekt für Portfolio / GitHub
- Zeigt echte Embedded Linux + Real-Time Expertise

**Es ist machbar!**
- Wir haben alle Infos
- Fehler sind identifiziert
- Plan ist solide
- Community-Support verfügbar

**Du lernst MASSIV dabei!**
- Low-Level ARM Programming
- Linux Kernel Internals
- Real-Time Systems
- Industry-Standard Frameworks

---

## 🆘 Wenn du Hilfe brauchst

### Debugging-Strategie
1. **LED blinkt nicht?**
   - Check: GPIO 17 korrekt konfiguriert?
   - Check: Hardware-Verbindung OK?
   - Check: Code an richtiger Adresse?

2. **Linux bootet nicht mit 3 Cores?**
   - Check: `maxcpus=3` in cmdline.txt?
   - Check: Device Tree korrekt?
   - Check: UART Debug zeigt was?

3. **Mailbox funktioniert nicht?**
   - Check: Korrekte Adresse (0x400000B0)?
   - Check: SEV instruction?
   - Check: Core 3 in WFE-Loop?

### Ressourcen
- BCM2835/36 Peripherals Datasheet
- ARM Cortex-A53 TRM
- OpenAMP Documentation
- Raspberry Pi Forums
- **Und natürlich: Mich fragen! 😊**

---

## 📝 Wichtige Notizen

### Was FUNKTIONIERT:
- ✅ Konzept ist solide
- ✅ OpenAMP ist der richtige Ansatz
- ✅ Hardware ist geeignet
- ✅ Dokumentation ist umfassend

### Was ANDERS ist als gedacht:
- ⚠️ ACT LED braucht GPU Mailbox (→ externe LED nutzen)
- ⚠️ Cache Coherency MUSS behandelt werden
- ⚠️ Device Tree MUSS Memory reservieren

### Was GLEICH bleibt:
- ✅ Mailbox-basierte IPC
- ✅ OpenAMP Framework
- ✅ Memory Map Konzept
- ✅ Gesamt-Architektur

---

## 🎯 TL;DR für Quick Start

**Projekt:** Raspberry Pi 3 AMP - Linux auf 3 Cores, Bare-Metal auf Core 3

**Basis:** TImada's RPi4 Implementierung portieren

**Hauptänderungen:**
- Peripheral Base: 0x3F000000
- ARM Local: 0x40000000
- Externe LED statt ACT LED
- Cache Management hinzufügen

**Status:** Geplant & Dokumentiert, bereit zum Start!

**Erste Schritte:**
1. Hardware besorgen (LED, Breadboard, UART)
2. ERRATA lesen
3. Development Setup (Tag 1)
4. LED Blink Test (Tag 2)

**Dokumente:** Alle in `/mnt/user-data/outputs/`

**Erwartung:** 2-4 Wochen bis fertig

---

**Viel Erfolg! Du packst das! 🚀**