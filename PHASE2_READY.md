# 🎯 Phase 2 Ready - Device Tree Overlay

**Datum:** 2025-11-23
**Status:** ✅ **Vorbereitet - Bereit zum Start!**

---

## ✅ Phase 1 Recap

Phase 1 wurde erfolgreich abgeschlossen:
- ✅ Linux bootet mit `maxcpus=3`
- ✅ UART0 ist frei für Bare-Metal
- ✅ Bare-Metal Code bei 0x20000000 bereit
- ✅ Dokumentation komplett

**Details:** Siehe `PHASE1_COMPLETE.md`

---

## 🔍 Memory Address Validation

**Frage:** Ist 0x20000000 eine valide Adresse auf RPi3?
**Antwort:** ✅ **JA! Perfekt geeignet!**

### Verifikation:

```
RPi3 Memory Layout:
- Total: 1 GB (0x00000000 - 0x3FFFFFFF)
- SDRAM: 0x00000000 - 0x3EFFFFFF (~1008 MB)
- Peripherals: 0x3F000000 - 0x3FFFFFFF (16 MB)

0x20000000 = 512 MB offset
→ Liegt SICHER im SDRAM Bereich!
→ Weit entfernt von Peripherals (0x3F000000)
```

### Warum 0x20000000 gut ist:

1. ✅ In der Mitte des RAMs (512 MB Position)
2. ✅ Genug Platz für Linux davor (512 MB)
3. ✅ Genug Platz für Linux danach (~499 MB)
4. ✅ Keine Konflikte mit Hardware (Peripherals bei 0x3F000000)
5. ✅ Einfach zu merken und zu berechnen
6. ✅ Gleiche Adresse wie TImada's RPi4 Projekt

**Visueller Memory Map:** Siehe `quick_reference_card.md` (neu hinzugefügt!)

---

## 🎯 Phase 2 Plan - Device Tree Overlay

### Ziel:
Memory bei 0x20000000-0x20BFFFFF für AMP reservieren

### Was ist ein Device Tree Overlay?

**Kurz gesagt:** Ein "Klebezettel" für Linux der sagt: "Diese Memory-Region ist RESERVED - nicht anfassen!"

**Technisch:**
- Device Tree = Bauplan der Hardware für Linux
- Overlay = Ergänzung/Modifikation des Device Trees
- `no-map` Flag = Linux darf diesen Speicher nicht nutzen

**Warum wichtig für AMP?**
- Core 3 Bare-Metal Code läuft bei 0x20000000
- Linux darf dort NICHT reinschreiben!
- Ohne Reservation → Kernel crash oder Datenverlust

### Aufgaben für Phase 2:

#### 1. Device Tree Overlay erstellen
**Datei:** `rpi3-amp-reserved-memory.dtso`

```dts
/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2837";

    fragment@0 {
        target-path = "/";
        __overlay__ {
            reserved-memory {
                #address-cells = <1>;
                #size-cells = <1>;
                ranges;

                // Bare-Metal Code (10 MB)
                amp_code@20000000 {
                    compatible = "shared-dma-pool";
                    reg = <0x20000000 0x00A00000>;  // 10 MB
                    no-map;
                };

                // Shared Memory IPC (2 MB)
                amp_shared@20A00000 {
                    compatible = "shared-dma-pool";
                    reg = <0x20A00000 0x00200000>;  // 2 MB
                    no-map;
                };
            };
        };
    };
};
```

#### 2. Overlay kompilieren

```bash
cd /path/to/project/rpi3_amp/dts
dtc -@ -I dts -O dtb -o rpi3-amp-reserved-memory.dtbo rpi3-amp-reserved-memory.dtso
```

**Flags:**
- `-@` = Enable symbol generation (für Overlays)
- `-I dts` = Input format: Device Tree Source
- `-O dtb` = Output format: Device Tree Blob
- `-o` = Output file

#### 3. Overlay auf SD-Karte installieren

```bash
# Overlay kopieren
sudo cp rpi3-amp-reserved-memory.dtbo /media/$USER/boot/overlays/

# In config.txt aktivieren
echo "dtoverlay=rpi3-amp-reserved-memory" | sudo tee -a /media/$USER/boot/config.txt
```

#### 4. Testen & Verifizieren

```bash
# Nach Reboot auf RPi3:
cat /proc/iomem | grep amp

# Erwartete Ausgabe:
# 20000000-209fffff : amp_code
# 20a00000-20bfffff : amp_shared
```

**Alternative Verifikation:**
```bash
# Device Tree dekompilieren
dtc -I fs /sys/firmware/devicetree/base > current.dts
cat current.dts | grep -A 10 reserved-memory

# Oder direkt:
cat /proc/device-tree/reserved-memory/amp_code@20000000/reg | od -t x4
```

---

## 📊 Memory Layout Nach Phase 2

```
0x00000000 ┌─────────────────────────────────────┐
           │        Linux RAM (512 MB)           │
0x1FFFFFFF └─────────────────────────────────────┘

0x20000000 ┌─────────────────────────────────────┐
           │   RESERVED - AMP Code (10 MB)       │ ← Device Tree "no-map"
0x209FFFFF └─────────────────────────────────────┘

0x20A00000 ┌─────────────────────────────────────┐
           │   RESERVED - Shared Mem (2 MB)      │ ← Device Tree "no-map"
0x20BFFFFF └─────────────────────────────────────┘

0x20C00000 ┌─────────────────────────────────────┐
           │    Linux RAM continued (~499 MB)    │
0x3EFFFFFF └─────────────────────────────────────┘

0x3F000000   BCM2837 Peripherals
```

**Linux sieht:**
- 512 MB RAM bei 0x00000000
- ~499 MB RAM bei 0x20C00000
- **SKIP:** 0x20000000-0x20BFFFFF (reserved!)

**Core 3 nutzt:**
- 0x20000000: Code, Data, Stack (10 MB)
- 0x20A00000: Shared Memory für IPC (2 MB)

---

## 🔧 Troubleshooting Phase 2

### Problem 1: `dtc` not found
```bash
sudo apt-get install device-tree-compiler
```

### Problem 2: Overlay lädt nicht
```bash
# Check boot messages
dmesg | grep -i "device tree\|overlay"

# Check overlay syntax
dtc -I dtb -O dts rpi3-amp-reserved-memory.dtbo
```

### Problem 3: Memory nicht reserviert
```bash
# Verify overlay in config.txt
cat /boot/config.txt | grep dtoverlay

# Check if overlay file exists
ls -la /boot/overlays/rpi3-amp-reserved-memory.dtbo
```

### Problem 4: `/proc/iomem` zeigt nichts
```bash
# Prüfe ob reserved-memory im Device Tree ist
cat /proc/device-tree/reserved-memory/*/reg | od -t x4

# Oder dekompilieren
dtc -I fs /sys/firmware/devicetree/base | grep -A 20 reserved
```

---

## ⏭️ Nach Phase 2: Phase 3 Preview

**Phase 3: Core 3 Launcher**

Wenn Memory Reservation funktioniert:
1. **Linux User-Space Tool** schreiben (`core3_launcher.c`)
2. Tool lädt `core3_amp.bin` in reservierten Speicher (0x20000000)
3. Tool weckt Core 3 per Mailbox auf (0x400000B0)
4. Core 3 startet und läuft parallel zu Linux!
5. UART zeigt Bare-Metal Debug Messages

**Tools:**
- `/dev/mem` für Memory Zugriff
- `mmap()` für Speicher-Mapping
- Mailbox Write für Core Wakeup

---

## 📁 Dateien für Phase 2

```
rpi3_amp_project/
├── quick_reference_card.md           # ✅ Memory Map Visualisierung hinzugefügt
├── PHASE1_COMPLETE.md                # ✅ Phase 1 Zusammenfassung
├── PHASE2_READY.md                   # ✅ Dieses Dokument
└── rpi3_amp/
    ├── dts/                          # ⏳ Zu erstellen
    │   └── rpi3-amp-reserved-memory.dtso  # ⏳ Device Tree Source
    ├── rpi3_amp_core3/
    │   └── core3_amp.bin             # ✅ Ready (aus Phase 1)
    └── AMP_CONFIGURATION_GUIDE.md    # ✅ Linux Config Guide
```

---

## ✅ Checkliste Phase 2

**Vorbereitung:**
- [x] Memory Address validiert (0x20000000 OK!)
- [x] Memory Map Visualisierung dokumentiert
- [x] Phase 2 Plan erstellt
- [ ] `dtc` installiert auf Entwicklungsrechner

**Durchführung:**
- [ ] Device Tree Overlay Source erstellen
- [ ] Overlay kompilieren
- [ ] Overlay auf SD-Karte kopieren
- [ ] `config.txt` anpassen
- [ ] RPi3 rebooten

**Verifikation:**
- [ ] `/proc/iomem` zeigt reserved regions
- [ ] Linux bootet ohne Fehler
- [ ] Cores: nur 0,1,2 aktiv
- [ ] Memory für AMP reserviert

**Bereit für Phase 3:**
- [ ] Memory Reservation verifiziert
- [ ] UIO Device (optional) vorbereitet

---

## 📚 Referenzen

**Device Tree Dokumentation:**
- Linux Kernel Device Tree Specification
- Raspberry Pi Overlay README: `/boot/overlays/README`

**Memory Reservation:**
- `reserved-memory` DT binding
- `no-map` vs `reusable` properties

**Tools:**
- `dtc` - Device Tree Compiler
- `dtc -I fs` - Dekompiliert laufenden Device Tree
- `od` - Octal/Hex dump für binary DT properties

---

**Status:** ✅ **Alles vorbereitet - Ready to go! 🚀**

**Nächster Schritt:** Device Tree Overlay erstellen und testen

---

*Dokumentiert - 2025-11-23*
