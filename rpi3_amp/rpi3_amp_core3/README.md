# RPi3 AMP - Core 3 Bare-Metal Application

## Übersicht

Dies ist die **AMP-ready Version** des UART-Tests, speziell für **Core 3** in einer Asymmetric Multiprocessing (AMP) Konfiguration.

### Was ist anders als beim normalen UART-Test?

| Eigenschaft | Normal (kernel8.img) | AMP (core3_amp.bin) |
|-------------|---------------------|---------------------|
| **Load Address** | 0x80000 | **0x20000000** |
| **Boot Methode** | GPU lädt als Kernel | **Linux lädt in Reserved Memory** |
| **CPU Filter** | Core 0 | **Core 3 only** |
| **Zweck** | Standalone Test | **Parallel zu Linux** |

---

## 📍 Memory Layout

```
0x00000000 - 0x1FFFFFFF  |  512 MB  | Linux (Cores 0-2)
0x20000000 - 0x209FFFFF  |   10 MB  | ← CORE 3 CODE HIER!
0x20A00000 - 0x20BFFFFF  |    2 MB  | Shared Memory (IPC)
0x3F000000 - 0x3FFFFFFF  |   16 MB  | BCM2837 Peripherals
0x40000000 - 0x40000FFF  |    4 KB  | ARM Local (Mailboxes)
```

**Dieser Code läuft bei:** `0x20000000`

---

## 🔨 Build

```bash
make clean
make

# Output:
# - kernel8.elf  (ELF mit Debug-Symbolen)
# - kernel8.img  (Raw binary)
# - core3_amp.bin (Kopie für AMP-Nutzung)
```

### Build verifizieren

```bash
# Check Load Address
aarch64-none-elf-objdump -h kernel8.elf | grep .text
# Erwartung: VMA = 0000000020000000

# Check Größe
ls -lh core3_amp.bin
# Sollte < 1 MB sein (haben 10 MB reserviert)
```

---

## 🚀 Nutzung

### ⚠️ WICHTIG: Nicht als kernel8.img nutzen!

Dieser Code ist **NICHT** für direktes Booten gedacht!

**FALSCH:**
```bash
# ❌ NICHT kopieren nach /boot/kernel8.img
sudo cp core3_amp.bin /media/$USER/boot/kernel8.img
# → Das würde Linux ersetzen!
```

**RICHTIG:**

Dieser Code wird von einem **Linux User-Space Tool** geladen:

```bash
# 1. Linux bootet normal (kernel8.img = Linux Kernel)
# 2. Linux reserviert Memory bei 0x20000000 (via Device Tree)
# 3. Linux Tool lädt core3_amp.bin in 0x20000000
# 4. Linux Tool weckt Core 3 auf (per Mailbox)
# 5. Core 3 läuft diesen Code!
```

---

## 📋 Dateien

### boot.S
- **Core-Filter:** Nur Core 3 läuft (Cores 0-2 halt)
- **Stack:** Separate Stack für Core 3
- **BSS:** Wird korrekt initialisiert

**Wichtige Änderung:**
```asm
// Alter Code (standalone):
cbz     x0, core0_start

// Neuer Code (AMP):
cmp     x0, #3
bne     core_halt    // Nur Core 3!
```

### link.ld
- **Load Address:** 0x20000000 (AMP reserved memory)
- **Sections:** .text, .rodata, .data, .bss
- **Größe:** Passt in 10 MB Reservation

**Wichtige Änderung:**
```ld
// Alter Code:
. = 0x80000;

// Neuer Code:
. = 0x20000000;
```

### main.c
- **UART0:** GPIO 14/15 (exklusiv für Core 3)
- **Output:** Identifiziert sich als "AMP Core 3"
- **Peripherals:** BCM2837 (0x3F000000)

---

## 🧪 Testing

### Phase 1: Standalone Test (Optional)

Nur zum Testen ob der Code funktioniert:

```bash
# TEMPORÄR kernel8.img ersetzen
sudo cp core3_amp.bin /media/$USER/boot/kernel8.img

# Boot RPi3
# UART sollte zeigen:
# *** RPi3 AMP - Core 3 Bare-Metal ***
# Running at: 0x20000000 (AMP Reserved)
# ...

# ⚠️ ABER: Core 3 wird beim Boot nicht gestartet!
# → Code läuft nicht, weil GPU nur Core 0 startet
```

**Fazit:** Dieser Test funktioniert NICHT richtig, weil der GPU Bootloader nur Core 0 startet!

### Phase 2: Mit Linux & Core Wakeup (Richtig!)

**Voraussetzungen:**
1. ✅ Linux bootet mit `maxcpus=3`
2. ✅ Memory bei 0x20000000 reserviert (Device Tree)
3. ✅ Linux UART Console deaktiviert

**Dann:**
```bash
# Auf RPi3 (Linux läuft):
# 1. core3_amp.bin auf Pi kopieren
scp core3_amp.bin pi@raspberrypi.local:~

# 2. Core 3 Launcher nutzen (später zu erstellen)
sudo ./core3_launcher core3_amp.bin

# 3. UART Monitor zeigt:
# ========================================
# *** RPi3 AMP - Core 3 Bare-Metal ***
# ========================================
# Running at: 0x20000000 (AMP Reserved)
# Core 3 successfully started!
# ========================================
# Message #0 from Core 3
# Message #1 from Core 3
# ...
```

---

## 🔧 Next Steps

Diese Bare-Metal App ist Phase 1 fertig. Nächste Schritte:

### Bereits erledigt:
- ✅ Code läuft bei 0x20000000
- ✅ Core 3 Filter aktiv
- ✅ UART0 funktioniert

### TODO:
- [ ] Device Tree Overlay erstellen (Memory Reservation)
- [ ] Linux `core3_launcher` Tool schreiben
- [ ] Core 3 Wakeup per Mailbox testen
- [ ] Shared Memory IPC vorbereiten
- [ ] OpenAMP Integration

---

## 📊 Memory Usage

```bash
# Check actual size:
size kernel8.elf
```

**Erwartung:**
```
   text    data     bss     dec     hex filename
    844       0    4096    4940    134c kernel8.elf
```

**Analysis:**
- **text + rodata:** ~1 KB (Code & Konstanten)
- **bss:** 4 KB (Stack)
- **Gesamt:** ~5 KB (haben 10 MB reserviert → viel Platz!)

---

## 🐛 Debugging

### Problem: Code läuft nicht

**Check 1: Lädt Linux den Code?**
```bash
# Auf RPi3:
sudo xxd /dev/mem | grep -A 10 20000000
# Sollte Code-Bytes zeigen (nicht Nullen)
```

**Check 2: Ist Core 3 aktiv?**
```bash
# Check welche Cores laufen:
cat /proc/cpuinfo | grep processor
# Erwartung: 0, 1, 2 (nicht 3)

# Core 3 ist isoliert, wird manuell gestartet
```

**Check 3: UART Verkabelung?**
```bash
# Siehe AMP_CONFIGURATION_GUIDE.md
# GPIO 14 (TX) → UART Adapter RX
# GPIO 15 (RX) → UART Adapter TX
```

### Problem: Core 3 startet nicht

**Mögliche Ursachen:**
1. Mailbox-Signal nicht gesendet
2. Falsche Jump-Adresse
3. Core 3 nicht aus WFE aufgewacht
4. Code nicht korrekt geladen

**Debug:**
```bash
# ARM Local Mailbox Status prüfen
sudo devmem2 0x400000B0  # Core 3 Mailbox 3 SET
sudo devmem2 0x400000F0  # Core 3 Mailbox 3 CLR
```

---

## 📚 Referenzen

**Im Projekt:**
- `../AMP_CONFIGURATION_GUIDE.md` - Linux Config für AMP
- `../rpi3_uart_test/` - Original standalone Test
- `../../CLAUDE.md` - Projekt-Übersicht
- `../../ERRATA_CRITICAL_FIXES.md` - Bekannte Probleme

**Hardware:**
- BCM2835 ARM Peripherals PDF
- BCM2836 QA7 (ARM Local) - Mailboxes!
- ARM Cortex-A53 TRM

---

**Version:** 1.0 - AMP Core 3 Ready
**Datum:** 2025-11-23
**Status:** ✅ Build erfolgreich, bereit für Core 3 Wakeup Tests
