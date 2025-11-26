# ✅ Erfolgreiche Tests - Status Log

Dokumentation aller erfolgreichen Meilensteine im RPi3 Bare-Metal Projekt.

---

## 🎉 Meilenstein 1: UART0 Bare-Metal Test - ERFOLGREICH!

**Datum:** 2025-11-23
**Status:** ✅ **FUNKTIONIERT**

### Was getestet wurde:
- Bare-Metal Code auf Raspberry Pi 3 Model B
- UART0 Kommunikation (GPIO 14/15)
- Boot von SD-Karte
- ARM64 Assembler Boot-Code
- C-Code Ausführung

### Hardware Setup:
```
Raspberry Pi 3 Model B
├─ GPIO 14 (TX, Pin 8)  -> UART Adapter RX
├─ GPIO 15 (RX, Pin 10) -> UART Adapter TX
└─ GND (Pin 6)          -> UART Adapter GND

Terminal: screen /dev/ttyUSB0 115200
```

### Konfiguration:
**config.txt:**
```ini
arm_64bit=1
enable_uart=1
```

**Boot-Prozess:**
1. bootcode.bin lädt GPU Firmware
2. start.elf lädt unser kernel8.img (914 Bytes)
3. Code startet bei 0x80000
4. Core 0 läuft, Cores 1-3 werden gehalted
5. UART0 wird initialisiert
6. Messages werden ausgegeben

### Erwartete Ausgabe (bekommen!):
```
*** UART0 Bare-Metal Test ***
GPIO 14/15 (Pins 8/10)
If you see this, UART works!

Message #0
Message #1
Message #2
...
```

### Code-Details:
- **Datei:** `main.c` (UART0 Test)
- **Assembler:** `boot.S` (Core 0 Boot, andere Cores halt)
- **Linker:** `link.ld` (Start bei 0x80000)
- **Build:** `make` produziert `kernel8.img` (914 Bytes)

### Wichtige Erkenntnisse:
1. ✅ **Hardware funktioniert** - Raspberry Pi, UART-Adapter, Verkabelung
2. ✅ **Build-System funktioniert** - Toolchain, Makefile, Linker-Script
3. ✅ **Peripherals funktionieren** - GPIO, UART0 (PL011)
4. ✅ **Adressen korrekt** - PERIPHERAL_BASE 0x3F000000 (RPi3 BCM2837)
5. ⚠️ **Linux bootet nicht** - erwartet, weil kernel8.img ersetzt wurde

### Peripherals getestet:
- ✅ GPIO (Pin-Konfiguration, ALT0 Funktionen)
- ✅ UART0 PL011 (Baudrate, Line Control, TX/RX)
- ✅ Memory-Mapped I/O (volatile Pointer)

### Nächste Schritte:
1. Linux wiederherstellen (kernel8.img.linux_backup)
2. AMP-Setup: Linux + Bare-Metal parallel
3. Device Tree für Memory Reservation
4. maxcpus=3 für Core 3 Freigabe
5. Core 3 per Mailbox aufwecken
6. OpenAMP Integration

---

## 🚧 In Arbeit: AMP-Setup (Linux + Core 3 Bare-Metal)

**Ziel:**
- Linux auf Cores 0-2
- Bare-Metal auf Core 3
- Inter-Core Kommunikation via Mailboxes

**Status:** Noch nicht begonnen

---

## 📊 Hardware-Kompatibilität

| Hardware | Status | Notizen |
|----------|--------|---------|
| Raspberry Pi 3 Model B | ✅ | BCM2837, Cortex-A53 |
| UART0 (GPIO 14/15) | ✅ | PL011, 115200 Baud |
| UART2 (GPIO 0/1) | ⚠️ | Nicht getestet (aber sollte funktionieren) |
| GPIO Control | ✅ | GPFSEL, GPPUD funktionieren |
| SD Card Boot | ✅ | kernel8.img wird geladen |
| ARM64 Mode | ✅ | arm_64bit=1 funktioniert |

---

## 🔧 Bekannte Probleme

### Problem: ACT LED nicht direkt steuerbar
**Details:** RPi3 Model B nutzt I2C GPIO Expander für ACT LED, nicht direkt GPIO 47
**Lösung:** Externe LED auf GPIO 17 verwenden, oder GPU Mailbox API nutzen
**Status:** Dokumentiert in CLAUDE.md

### Problem: Linux bootet nicht mit unserem kernel8.img
**Details:** Bare-Metal Test ersetzt Linux-Kernel
**Lösung:** Für Tests OK, für AMP: anderen Boot-Mechanismus nutzen
**Status:** Erwartet, kein Bug

---

## 📚 Referenzen

- BCM2837 Peripherals: 0x3F000000 (nicht 0xFE000000 wie RPi4!)
- ARM Local Peripherals: 0x40000000 (Mailboxes!)
- UART0 Base: 0x3F201000
- UART2 Base: 0x3F201400
- GPIO Base: 0x3F200000

---

**Letzte Aktualisierung:** 2025-11-23
**Nächstes Ziel:** AMP-Setup mit Linux + Core 3 Bare-Metal
