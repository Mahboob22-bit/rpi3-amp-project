# RPi3 AMP Configuration Guide

## Ziel
Linux (Cores 0-2) und Bare-Metal (Core 3) parallel betreiben mit korrekter UART-Konfiguration.

---

## 🔧 SD-Karte Konfiguration

### 1. Linux UART Console deaktivieren

**Datei:** `/boot/cmdline.txt` (oder `/boot/firmware/cmdline.txt` bei neueren OS Versionen)

#### ❌ VORHER (Standard):
```
console=serial0,115200 console=tty1 root=PARTUUID=xxxxxxxx-xx rootfstype=ext4 fsck.repair=yes rootwait quiet splash
```

#### ✅ NACHHER (AMP-Ready):
```
root=PARTUUID=xxxxxxxx-xx rootfstype=ext4 fsck.repair=yes rootwait maxcpus=3
```

**Änderungen:**
- ❌ **ENTFERNT:** `console=serial0,115200` - Linux nutzt UART0 nicht mehr
- ❌ **ENTFERNT:** `console=tty1` - Kein HDMI Console (optional)
- ❌ **ENTFERNT:** `quiet splash` - Wir wollen Boot-Messages sehen (optional)
- ✅ **HINZUGEFÜGT:** `maxcpus=3` - Linux nutzt nur Cores 0-2

**WICHTIG:**
- Die gesamte Konfiguration MUSS in EINER Zeile sein (keine Zeilenumbrüche!)
- Behalte deine spezifische PARTUUID bei

---

### 2. UART aktivieren & Bluetooth deaktivieren (config.txt)

**Datei:** `/boot/config.txt` (oder `/boot/firmware/config.txt`)

**Am Ende der Datei (unter `[all]` Sektion) hinzufügen:**
```ini
[all]
# KRITISCH: Bluetooth deaktivieren!
# Auf RPi3 nutzt Bluetooth standardmäßig UART0 (PL011)
# Mit disable-bt wird UART0 auf GPIO 14/15 gemappt
dtoverlay=disable-bt

# UART0 Hardware aktivieren
enable_uart=1

# 64-bit mode (sollte bereits aktiviert sein)
arm_64bit=1
```

**⚠️ WICHTIG - Warum `disable-bt`?**

Auf **Raspberry Pi 3** gilt:
- **Standard:** Bluetooth nutzt UART0 (PL011, der gute UART)
- **Standard:** Mini UART ist auf GPIO 14/15 (instabil, für Console)
- **Mit `disable-bt`:** UART0 wird auf GPIO 14/15 gemappt ✅
- **Mit `disable-bt`:** Bluetooth ist deaktiviert (für AMP OK!)

**Was macht `enable_uart=1`?**
- Aktiviert UART0 Hardware
- Setzt korrekte GPIO ALT Funktionen (GPIO 14/15 → ALT0)
- **Zusammen mit `disable-bt`:** Voller PL011-UART auf GPIO 14/15!

---

## 📋 Schritt-für-Schritt Anleitung

### Auf deinem PC (SD-Karte eingesteckt):

```bash
# 1. Boot-Partition mounten
# (Automatisch gemountet als /media/$USER/boot oder /media/$USER/bootfs)
cd /media/$USER/boot  # oder bootfs

# 2. Backup erstellen!
sudo cp cmdline.txt cmdline.txt.backup
sudo cp config.txt config.txt.backup

# 3. cmdline.txt bearbeiten
sudo nano cmdline.txt

# Entferne: console=serial0,115200 console=tty1 quiet splash
# Füge hinzu: maxcpus=3
# Speichern: Ctrl+O, Enter, Ctrl+X

# 4. config.txt bearbeiten
sudo nano config.txt

# Am Ende hinzufügen:
# enable_uart=1
# arm_64bit=1
# Speichern: Ctrl+O, Enter, Ctrl+X

# 5. Sync & Unmount
sync
cd ~
sudo umount /media/$USER/boot
```

---

## ✅ Test: Linux bootet ohne UART Console

### Nach dem Boot:

**SSH verbinden:**
```bash
ssh pi@raspberrypi.local
# oder mit IP: ssh pi@192.168.x.x
```

**Prüfen:**
```bash
# 1. Nur 3 CPUs aktiv?
cat /proc/cpuinfo | grep processor
# Erwartung: processor 0, 1, 2 (NICHT 3!)

# 2. UART0 Hardware vorhanden?
ls -l /dev/ttyAMA0
# Erwartung: Device existiert

# 3. Kernel Boot Log prüfen
dmesg | grep uart
dmesg | grep tty
# Sollte zeigen: UART0 aktiviert, aber nicht als Console
```

**Erwartetes Ergebnis:**
- ✅ SSH funktioniert
- ✅ Linux läuft normal
- ✅ Nur 3 CPUs (0, 1, 2) sichtbar
- ✅ UART0 Hardware vorhanden, aber nicht von Linux genutzt

---

## ⚠️ KRITISCH: systemd Serial Getty Service deaktivieren

**Problem:** Auch mit korrekter cmdline.txt startet systemd automatisch einen **Login Prompt** auf UART!

### Nach dem ersten Boot:

**1. Check ob Service läuft:**
```bash
systemctl status serial-getty@ttyAMA0.service
```

**Falls `active (running)` → UART ist NICHT frei!**

### 2. Service permanent deaktivieren:

```bash
# Service stoppen
sudo systemctl stop serial-getty@ttyAMA0.service

# Service beim Boot deaktivieren
sudo systemctl disable serial-getty@ttyAMA0.service

# Service "masken" (verhindert manuelles Starten)
sudo systemctl mask serial-getty@ttyAMA0.service
```

**Was macht `mask`?**
- Erstellt Symlink: `/etc/systemd/system/serial-getty@ttyAMA0.service → /dev/null`
- Verhindert dass der Service jemals gestartet werden kann
- Bleibt permanent nach Reboot

### 3. Verifizieren:

```bash
# Status prüfen (sollte "masked" zeigen)
systemctl status serial-getty@ttyAMA0.service

# Prüfen ob UART frei ist
sudo lsof /dev/ttyAMA0
# Sollte KEINE Ausgabe haben!
```

**Nach Reboot:**
- Service bleibt disabled & masked ✅
- UART0 ist frei für Bare-Metal! ✅

---

## 🔌 Hardware: UART Verkabelung

**USB-UART Adapter → RPi3:**

```
USB-UART Adapter        Raspberry Pi 3
─────────────────       ───────────────
GND        ───────────  GND (Pin 6 oder 9)
RX         ───────────  GPIO 14 (TXD0, Pin 8)
TX         ───────────  GPIO 15 (RXD0, Pin 10)
```

**⚠️ WICHTIG:**
- **RX ↔ TX gekreuzt!** (Adapter RX → Pi TX, Adapter TX → Pi RX)
- **3.3V Pegel!** Kein 5V Adapter verwenden!
- **Kein VCC anschließen!** Pi hat eigene Stromversorgung

**Auf deinem PC:**
```bash
# UART Monitor starten
screen /dev/ttyUSB0 115200

# oder
minicom -D /dev/ttyUSB0 -b 115200

# Nach diesem Setup siehst du hier:
# - NICHTS von Linux (Console deaktiviert)
# - Output von Bare-Metal Code (Core 3)
```

---

## 🧪 Test: Bare-Metal UART funktioniert noch

**Nach obiger Linux-Konfiguration:**

### Option A: Bare-Metal alleine testen (temporär)

```bash
# Auf PC (SD-Karte Boot-Partition):
sudo cp /path/to/rpi3_amp/rpi3_uart_test/core3_uart_test.bin kernel8.img

# SD-Karte in RPi3, Boot
# UART sollte zeigen:
# > Core 3 UART Test
# > UART0 initialized successfully!
# > ...
```

**Nach Test: Linux Kernel zurückkopieren!**

### Option B: Mit Device Tree Overlay (später in Phase 2)

Hier wird Bare-Metal Code von Linux geladen und Core 3 gestartet (komplexer).

---

## 📊 Zusammenfassung - Erfolgreiche Konfiguration

| Komponente | Konfiguration | Status |
|------------|---------------|--------|
| **cmdline.txt** | `console=` entfernt, `maxcpus=3` | ✅ |
| **config.txt** | `dtoverlay=disable-bt`, `enable_uart=1` | ✅ |
| **Bluetooth** | Deaktiviert (gibt UART0 frei) | ✅ |
| **serial-getty** | disabled, stopped, masked | ✅ |
| **Linux CPUs** | 0, 1, 2 (Core 3 isoliert) | ✅ |
| **Core 3** | Frei für Bare-Metal | ✅ |
| **UART0 Hardware** | PL011 auf GPIO 14/15 | ✅ |
| **UART0 Nutzung** | Exklusiv für Bare-Metal | ✅ |
| **Linux Debugging** | SSH only | ✅ |
| **Bare-Metal Debug** | UART0 (GPIO 14/15) | ✅ |

---

## 🚀 Nächste Schritte (nach dieser Konfiguration)

1. ✅ Linux bootet mit maxcpus=3
2. ⏳ Device Tree Overlay für Memory Reservation
3. ⏳ Bare-Metal Code für 0x20000000 anpassen
4. ⏳ Core 3 per Mailbox von Linux starten
5. ⏳ OpenAMP Integration

---

## 🐛 Troubleshooting

### Problem: SSH funktioniert nicht

**Prüfen:**
```bash
# Auf PC (SD-Karte):
cat cmdline.txt
# Stelle sicher: root=PARTUUID=... ist noch da!
```

**Lösung:**
- Backup wiederherstellen: `sudo cp cmdline.txt.backup cmdline.txt`
- Nur `console=...` entfernen, REST behalten!

### Problem: UART zeigt Linux Messages (trotz config!)

**Häufigste Ursache:** systemd `serial-getty` Service läuft noch!

**Lösung:**
```bash
# Auf RPi3 (SSH):
sudo systemctl mask serial-getty@ttyAMA0.service
sudo systemctl stop serial-getty@ttyAMA0.service
sudo reboot
```

**Verifizieren:**
```bash
sudo lsof /dev/ttyAMA0
# Sollte nichts zeigen!
```

### Problem: UART zeigt nichts (beim Bare-Metal Test)

**Prüfen:**
1. Verkabelung korrekt? (RX↔TX gekreuzt)
2. `enable_uart=1` in config.txt?
3. USB-UART Adapter erkannt? `ls /dev/ttyUSB*`
4. Baudrate korrekt? (115200)

### Problem: Linux zeigt nur 2 CPUs (statt 3)

**Ursache:** `maxcpus=3` zählt von 0!
- `maxcpus=3` → CPUs 0, 1, 2 (3 Stück) ✅
- `maxcpus=2` → CPUs 0, 1 (2 Stück) ❌

---

**Erstellt:** 2025-11-23
**Version:** 1.0 - Initial AMP Configuration
