# ✅ Phase 2 Complete - Memory Reservation Erfolgreich!

**Datum:** 2025-11-24
**Status:** ✅ **ERFOLGREICH ABGESCHLOSSEN**
**SSH:** `ssh admin@rpi3-amp`

---

## 🎯 Phase 2 Ziel

Memory-Regionen für AMP reservieren, damit Linux sie nicht nutzt:
- **0x20000000-0x209FFFFF** (10 MB): Bare-Metal Code für Core 3
- **0x20A00000-0x20BFFFFF** (2 MB): Shared Memory für OpenAMP IPC

---

## ✅ Durchgeführte Schritte

### 1. Device Tree Overlay erstellt
**Datei:** `rpi3_amp/dts/rpi3-amp-reserved-memory.dtso`

```dts
/ {
    compatible = "brcm,bcm2837";

    fragment@0 {
        target-path = "/";
        __overlay__ {
            reserved-memory {
                amp_code@20000000 {
                    compatible = "shared-dma-pool";
                    reg = <0x20000000 0x00A00000>;  /* 10 MB */
                    no-map;
                };

                amp_shared@20A00000 {
                    compatible = "shared-dma-pool";
                    reg = <0x20A00000 0x00200000>;  /* 2 MB */
                    no-map;
                };
            };
        };
    };
}
```

### 2. Overlay kompiliert
```bash
# Auf dem RPi
dtc -@ -I dts -O dtb -o rpi3-amp-reserved-memory.dtbo rpi3-amp-reserved-memory.dtso
```

**Output:** 468 bytes compiled overlay

### 3. Overlay installiert
```bash
sudo cp rpi3-amp-reserved-memory.dtbo /boot/firmware/overlays/
echo "dtoverlay=rpi3-amp-reserved-memory" | sudo tee -a /boot/firmware/config.txt
```

### 4. System neu gestartet
```bash
sudo reboot
```

**Workflow (WSL → RPi):**
- SCP zum Übertragen: `scp file admin@rpi3-amp:~/`
- SSH für Commands: `ssh admin@rpi3-amp "command"`

---

## ✅ Verifikation - Alles funktioniert!

### Device Tree Nodes
```bash
$ ls -la /proc/device-tree/reserved-memory/
drwxr-xr-x  2 root root  0 Nov 24 16:29 amp_code@20000000
drwxr-xr-x  2 root root  0 Nov 24 16:29 amp_shared@20A00000
```

### Boot Messages (dmesg)
```
[    0.000000] OF: reserved mem: 0x0000000020000000..0x00000000209fffff (10240 KiB) nomap non-reusable amp_code@20000000
[    0.000000] OF: reserved mem: 0x0000000020a00000..0x0000000020bfffff (2048 KiB) nomap non-reusable amp_shared@20A00000
```

**Wichtig:** `nomap` bedeutet Linux kann diese Bereiche NICHT nutzen! ✅

### Memory Adressen verifiziert
```bash
# amp_code@20000000
$ cat /proc/device-tree/reserved-memory/amp_code@20000000/reg | od -t x4
0000000 00000020 0000a000
        ^^^^^^^^ ^^^^^^^^
        0x20000000 (Start)  0x00A00000 (10 MB)

# amp_shared@20A00000
$ cat /proc/device-tree/reserved-memory/amp_shared@20A00000/reg | od -t x4
0000000 0000a020 00002000
        ^^^^^^^^ ^^^^^^^^
        0x20A00000 (Start)  0x00200000 (2 MB)
```

### CPU Status
```bash
$ cat /proc/cpuinfo | grep processor
processor	: 0
processor	: 1
processor	: 2
```

**✅ Core 3 ist NICHT in der Liste → reserviert für Bare-Metal!**

### Kernel Command Line
```bash
$ cat /proc/cmdline
... maxcpus=3 ...
```

**✅ maxcpus=3 ist aktiv!**

### Memory Status
```
Total Memory:    970752K (~970 MB)
Available:       630736K (~631 MB)
Reserved:         72912K (~72 MB)  ← Inkl. unsere AMP Regionen!
```

---

## 📊 Aktueller Memory Layout

```
Physical Address Space:

0x00000000 ┌─────────────────────────────────────┐
           │                                     │
           │        Linux RAM (~512 MB)          │
           │                                     │
0x1FFFFFFF └─────────────────────────────────────┘

0x20000000 ┌─────────────────────────────────────┐ ✅ RESERVED!
           │  amp_code@20000000                  │
           │  10 MB - Bare-Metal Code/Data       │
           │  nomap - Linux kann nicht zugreifen │
0x209FFFFF └─────────────────────────────────────┘

0x20A00000 ┌─────────────────────────────────────┐ ✅ RESERVED!
           │  amp_shared@20A00000                │
           │  2 MB - Shared Memory für IPC       │
           │  nomap - Linux kann nicht zugreifen │
0x20BFFFFF └─────────────────────────────────────┘

0x20C00000 ┌─────────────────────────────────────┐
           │                                     │
           │    Linux RAM continued (~446 MB)    │
           │                                     │
0x3EFFFFFF └─────────────────────────────────────┘

0x3F000000   BCM2837 Peripherals
```

---

## 📝 Konfigurationsdateien

### `/boot/firmware/config.txt` (Ergänzungen)
```ini
[all]
enable_uart=1                        # ✅ Phase 1
dtoverlay=disable-bt                 # ✅ Phase 1
dtoverlay=rpi3-amp-reserved-memory   # ✅ Phase 2 NEU!
```

### `/boot/firmware/cmdline.txt`
```
... maxcpus=3 ...
```

**✅ Alle Konfigurationen aktiv!**

---

## 🎯 Was haben wir erreicht?

1. ✅ **Memory Reservation funktioniert**
   - 10 MB bei 0x20000000 für Bare-Metal reserviert
   - 2 MB bei 0x20A00000 für Shared Memory reserviert
   - Linux kann diese Bereiche NICHT überschreiben

2. ✅ **Core 3 ist frei**
   - Nur Core 0, 1, 2 laufen Linux
   - Core 3 wartet darauf, geweckt zu werden

3. ✅ **System ist stabil**
   - Linux bootet ohne Fehler
   - Alle Konfigurationen aktiv
   - Memory Layout ist korrekt

---

## ⏭️ Nächste Schritte: Phase 3

**Phase 3 Ziel:** Core 3 Launcher - Bare-Metal Code auf Core 3 starten!

### Was kommt als nächstes?

1. **Linux User-Space Tool schreiben** (`core3_launcher.c`)
   - Öffnet `/dev/mem` für direkten Memory-Zugriff
   - Lädt Bare-Metal Binary (kernel8.img) in reservierten Speicher (0x20000000)
   - Weckt Core 3 via ARM Local Mailbox (0x400000B0)

2. **Bare-Metal Code vorbereiten**
   - Simple Test-Binary die:
     - UART Output sendet ("Core 3 is alive!")
     - Counter hochzählt
     - In Endlosschleife läuft
   - ✅ Code bereits vorhanden in `rpi3_amp/rpi3_amp_core3/main.c`

3. **Testen**
   - Linux läuft auf Core 0-2
   - Bare-Metal läuft auf Core 3
   - Beide parallel aktiv!
   - UART zeigt Messages von Core 3

### Phase 3 Komponenten

```
Linux (Core 0-2)                    Bare-Metal (Core 3)
─────────────────                   ───────────────────
┌──────────────────┐               ┌──────────────────┐
│ core3_launcher   │               │  core3_amp.bin   │
│                  │               │                  │
│ 1. Load binary   │──────────────▶│ @ 0x20000000     │
│    to 0x20000000 │               │                  │
│                  │               │ - UART output    │
│ 2. Write mailbox │──────────────▶│ - Counter        │
│    @ 0x400000B0  │  (wake up!)   │ - Infinite loop  │
│                  │               │                  │
└──────────────────┘               └──────────────────┘
        │                                   │
        └────────── Both running! ──────────┘
```

### Notwendige Tools

- **gcc** (für core3_launcher)
- **aarch64-none-elf-gcc** (für Bare-Metal Code)
- `/dev/mem` Zugriff (evtl. `sudo` erforderlich)

---

## 📚 Dateien für Phase 3 Vorbereitung

**Bereits vorhanden:**
- ✅ `rpi3_amp/dts/rpi3-amp-reserved-memory.dtso` - Device Tree Overlay
- ✅ `rpi3_amp/dts/README.md` - Installations-Guide
- ✅ Memory Reservation aktiv auf RPi

**Zu erstellen in Phase 3:**
- ⏳ `rpi3_amp/core3_launcher/core3_launcher.c` - Linux Tool zum Starten von Core 3
- ⏳ `rpi3_amp/core3_simple_test/` - Simple Bare-Metal Test für Core 3
- ⏳ Makefile für beide Komponenten

---

## 🔍 Troubleshooting Referenz

Falls Probleme auftreten:

### Memory Reservation prüfen
```bash
# Device Tree Nodes
ls -la /proc/device-tree/reserved-memory/

# Boot Messages
dmesg | grep -i "reserved\|amp"

# Memory Adressen
cat /proc/device-tree/reserved-memory/amp_code@20000000/reg | od -t x4
```

### Overlay prüfen
```bash
# Ist Overlay geladen?
ls -la /boot/firmware/overlays/rpi3-amp-reserved-memory.dtbo

# Ist es in config.txt aktiviert?
cat /boot/firmware/config.txt | grep dtoverlay
```

### Core Status prüfen
```bash
# Nur 3 Cores sichtbar?
cat /proc/cpuinfo | grep processor

# maxcpus aktiv?
cat /proc/cmdline | grep maxcpus
```

---

## ✅ Phase 2 Checkliste - Alle Abgehakt!

**Vorbereitung:**
- [x] Memory Address validiert (0x20000000 OK!)
- [x] Device Tree Overlay Source erstellt
- [x] dtc verfügbar auf RPi

**Durchführung:**
- [x] Overlay kompiliert (468 bytes)
- [x] Nach /boot/firmware/overlays/ installiert
- [x] config.txt angepasst
- [x] RPi neu gestartet

**Verifikation:**
- [x] Device Tree Nodes existieren
- [x] dmesg zeigt reserved memory mit `nomap`
- [x] Memory Adressen korrekt (0x20000000, 0x20A00000)
- [x] Nur Core 0, 1, 2 aktiv
- [x] Linux bootet ohne Fehler

**Bereit für Phase 3:**
- [x] Memory für Bare-Metal Code reserviert (10 MB)
- [x] Memory für Shared IPC reserviert (2 MB)
- [x] Core 3 ist frei und wartet

---

## 🎉 Erfolg!

**Phase 2 ist komplett und erfolgreich abgeschlossen!**

Die Hardware ist jetzt bereit für AMP:
- Linux hat Core 0-2 und genug Memory
- Core 3 ist frei und reserviert
- 12 MB Memory sind geschützt und bereit für Bare-Metal Code

**Wir können jetzt Core 3 wecken und Bare-Metal Code darauf laufen lassen!** 🚀

---

**Status:** ✅ **PHASE 2 COMPLETE - Ready for Phase 3!**

*Dokumentiert - 2025-11-24*
