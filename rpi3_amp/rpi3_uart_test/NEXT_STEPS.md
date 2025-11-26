# 🚀 Nächste Schritte - AMP Roadmap

**Aktueller Status:** ✅ Bare-Metal UART0 Test erfolgreich!

Jetzt geht es weiter zum eigentlichen Ziel: **Linux und Bare-Metal parallel laufen lassen (AMP)**

---

## 📍 Wo wir stehen

✅ **Was funktioniert:**
- Bare-Metal Code auf RPi3 läuft
- UART-Kommunikation funktioniert
- Hardware Setup ist OK
- Build-System ist OK
- Peripherals (GPIO, UART) funktionieren

❌ **Was noch fehlt:**
- Linux bootet nicht (Kernel wurde ersetzt)
- Core 3 wird nicht isoliert
- Keine Inter-Core Communication
- Kein shared Memory Setup

---

## 🎯 Phase 1: Linux wiederherstellen

**Ziel:** Linux bootet wieder normal, unser Bare-Metal Code ist für später bereit.

### Schritt 1.1: Linux Kernel zurückholen

```bash
# Auf PC mit SD-Karte:
# Option A: Backup zurückkopieren
sudo cp /media/$USER/boot/kernel8.img.linux_backup /media/$USER/boot/kernel8.img

# Option B: Von frischem Image holen
# Download Raspberry Pi OS Lite 64-bit
# Extrahiere kernel8.img
# Kopiere auf SD-Karte

# Option C: Kompletter Neustart
# SD-Karte mit Raspberry Pi Imager neu flashen
```

### Schritt 1.2: Bare-Metal Code für AMP vorbereiten

```bash
# Unser Test-Binary umbenennen
cp kernel8.img rpi3_uart_test.bin

# Für später: Dieses Binary wird NICHT als kernel8.img gebootet,
# sondern von Linux in reservierten Speicher geladen!
```

### Schritt 1.3: Test - Linux bootet wieder

```bash
# SD-Karte in RPi3
# Boot
# SSH sollte funktionieren
# UART0 zeigt Linux Boot-Messages
```

**Status:** ⏳ Noch nicht gemacht

---

## 🎯 Phase 2: AMP Konfiguration

**Ziel:** Linux nutzt nur Cores 0-2, Core 3 ist frei für Bare-Metal

### Schritt 2.1: Kernel Commandline anpassen

```bash
# cmdline.txt bearbeiten:
sudo nano /boot/cmdline.txt

# Am ENDE der Zeile hinzufügen (nicht neue Zeile!):
maxcpus=3

# Das sagt Linux: "Nutze nur 3 CPUs (0, 1, 2)"
# Core 3 bleibt ungenutzt
```

### Schritt 2.2: Memory für Core 3 reservieren

**Device Tree Overlay erstellen:**

```dts
// amp-reserved-memory.dtso
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

                // Bare-Metal Code Region (10 MB)
                amp_code: amp_code@20000000 {
                    compatible = "shared-dma-pool";
                    reg = <0x20000000 0x00A00000>;  // 10 MB
                    no-map;  // Linux darf hier NICHT hin!
                };

                // Shared Memory IPC (2 MB)
                amp_shared: amp_shared@20A00000 {
                    compatible = "shared-dma-pool";
                    reg = <0x20A00000 0x00200000>;  // 2 MB
                    no-map;
                };
            };
        };
    };
};
```

**Kompilieren und installieren:**

```bash
# Kompilieren
dtc -@ -I dts -O dtb -o amp-reserved-memory.dtbo amp-reserved-memory.dtso

# Auf SD-Karte kopieren
sudo cp amp-reserved-memory.dtbo /media/$USER/boot/overlays/

# In config.txt aktivieren
sudo nano /media/$USER/boot/config.txt
# Hinzufügen:
dtoverlay=amp-reserved-memory
```

### Schritt 2.3: Testen - Memory Reservation

```bash
# Nach Boot auf RPi3:
cat /proc/iomem | grep amp

# Sollte zeigen:
# 20000000-209fffff : amp_code
# 20a00000-20bfffff : amp_shared
```

**Status:** ⏳ Noch nicht gemacht

---

## 🎯 Phase 3: Core 3 aufwecken

**Ziel:** Linux lädt Bare-Metal Code in reservierten Speicher und startet Core 3

### Schritt 3.1: Bare-Metal Code anpassen

**Wichtige Änderungen:**

```c
// main.c - Core 3 Version
// Boot-Code muss Core 3 durchlassen:

// boot.S:
_start:
    mrs     x0, mpidr_el1
    and     x0, x0, #0x3
    cmp     x0, #3          // Nur Core 3!
    bne     core_halt

    // Core 3: weiter zum Setup
    b       core3_start

core3_start:
    // Stack setzen
    // BSS clearen
    // main() aufrufen
```

**Linker-Script anpassen:**

```ld
// Code startet bei 0x20000000 (reservierter Bereich!)
. = 0x20000000;
```

**Neu bauen:**

```bash
make clean && make
# Produziert kernel8.img -> umbenennen zu core3_app.bin
mv kernel8.img core3_app.bin
```

### Schritt 3.2: Linux User-Space Tool

**C-Programm zum Core 3 Start:**

```c
// core3_launcher.c
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define AMP_CODE_BASE    0x20000000
#define AMP_CODE_SIZE    0x00A00000
#define MAILBOX3_CORE3   0x400000B0

int main(int argc, char *argv[]) {
    // 1. Binary einlesen
    FILE *fp = fopen("core3_app.bin", "rb");
    // Binary-Daten lesen...

    // 2. In reservierten Speicher kopieren
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *amp_mem = mmap(NULL, AMP_CODE_SIZE,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED, mem_fd, AMP_CODE_BASE);

    // Kopieren...
    memcpy(amp_mem, binary_data, binary_size);

    // 3. Core 3 per Mailbox aufwecken
    volatile uint32_t *mailbox = mmap(NULL, 4096,
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED, mem_fd, 0x40000000);

    mailbox[0xB0/4] = AMP_CODE_BASE;  // Jump-Adresse

    // 4. Core 3 wacht auf und läuft!
    return 0;
}
```

### Schritt 3.3: Test - Core 3 läuft

```bash
# Auf RPi3 (Linux läuft):
sudo ./core3_launcher

# UART sollte Meldungen von Core 3 zeigen!
# (Wenn UART2 genutzt wird - UART0 ist von Linux belegt)
```

**Status:** ⏳ Noch nicht gemacht

---

## 🎯 Phase 4: OpenAMP Integration

**Ziel:** Strukturierte IPC zwischen Linux und Core 3

### Wichtige Komponenten:
1. **libmetal** - Hardware Abstraction Layer
2. **OpenAMP** - Remoteproc & RPMsg Framework
3. **Resource Table** - Beschreibt Core 3 Ressourcen
4. **VirtIO Rings** - Shared Memory Buffer

### Das ist komplex!
Siehe `../libmetal_rpi3/` und `../open-amp_rpi3/` für Ports.

**Status:** ⏳ Große Aufgabe, später

---

## 📊 Prioritäten

**Kurzfristig (nächste Sessions):**
1. ✅ **DONE:** Bare-Metal UART Test
2. ⏳ **TODO:** Linux wiederherstellen
3. ⏳ **TODO:** maxcpus=3 testen
4. ⏳ **TODO:** Memory Reservation via Device Tree

**Mittelfristig:**
5. ⏳ Core 3 manuell aufwecken (ohne OpenAMP)
6. ⏳ Einfache Mailbox-Kommunikation testen

**Langfristig:**
7. ⏳ OpenAMP/RPMsg integrieren
8. ⏳ FreeRTOS auf Core 3
9. ⏳ Produktions-Anwendung entwickeln

---

## 🔧 Nützliche Ressourcen

**Im Projekt:**
- `../../rpi4_rpmsg_ref/` - TImada's RPi4 Referenz
- `../../docs/` - Detaillierte Dokumentation
- `../libmetal_rpi3/` - Begonnene libmetal Port
- `../open-amp_rpi3/` - Begonnene OpenAMP Port

**Dokumentation:**
- BCM2836 QA7 (ARM Local Peripherals) - CRITICAL für Mailboxes!
- OpenAMP Documentation
- Raspberry Pi Boot Flow

---

**Letzte Aktualisierung:** 2025-11-23
**Bereit für nächsten Schritt:** Phase 1 - Linux wiederherstellen
