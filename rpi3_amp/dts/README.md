# Device Tree Overlay für RPi3 AMP

## Dateien

- **rpi3-amp-reserved-memory.dtso** - Device Tree Source (Human-readable)
- **rpi3-amp-reserved-memory.dtbo** - Device Tree Blob (Compiled, wird auf RPi erstellt)

## Installation auf dem Raspberry Pi

### Schritt 1: Dateien auf RPi kopieren

Von WSL aus:
```bash
# SCP zum Pi (passe IP und Username an)
scp rpi3-amp-reserved-memory.dtso pi@<IP-ADDRESS>:~/
```

### Schritt 2: Auf dem RPi kompilieren

Via SSH auf dem Pi:
```bash
# dtc installieren (falls nicht vorhanden)
sudo apt-get update
sudo apt-get install device-tree-compiler

# Overlay kompilieren
dtc -@ -I dts -O dtb -o rpi3-amp-reserved-memory.dtbo rpi3-amp-reserved-memory.dtso

# Nach /boot/overlays kopieren
sudo cp rpi3-amp-reserved-memory.dtbo /boot/overlays/

# Permissions prüfen
ls -la /boot/overlays/rpi3-amp-reserved-memory.dtbo
```

### Schritt 3: config.txt anpassen

```bash
# Overlay aktivieren
echo "dtoverlay=rpi3-amp-reserved-memory" | sudo tee -a /boot/config.txt

# Prüfen
tail /boot/config.txt
```

### Schritt 4: Reboot

```bash
sudo reboot
```

### Schritt 5: Verifizieren

Nach dem Neustart:
```bash
# Reserved Memory prüfen
cat /proc/iomem | grep amp

# Erwartete Ausgabe:
#   20000000-209fffff : amp_code@20000000
#   20a00000-20bfffff : amp_shared@20A00000

# Alternative: Device Tree inspizieren
cat /proc/device-tree/reserved-memory/amp_code@20000000/reg | od -t x4

# Oder komplett dekompilieren
dtc -I fs /sys/firmware/devicetree/base > /tmp/current.dts
cat /tmp/current.dts | grep -A 10 reserved-memory
```

## Was macht das Overlay?

**Reserviert 12 MB Memory:**
- **0x20000000 - 0x209FFFFF** (10 MB): Bare-Metal Code für Core 3
- **0x20A00000 - 0x20BFFFFF** (2 MB): Shared Memory für OpenAMP IPC

**`no-map` Flag:** Linux darf diese Regionen NICHT nutzen!

## Memory Layout nach Installation

```
0x00000000 - 0x1FFFFFFF  |  512 MB  | Linux RAM
0x20000000 - 0x209FFFFF  |   10 MB  | RESERVED - Bare-Metal Code
0x20A00000 - 0x20BFFFFF  |    2 MB  | RESERVED - Shared Memory
0x20C00000 - 0x3EFFFFFF  | ~499 MB  | Linux RAM (continued)
0x3F000000 - 0x3FFFFFFF  |   16 MB  | BCM2837 Peripherals
```

## Troubleshooting

### Overlay lädt nicht

```bash
# Boot Messages prüfen
dmesg | grep -i "device tree\|overlay\|reserved"

# Syntax prüfen
dtc -I dtb -O dts /boot/overlays/rpi3-amp-reserved-memory.dtbo
```

### Memory nicht reserviert

```bash
# config.txt prüfen
cat /boot/config.txt | grep dtoverlay

# Overlay File prüfen
ls -la /boot/overlays/rpi3-amp-reserved-memory.dtbo
```

### /proc/iomem zeigt nichts

```bash
# Direct DT check
ls -la /proc/device-tree/reserved-memory/

# Oder mit od
for node in /proc/device-tree/reserved-memory/amp_*; do
    echo "Node: $node"
    cat "$node/reg" | od -t x4
done
```

## Erfolgreiche Installation

Wenn alles funktioniert:
- ✅ `/proc/iomem` zeigt amp_code und amp_shared
- ✅ Linux bootet normal (3 Cores aktiv)
- ✅ Memory Gap bei 0x20000000 vorhanden
- ✅ Bereit für Phase 3: Core 3 Launcher!
