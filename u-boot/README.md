# U-Boot for RPi3 AMP

This directory contains the custom U-Boot files for the RPi3 AMP project.

## Files

- `u-boot.bin` (637 KB) - Compiled U-Boot bootloader for RPi3
- `boot.scr` (2.5 KB) - Compiled boot script
- `boot.scr.txt` - Boot script source (human-readable)

## Installation

Copy these files to the RPi SD card boot partition:

```bash
# Mount SD card (WSL example)
sudo mount -t drvfs D: /mnt/d

# Backup original kernel
sudo cp /mnt/d/kernel8.img /mnt/d/kernel8.img.backup

# Install U-Boot as kernel8.img
sudo cp u-boot.bin /mnt/d/kernel8.img

# Install boot script
sudo cp boot.scr /mnt/d/

# Install Core 3 binary
sudo cp ../rpi3_amp/rpi3_amp_core3/core3_amp.bin /mnt/d/

# Unmount
sudo umount /mnt/d
```

## How It Works

1. GPU loads U-Boot (kernel8.img)
2. U-Boot runs boot.scr script
3. Script loads Core 3 binary to 0x20000000
4. Script writes to ARM Spin Table (0xF0)
5. Core 3 starts running bare-metal code
6. Script loads Linux kernel (kernel8.img.backup)
7. Linux boots on Cores 0-2

## Rebuilding U-Boot

If you need to rebuild U-Boot from source:

```bash
# Clone U-Boot
git clone --depth 1 --branch v2026.01-rc3 https://github.com/u-boot/u-boot.git u-boot-src
cd u-boot-src

# Configure for RPi3
make rpi_3_defconfig

# Enable cache commands (optional)
echo "CONFIG_CMD_CACHE=y" >> .config

# Build
make CROSS_COMPILE=aarch64-none-elf-

# Copy binary
cp u-boot.bin ../u-boot/
```

## Modifying Boot Script

Edit `boot.scr.txt`, then recompile:

```bash
# Requires mkimage (from U-Boot tools)
mkimage -A arm64 -O linux -T script -C none -d boot.scr.txt boot.scr
```

## Recovery

If the system doesn't boot:

```bash
# Restore original kernel
sudo cp /mnt/d/kernel8.img.backup /mnt/d/kernel8.img
sudo rm /mnt/d/boot.scr
```

## Documentation

See:
- `../PHASE3B_SUCCESS.md` - Complete Phase 3B documentation
- `../CURRENT_STATUS.md` - Project status and next steps
- `../CLAUDE.md` - Project overview
