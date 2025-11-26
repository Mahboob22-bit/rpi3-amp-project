# RPi3 AMP Project - Current Status

**Last Updated:** 2025-11-26
**Status:** ✅ **Phase 4 In Progress - Shared Memory IPC Working!**

---

## 🎯 Quick Summary

**What Works:**
```
┌────────────────────────────────────────────────────────────┐
│  Core 0-2: Linux (Raspberry Pi OS)                         │
│  Core 3:   Modular Bare-Metal Firmware @ 0x20000000        │
│  Memory:   512 MB Linux, 12 MB AMP                         │
│  Boot:     U-Boot → Core 3 → Linux                         │
│  IPC:      Shared Memory @ 0x20A00000 ✅                   │
│  Deploy:   Via SSH (no SD card swap needed!)               │
└────────────────────────────────────────────────────────────┘
```

**Proof:**
- ✅ UART shows Core 3 ASCII banner and heartbeats
- ✅ Linux boots and SSH works
- ✅ `read_shared_mem` on Linux shows Core 3 status
- ✅ No interference between Linux and Core 3

---

## 📂 Important Files

**Working Configuration (on RPi):**
```
/boot/firmware/
├── kernel8.img         ← U-Boot (637 KB)
├── kernel8.img.backup  ← Original Linux Kernel
├── boot.scr            ← Boot script (CRITICAL!)
└── core3_amp.bin       ← Core 3 firmware (~12 KB)

/usr/local/bin/
└── read_shared_mem     ← Linux tool to read Core 3 status
```

**Source Files (Modular Structure):**
```
/home/mahboob/rpi3_amp_project/rpi3_amp/
├── rpi3_amp_core3/           ← Core 3 Firmware
│   ├── boot.S                ← Assembly startup
│   ├── link.ld               ← Linker script (0x20000000)
│   ├── common.h              ← Hardware addresses, types
│   ├── uart.h / uart.c       ← UART0 driver with printf
│   ├── timer.h / timer.c     ← System Timer (real timestamps)
│   ├── memory.h / memory.c   ← Shared memory & memtest
│   ├── main.c                ← Main program + heartbeat
│   ├── Makefile              ← Build + SSH deploy
│   └── core3_amp.bin         ← Compiled binary
└── linux_tools/              ← Linux-side tools
    ├── read_shared_mem.c     ← Shared memory reader
    └── Makefile              ← Build + deploy
```

---

## 🚀 Quick Workflow

### Build & Deploy (via SSH - no SD card needed!)

```bash
# 1. Build firmware
cd ~/rpi3_amp_project/rpi3_amp/rpi3_amp_core3
make clean && make

# 2. Deploy to RPi3
make deploy

# 3. Reboot to load new firmware
ssh admin@rpi3-amp 'sudo reboot'

# 4. Watch UART output
screen /dev/ttyUSB0 115200
```

### Read Core 3 Status from Linux

```bash
# On RPi3:
sudo read_shared_mem          # One-time status
sudo read_shared_mem -w       # Watch mode (continuous)
```

---

## 📊 Project Phases

```
✅ Phase 1: Planning & Setup
✅ Phase 2: Memory Reservation (Device Tree)
✅ Phase 3A: Userspace Launcher (Failed - cache issues)
✅ Phase 3B: U-Boot Boot (SUCCESS! 🎉)
🎯 Phase 4: Simple IPC ← YOU ARE HERE
   ├── ✅ Shared Memory Status Structure
   ├── ✅ Linux Reader Tool
   ├── ✅ Real Timestamps (System Timer)
   ├── ✅ Periodic Heartbeat
   └── ⏳ Mailbox-based IPC (Next)
⏳ Phase 5: OpenAMP/RPMsg
⏳ Phase 6: FreeRTOS Integration
```

---

## 🔧 Core 3 Firmware Features

### Current Features (v1.0.0)
| Feature | Status | Description |
|---------|--------|-------------|
| **UART Output** | ✅ | ASCII banner, formatted output |
| **System Timer** | ✅ | Real timestamps (HH:MM:SS.mmm) |
| **Shared Memory** | ✅ | Status struct at 0x20A00000 |
| **Heartbeat** | ✅ | Every 5 seconds |
| **SSH Deploy** | ✅ | `make deploy` workflow |

### Shared Memory Status Structure
```c
typedef struct {
    uint32_t magic;             // 0x52503341 ("RP3A")
    uint32_t version;           // Firmware version
    uint32_t core3_state;       // RUNNING, ERROR, etc.
    uint32_t boot_count;        // Number of boots
    uint64_t uptime_ticks;      // Uptime in µs
    uint32_t heartbeat_counter; // Heartbeat count
    uint32_t memtest_status;    // Memory test result
    char debug_message[128];    // Debug string
} shared_status_t;
```

### Known Issues (to fix later)
- `cpu_info.c` disabled (crashes due to EL2 register access)
- `wfe` instruction causes crash (using busy-wait instead)
- Memory test disabled for now

---

## 🎯 Next Steps

### Immediate (Phase 4 Completion)
1. **Re-enable CPU Info** - Fix register access for EL2
2. **Re-enable Memory Test** - Verify shared memory integrity
3. **Bidirectional IPC** - Linux writes commands, Core 3 responds

### Near-term (Phase 5)
1. **ARM Local Mailbox IPC** - Interrupt-driven communication
2. **Port libmetal** - Hardware abstraction for OpenAMP
3. **Port OpenAMP** - Full IPC framework

### Long-term (Phase 6)
1. **FreeRTOS Integration** - RTOS on Core 3
2. **RPMsg Channels** - Standard Linux IPC
3. **Production-ready AMP** - Like TImada's RPi4 implementation

---

## 💡 Quick Commands

```bash
# SSH to RPi
ssh admin@rpi3-amp

# UART Monitor
screen /dev/ttyUSB0 115200

# Build Core 3 firmware
cd rpi3_amp/rpi3_amp_core3 && make clean && make

# Deploy to RPi3
make deploy

# Deploy and reboot
make deploy-reboot

# Read Core 3 status (on RPi)
sudo read_shared_mem
sudo read_shared_mem -w   # Watch mode
```

---

## 🐛 Troubleshooting

### Core 3 doesn't start after deploy:
1. Reboot the RPi3: `ssh admin@rpi3-amp 'sudo reboot'`
2. Check UART for boot messages
3. Verify binary was deployed: `ssh admin@rpi3-amp 'ls -la /boot/firmware/core3_amp.bin'`

### No heartbeats after first one:
- Fixed in current version (replaced `wfe` with busy-wait)
- Rebuild and redeploy: `make clean && make && make deploy-reboot`

### `read_shared_mem` shows invalid magic:
- Core 3 firmware not running or crashed
- Reboot RPi3 and check UART for errors

### Linux doesn't boot:
1. Restore kernel: `ssh admin@rpi3-amp 'sudo cp /boot/firmware/kernel8.img.backup /boot/firmware/kernel8.img'`
2. Remove boot script: `ssh admin@rpi3-amp 'sudo rm /boot/firmware/boot.scr'`
3. Reboot

---

## 📚 Documentation

| File | Description |
|------|-------------|
| `CLAUDE.md` | Project overview, architecture |
| `CURRENT_STATUS.md` | This file - quick start |
| `PHASE3B_SUCCESS.md` | U-Boot boot method details |
| `quick_reference_card.md` | Hardware addresses |
| `ERRATA_CRITICAL_FIXES.md` | Known issues |

---

**Ready to continue! 🚀**

Core 3 is running with shared memory IPC. Next: Mailbox-based bidirectional communication!
