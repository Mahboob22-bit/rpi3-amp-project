# RPi3 AMP Project - Current Status

**Last Updated:** 2025-11-25
**Status:** ✅ **Phase 3B Complete - Core 3 Running!**

---

## 🎯 Quick Summary

**What Works:**
```
┌────────────────────────────────────────┐
│  Core 0-2: Linux (Raspberry Pi OS)    │
│  Core 3:   Bare-Metal @ 0x20000000    │
│  Memory:   512 MB Linux, 12 MB AMP    │
│  Boot:     U-Boot → Core 3 → Linux    │
└────────────────────────────────────────┘
```

**Proof:**
- ✅ UART shows Core 3 messages
- ✅ Linux boots and SSH works
- ✅ No interference between Linux and Core 3

---

## 📂 Important Files

**Working Configuration (on RPi):**
```
/boot/firmware/
├── kernel8.img         ← U-Boot (637 KB)
├── kernel8.img.backup  ← Original Linux Kernel
├── boot.scr            ← Boot script (CRITICAL!)
└── core3_amp.bin       ← Core 3 code
```

**Source Files:**
```
/home/mahboob/rpi3_amp_project/
├── u-boot-rpi3/
│   ├── u-boot.bin      ← U-Boot binary
│   ├── boot.scr.txt    ← Boot script source
│   └── boot.scr        ← Compiled script
├── rpi3_amp/rpi3_amp_core3/
│   ├── boot.S          ← Assembly startup
│   ├── main.c          ← Core 3 C code
│   └── core3_amp.bin   ← Compiled binary
└── Documentation/
    ├── PHASE3B_SUCCESS.md  ← Full report of this phase
    ├── CURRENT_STATUS.md   ← This file
    └── CLAUDE.md           ← Project overview
```

---

## 🚀 What to Do Next (Phase 4)

**Goal:** Add simple communication between Linux and Core 3

**Options (in order of complexity):**

1. **Shared Memory Status** (Easiest - Start here!)
   - Core 3 writes status to 0x20A00000
   - Linux reads via `/dev/mem`
   - No interrupts needed

2. **Mailbox IPC** (Medium)
   - Use ARM Local Mailboxes (0x40000000)
   - Core 3 waits for messages
   - Linux sends commands
   - Interrupt-driven

3. **OpenAMP/RPMsg** (Complex - Final goal)
   - Full IPC framework
   - Requires porting libmetal + OpenAMP
   - Similar to TImada's RPi4 implementation

**Recommendation:** Start with #1, then #2, then #3

---

## 🔧 How to Modify and Test

### Change Core 3 Code:
```bash
cd /home/mahboob/rpi3_amp_project/rpi3_amp/rpi3_amp_core3
# Edit main.c
make clean && make

# Deploy to SD card (mount D: in Windows/WSL):
sudo mount -t drvfs D: /mnt/d
sudo cp core3_amp.bin /mnt/d/
sudo umount /mnt/d
```

### Change Boot Script:
```bash
cd /home/mahboob/rpi3_amp_project/u-boot-rpi3
# Edit boot.scr.txt
./tools/mkimage -A arm64 -O linux -T script -C none -d boot.scr.txt boot.scr

# Deploy:
sudo mount -t drvfs D: /mnt/d
sudo cp boot.scr /mnt/d/
sudo umount /mnt/d
```

### Test:
1. Insert SD card in RPi
2. Connect UART: `screen /dev/ttyUSB0 115200`
3. Power on RPi
4. Watch UART for Core 3 messages
5. SSH to check Linux: `ssh admin@rpi3-amp`

---

## 📖 Key Documentation to Read

**Before starting Phase 4:**
1. `PHASE3B_SUCCESS.md` - Understand what we fixed in Phase 3B
2. `quick_reference_card.md` - Hardware addresses and code snippets
3. `CLAUDE.md` - Overall project architecture

**Reference implementations:**
- `rpi4_ref/` - TImada's RPi4 implementation (final goal)
- `rpi3_tutorial_ref/` - Bare-metal RPi3 tutorials

---

## 🎓 What We Learned in Phase 3B

### ARM Spin Table (Core Boot Mechanism)

**Simple explanation:**
- Cores 1-3 sleep in a loop, checking addresses 0xE0, 0xE8, 0xF0
- To wake Core 3: Write jump address to 0xF0, send event
- Core 3 jumps to that address and runs

**Implementation:**
- GPU loads `armstub8.bin` which parks cores
- U-Boot writes to Spin Table
- No need to modify our Core 3 code!

### U-Boot Boot Script Tricks

**Problem:** Can't load directly to reserved memory
**Solution:** Load to temp address (0x00100000), then copy

**Problem:** Need to wake Core 3
**Solution:** Memory barriers (`dcache flush`) work instead of SEV

**Problem:** Linux kernel overwritten by U-Boot
**Solution:** Keep backup as `kernel8.img.backup`

---

## 🐛 Troubleshooting

### Core 3 doesn't start:
1. Check UART - is binary loaded correctly?
2. Check `boot.scr` is on SD card
3. Re-compile Core 3 code
4. Check Spin Table write in boot script

### Linux doesn't boot:
1. Check `kernel8.img.backup` exists
2. Restore kernel: `cp kernel8.img.backup kernel8.img`
3. Remove boot.scr to disable U-Boot script

### Recovery:
```bash
# On SD card (mounted as D:):
sudo mount -t drvfs D: /mnt/d
sudo cp /mnt/d/kernel8.img.backup /mnt/d/kernel8.img
sudo rm /mnt/d/boot.scr  # Disable U-Boot script
sudo umount /mnt/d
```

---

## 📊 Project Phases

```
✅ Phase 1: Planning & Setup
✅ Phase 2: Memory Reservation (Device Tree)
✅ Phase 3A: Userspace Launcher (Failed - cache issues)
✅ Phase 3B: U-Boot Boot (SUCCESS! 🎉)
🎯 Phase 4: Simple IPC ← YOU ARE HERE
⏳ Phase 5: OpenAMP/RPMsg
⏳ Phase 6: FreeRTOS Integration
```

---

## 💡 Quick Commands

```bash
# SSH to RPi
ssh admin@rpi3-amp

# UART Monitor
screen /dev/ttyUSB0 115200

# Mount SD card (WSL)
sudo mount -t drvfs D: /mnt/d

# Check Core 3 binary
hexdump -C rpi3_amp/rpi3_amp_core3/core3_amp.bin | head

# Compile Core 3
cd rpi3_amp/rpi3_amp_core3 && make clean && make

# Compile Boot Script
cd u-boot-rpi3 && ./tools/mkimage -A arm64 -O linux -T script -C none -d boot.scr.txt boot.scr
```

---

## 🎯 Immediate Next Steps

**For the next chat session:**

1. **Test Shared Memory Communication**
   - Modify `main.c` to write pattern to 0x20A00000
   - Write Linux program to read via `/dev/mem`
   - Verify data integrity

2. **Expand Core 3 Functionality**
   - Add more GPIO control
   - Add timer/delays
   - Add more UART messages

3. **Prepare for Mailbox IPC**
   - Study ARM Local Mailbox registers
   - Understand IRQ handling on Core 3
   - Plan interrupt-driven communication

---

**Ready to continue! 🚀**

See `PHASE3B_SUCCESS.md` for full technical details.
