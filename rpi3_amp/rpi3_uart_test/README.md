# RPi3 Bare-Metal UART Test

## ✅ **STATUS: UART0 Test ERFOLGREICH!**

Bare-Metal Code läuft auf Raspberry Pi 3 und kommuniziert via UART! 🎉

---

## 🎯 Ziel

Teste ob Bare-Metal Code auf Raspberry Pi 3 läuft und UART-Kommunikation funktioniert.

**Aktueller Test: UART0 (GPIO 14/15)** - ✅ **FUNKTIONIERT!**
- Nutzt Linux Console Pins (GPIO 14/15)
- Einfach zu testen
- Kabel vermutlich schon angeschlossen

**Alternative Tests verfügbar in `examples/`:**
- UART2 (GPIO 0/1) - noch nicht getestet
- LED Blink (GPIO 17) - visueller Test
- Minimal Boot - absolut minimal

## 🔌 Hardware Setup (UART0 - Aktueller Test)

### Pin-Belegung UART0

```
UART0 ist auf GPIO 14 und 15:

GPIO 14 (UART0 TX)  -> Physical Pin 8
GPIO 15 (UART0 RX)  -> Physical Pin 10
GND                 -> Physical Pin 6 (oder 9, 14, 20, 25, 30, 34, 39)
```

### UART-to-USB Adapter Verbindung

```
Raspberry Pi 3          UART-to-USB Adapter
--------------------------------------------
Pin 8  (GPIO 14, TX) -> RX (gekreuzt!)
Pin 10 (GPIO 15, RX) -> TX (gekreuzt!)
Pin 6  (GND)         -> GND
```

**WICHTIG:** TX vom Pi geht zu RX vom Adapter (gekreuzt)!

### Terminal-Verbindung

```bash
# Screen (empfohlen)
screen /dev/ttyUSB0 115200

# Oder minicom
minicom -D /dev/ttyUSB0 -b 115200
```

### Pinout-Referenz

```
Raspberry Pi 3 GPIO Header (Top View):

    3V3  (1)  (2)  5V
  GPIO2  (3)  (4)  5V
  GPIO3  (5)  (6)  GND
  GPIO4  (7)  (8)  GPIO14 (UART0 TX - Linux!)
    GND  (9) (10)  GPIO15 (UART0 RX - Linux!)
 GPIO17 (11) (12)  GPIO18
 GPIO27 (13) (14)  GND
 GPIO22 (15) (16)  GPIO23
    3V3 (17) (18)  GPIO24
 GPIO10 (19) (20)  GND
  GPIO9 (21) (22)  GPIO25
 GPIO11 (23) (24)  GPIO8
    GND (25) (26)  GPIO7
  GPIO0 (27) (28)  GPIO1    <- UART2 hier!
  GPIO5 (29) (30)  GND
  GPIO6 (31) (32)  GPIO12
 GPIO13 (33) (34)  GND
 GPIO19 (35) (36)  GPIO16
 GPIO26 (37) (38)  GPIO20
    GND (39) (40)  GPIO21
```

## 🛠️ Build-Anleitung

### Voraussetzungen

```bash
# Cross-Compiler (falls noch nicht installiert)
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf.tar.xz
tar xf arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf.tar.xz
export PATH=$PATH:$PWD/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-elf/bin

# Testen
aarch64-none-elf-gcc --version
```

### Kompilieren

```bash
cd uart2_test
make

# Output sollte sein:
# kernel8.img erstellt
```

### Optional: Disassembly ansehen

```bash
make disasm
less kernel8.list
```

## 💾 SD-Karte vorbereiten

### Option 1: Einfacher Test (nur Bare-Metal, kein Linux)

```bash
# 1. SD-Karte mit Raspberry Pi OS Lite flashen
# 2. Boot-Partition mounten
# 3. Originales kernel8.img sichern
sudo cp /media/$USER/boot/kernel8.img /media/$USER/boot/kernel8.img.backup

# 4. Unser kernel8.img kopieren
sudo cp kernel8.img /media/$USER/boot/

# 5. config.txt anpassen
sudo nano /media/$USER/boot/config.txt
# Füge hinzu:
arm_64bit=1
enable_uart=1

# 6. Sync & Unmount
sudo sync
sudo umount /media/$USER/boot
```

### Option 2: Mit Linux (AMP-Style)

```bash
# In cmdline.txt:
sudo nano /media/$USER/boot/cmdline.txt
# Füge am Ende hinzu:
maxcpus=3

# Das gibt Core 3 für Bare-Metal frei
# (Für später, wenn du Linux + Bare-Metal parallel nutzt)
```

## 🖥️ Terminal-Programm einrichten

### Linux

```bash
# Option 1: screen
screen /dev/ttyUSB0 115200

# Option 2: minicom
minicom -D /dev/ttyUSB0 -b 115200

# Option 3: picocom
picocom -b 115200 /dev/ttyUSB0

# Option 4: Python
python3 -m serial.tools.miniterm /dev/ttyUSB0 115200
```

### Windows

- PuTTY: Serial, COM Port auswählen, 115200 Baud
- TeraTerm: Serial, COM Port, 115200 8N1
- RealTerm

### Terminal-Einstellungen

```
Baudrate:  115200
Data Bits: 8
Parity:    None
Stop Bits: 1
Notation:  115200 8N1
```

## ✅ Erwartete Ausgabe (UART0 Test - ERFOLGREICH!)

Wenn alles funktioniert, siehst du im Terminal:

```
*** UART0 Bare-Metal Test ***
GPIO 14/15 (Pins 8/10)
If you see this, UART works!

Message #0
Message #1
Message #2
Message #3
...
```

**Status:** ✅ **DIESE AUSGABE WURDE ERFOLGREICH GETESTET!**

Jede Sekunde erscheint eine neue Nachricht. Das zeigt, dass:
- Hardware funktioniert ✅
- Build-System funktioniert ✅
- Bare-Metal Code läuft ✅
- UART-Kommunikation funktioniert ✅

## 🐛 Troubleshooting

### Problem: Nichts im Terminal

**Check 1: Hardware**
```bash
# Sind die Kabel richtig?
# TX -> RX gekreuzt?
# GND verbunden?

# USB-Adapter wird erkannt?
dmesg | tail
# Sollte zeigen: ttyUSB0 connected
```

**Check 2: Permissions**
```bash
# User in dialout Gruppe?
sudo usermod -a -G dialout $USER
# Neu einloggen!

# Oder temporär:
sudo chmod 666 /dev/ttyUSB0
```

**Check 3: Terminal-Settings**
```
✅ 115200 Baud
✅ 8N1
✅ Kein Flow Control
✅ Richtiger Port (/dev/ttyUSB0)
```

### Problem: Kryptische Zeichen

**Falsche Baudrate!**
- Terminal: 115200
- Code verwendet: 115200
- Sollte passen

**Check:**
```c
// In main.c:
*UART2_IBRD = 26;   // Für 115200 @ 48MHz
*UART2_FBRD = 3;
```

### Problem: RPi3 bootet nicht

**Check 1: SD-Karte**
```bash
# Sind alle Dateien da?
ls -la /media/$USER/boot/
# Mindestens:
# - bootcode.bin
# - start.elf
# - kernel8.img (unser!)
# - config.txt
```

**Check 2: config.txt**
```ini
# Muss enthalten:
arm_64bit=1
enable_uart=1
```

**Check 3: Richtiger Kernel**
```bash
# Ist es UNSER kernel8.img?
ls -lh /media/$USER/boot/kernel8.img
# Sollte nur ~10-20 KB sein (nicht mehrere MB!)
```

### Problem: "Running on Core: 0" (nicht Core 3)

Das ist OK für den ersten Test! Bedeutet einfach:
- Dein Code läuft auf Core 0
- Für AMP brauchst du später Device Tree und maxcpus=3
- Aber UART2 funktioniert!

## 📊 Was wurde erfolgreich getestet?

1. ✅ **Boot auf ARM64:** Code startet bei 0x80000 in 64-bit Mode
2. ✅ **GPIO Konfiguration:** GPIO 14/15 als UART0 (ALT0) funktioniert
3. ✅ **UART0 Init:** Baudrate 115200, 8N1, TX/RX funktioniert
4. ✅ **Memory-Mapped I/O:** Peripherals bei 0x3F000000 korrekt angesprochen
5. ✅ **String Output:** Formatierte Ausgaben funktionieren
6. ✅ **Timing:** Delay-Loop läuft stabil

## 🚀 Nächste Schritte - AMP Setup

**Aktueller Status:** Bare-Metal Test erfolgreich! ✅

**Nächstes Ziel:** Linux + Bare-Metal parallel laufen lassen (AMP)

### Phase 1: Linux wiederherstellen
1. ✅ Bare-Metal Test funktioniert
2. ⏳ Linux kernel8.img zurückkopieren
3. ⏳ Linux bootet wieder normal

### Phase 2: AMP-Konfiguration
1. ⏳ Device Tree Overlay: Memory für Core 3 reservieren
2. ⏳ `maxcpus=3` in cmdline.txt: Core 3 für Bare-Metal freigeben
3. ⏳ Bare-Metal Code in reservierten Speicher laden (nicht als kernel8.img!)

### Phase 3: Inter-Core Communication
1. ⏳ Mailbox-Test: Core 3 per Mailbox von Linux aufwecken
2. ⏳ Shared Memory Setup: Linux ↔ Core 3
3. ⏳ OpenAMP/RPMsg: IPC Framework integrieren

### Phase 4: Production Setup
1. ⏳ FreeRTOS auf Core 3 portieren
2. ⏳ Anwendungs-Code entwickeln
3. ⏳ Stabilitäts-Tests

## 📚 Technische Details

### UART0 Hardware (aktueller Test)

```c
Base Address: 0x3F201000
Clock:        48 MHz (von VPU)
Type:         ARM PL011
GPIOs:        14 (TX) und 15 (RX)
Alt Mode:     ALT0
```

### UART2 Hardware (alternative, in examples/)

```c
Base Address: 0x3F201400
Clock:        48 MHz (von VPU)
Type:         ARM PL011 (identisch zu UART0)
GPIOs:        0 (TX) und 1 (RX)
Alt Mode:     ALT4
```

### Baudrate-Berechnung

```
Baudrate = UART_CLK / (16 * (IBRD + FBRD/64))
115200   = 48000000 / (16 * (26 + 3/64))
         = 48000000 / (16 * 26.046875)
         = 48000000 / 416.75
         ≈ 115190 (nur 10 Baud Abweichung - OK!)
```

### Memory Map

```
0x00080000 : Code Start (kernel8.img geladen hierhin)
0x3F000000 : BCM2837 Peripherals Base (RPi3!)
0x3F200000 : GPIO Controller
0x3F201000 : UART0 (PL011) - Aktueller Test ✅
0x3F201400 : UART2 (PL011) - Alternative
0x3F215000 : UART1 (Mini UART)
0x40000000 : ARM Local Peripherals (Mailboxes für AMP!)
```

**Wichtig:** RPi3 nutzt `0x3F000000` (nicht `0xFE000000` wie RPi4!)

## 🔗 Referenzen

- BCM2835 ARM Peripherals (gilt auch für BCM2837)
- ARM PL011 Technical Reference Manual
- Raspberry Pi GPIO Pinout: https://pinout.xyz/

## 📝 Lizenz

Dieser Test-Code ist Public Domain. Nutze ihn wie du willst!

---

**Viel Erfolg! 🎉**

Bei Problemen: Überprüfe Hardware-Verbindungen zuerst, dann Terminal-Settings!
