# 🚨 ERRATA - Kritische Fehler im RPi3 AMP Plan

## ❌ FEHLER #1: ACT LED Control (KRITISCH!)

### Das Problem
**In unseren Dokumenten steht:**
```c
#define ACT_LED_PIN  47  // RPi3 ACT LED
// GPIO 47 direkt setzen...
*GPSET1 = (1 << (47-32));  // ❌ FUNKTIONIERT NICHT!
```

### Die Wahrheit
**Auf Raspberry Pi 3 Model B:**
- ACT LED ist **NICHT** direkt über GPIO 47 steuerbar!
- Die LED ist über einen **I2C GPIO Expander** (U20) angeschlossen
- Der Expander sitzt nahe dem DSI Connector
- **NUR über GPU Mailbox Property Interface steuerbar!**

### Betroffene Hardware
- ✅ **RPi Zero / Zero W:** GPIO 47 funktioniert
- ❌ **RPi 3 Model B:** Nur via Mailbox!
- ❌ **RPi 3 Model B+:** Nur via Mailbox!
- ✅ **RPi 4:** GPIO Control möglich (aber andere Adressen)

### Die richtige Lösung für RPi3

#### Option A: GPU Mailbox (kompliziert für Bare-Metal)
```c
// Property Tag: SET_GPIO_STATE
// Tag ID: 0x00038041
// GPIO 130 = ACT LED (virtuelle GPIO!)

struct mailbox_property {
    uint32_t tag_id;      // 0x00038041
    uint32_t buffer_size; // 8
    uint32_t req_resp;    // 8 (request) / 8 (response)
    uint32_t gpio;        // 130 (ACT LED)
    uint32_t state;       // 0 = OFF, 1 = ON
};
```

#### Option B: Externe LED nutzen (EMPFOHLEN für Testing!)
```c
// Einfach einen anderen GPIO Pin verwenden!
#define TEST_LED_PIN  17  // GPIO 17 = Physical Pin 11

// GPIO 17 ist direkt über GPIO Controller steuerbar
void init_test_led(void) {
    uint32_t *GPFSEL1 = (uint32_t*)0x3F200004;
    uint32_t ra = *GPFSEL1;
    ra &= ~(7 << 21);  // Clear GPIO 17 function
    ra |= (1 << 21);   // Set as output
    *GPFSEL1 = ra;
}

void led_on(void) {
    uint32_t *GPSET0 = (uint32_t*)0x3F20001C;
    *GPSET0 = (1 << 17);
}

void led_off(void) {
    uint32_t *GPCLR0 = (uint32_t*)0x3F200028;
    *GPCLR0 = (1 << 17);
}
```

---

## ⚠️ FEHLER #2: Mailbox Property Interface für LED

### Das Problem
Unser Plan nutzt ARM Local Mailboxes (Core-to-Core), aber ACT LED braucht GPU Property Mailbox!

### Zwei verschiedene Mailbox-Systeme

#### 1. ARM Local Mailboxes (Core-to-Core IPC)
```
Base: 0x40000000
Core 0 MB3 SET: 0x40000080
Core 1 MB3 SET: 0x40000090
Core 2 MB3 SET: 0x400000A0
Core 3 MB3 SET: 0x400000B0

→ Für AMP/OpenAMP/RPMsg: ✅ KORREKT
```

#### 2. GPU Property Mailbox (ARM ↔ VideoCore)
```
Base: 0x3F00B880
Mailbox 0 Read:   0x3F00B880
Mailbox 0 Status: 0x3F00B898
Mailbox 0 Write:  0x3F00B8A0

→ Für ACT LED Control: ✅ BENÖTIGT (aber kompliziert)
```

### Workaround
**Für frühe Tests: Externe LED verwenden!**

---

## ⚠️ FEHLER #3: Device Tree CPU Disable

### Das Problem
```dts
fragment@1 {
    target-path = "/cpus";
    __overlay__ {
        cpu@3 {
            status = "disabled";  // ❌ Könnte nicht reichen!
        };
    };
};
```

### Besserer Ansatz
```dts
fragment@1 {
    target-path = "/cpus";
    __overlay__ {
        cpu@3 {
            device_type = "cpu";
            status = "disabled";
        };
    };
};

// ODER besser noch: CPU gar nicht erst deaktivieren,
// sondern nur mit maxcpus=3 booten!
```

### Warum?
- `status = "disabled"` könnte von Linux ignoriert werden
- **Sicherer:** `maxcpus=3` in cmdline.txt
- Dann bleibt Core 3 im Spin-Loop und wartet auf Mailbox

---

## ⚠️ FEHLER #4: Memory Overlap Potential

### Das Problem
```
Linux RAM:           0x00000000 - 0x3EFFFFFF
Bare-Metal Code:     0x20000000 - 0x209FFFFF  // MITTEN IN LINUX RAM!
```

### Die Lösung
**Device Tree MUSS Memory reservieren:**
```dts
reserved-memory {
    #address-cells = <1>;
    #size-cells = <1>;
    ranges;
    
    amp_core3: amp_reserved@20000000 {
        compatible = "shared-dma-pool";
        reg = <0x20000000 0x1000000>;  // 16 MB
        no-map;  // ← KRITISCH! Linux darf hier NICHT zugreifen!
    };
};
```

**UND cmdline.txt:**
```
mem=512M  // Linux nur bis 0x20000000
```

---

## ⚠️ FEHLER #5: UART Pin Assignments - KRITISCHER FEHLER IN ORIGINALDOKU!

### ❌ Das Problem - UART2 existiert NICHT auf RPi3!
```c
// ❌ FALSCH - Diese Information ist nur für RPi4 gültig!
// GPIO 0/1 für UART2
#define UART2_TXD  0  // GPIO 0 = ALT4
#define UART2_RXD  1  // GPIO 1 = ALT4
```

### ✅ KORREKT: RPi3 (BCM2837) UART Mapping
**RPi3 hat NUR 2 UARTs:**
- **UART0 (PL011):** GPIO 14/15 (ALT0) ← Vollwertiger UART, Linux Console
- **UART1 (Mini UART):** GPIO 14/15 (ALT5) ← Reduzierte Features, instabil

**❌ UART2-5 EXISTIEREN NICHT auf BCM2837!**

### ✅ KORREKT: RPi4 (BCM2711) UART Mapping
**RPi4 hat 6 UARTs:**
- **UART0 (PL011):** GPIO 14/15 (ALT0)
- **UART1 (Mini UART):** GPIO 14/15 (ALT5)
- **UART2:** GPIO 0/1 (ALT4) ← NUR auf RPi4!
- **UART3:** GPIO 4/5 (ALT4) ← NUR auf RPi4!
- **UART4:** GPIO 8/9 (ALT4) ← NUR auf RPi4!
- **UART5:** GPIO 12/13 (ALT4) ← NUR auf RPi4!

### Empfehlung für RPi3 AMP Setup
```c
// ✅ OPTION 1: UART0 exklusiv für Bare-Metal (EMPFOHLEN)
// - Linux UART Console deaktivieren (cmdline.txt)
// - Bare-Metal nutzt UART0 (GPIO 14/15, ALT0)
// - Volle PL011 Features
// - Linux Debug über SSH

// ⚠️ OPTION 2: UART1 (Mini UART) für Bare-Metal
// - Linux behält UART0
// - Bare-Metal nutzt UART1 (GPIO 14/15, ALT5)
// - Limitierte Features, abhängig von VPU Clock
// - Instabil bei GPU-Last

// ❌ NICHT MÖGLICH: UART2-5 auf RPi3
// → Diese UARTs existieren nur auf RPi4!
```

---

## ⚠️ FEHLER #6: Cache Coherency ignoriert

### Das Problem
Unser Plan erwähnt Cache Management nur kurz, aber das ist KRITISCH für AMP!

### Das fehlende Stück
```c
// Shared Memory MUSS Cache-Coherent sein!

// OPTION 1: Shared Memory als Uncached markieren
// In MMU Config:
{
    .addr = 0x20A00000,  // Shared Memory
    .size = SIZE_2M,
    .executable = XN_ON,
    .sharable = OUTER_SHARABLE,  // ← WICHTIG!
    .permission = READ_WRITE,
    .policy = TYPE_MEM_UNCACHED,  // ← KRITISCH!
},

// OPTION 2: Manuelle Cache Flushes
void flush_dcache_range(void *addr, size_t size) {
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end = start + size;
    
    for (uintptr_t va = start; va < end; va += 64) {
        asm volatile("dc cvac, %0" : : "r"(va) : "memory");
    }
    asm volatile("dsb sy" ::: "memory");
}
```

### Cortex-A53 Cache Details
- **L1 DCache:** 32 KB per core
- **L1 ICache:** 48 KB per core (16KB + 32KB)
- **L2 Cache:** 512 KB (shared)
- **NO Hardware Cache Coherency ohne SCU config!**

---

## ✅ KORRIGIERTE Memory Map

```
0x00000000 - 0x1FFFFFFF  | 512 MB  | Linux (via mem=512M)
0x20000000 - 0x209FFFFF  |  10 MB  | Bare-Metal Code/Data (RESERVED!)
0x20A00000 - 0x20BFFFFF  |   2 MB  | Shared Memory IPC (UNCACHED!)
0x20C00000 - 0x2FFFFFFF  | 244 MB  | UNUSED (reserved von mem=512M)
0x30000000 - 0x3EFFFFFF  | 240 MB  | UNUSED
0x3F000000 - 0x3FFFFFFF  |  16 MB  | BCM2837 Peripherals
0x40000000 - 0x40000FFF  |   4 KB  | ARM Local Peripherals
```

---

## ✅ KORREKTUREN für Week 1 Plan

### Tag 2: LED Test - NEU

**STATT ACT LED:**
```c
// Verwende GPIO 17 mit externer LED!
// Hardware Setup:
// GPIO 17 (Pin 11) → 330Ω Resistor → LED Anode → LED Cathode → GND (Pin 6)

#define GPIO_BASE   0x3F200000
#define GPFSEL1     ((volatile uint32_t*)(GPIO_BASE + 0x04))
#define GPSET0      ((volatile uint32_t*)(GPIO_BASE + 0x1C))
#define GPCLR0      ((volatile uint32_t*)(GPIO_BASE + 0x28))

#define TEST_LED    17

void init_led(void) {
    uint32_t ra = *GPFSEL1;
    ra &= ~(7 << 21);  // GPIO 17: bits 21-23
    ra |= (1 << 21);   // Set as output
    *GPFSEL1 = ra;
}

void led_on(void)  { *GPSET0 = (1 << TEST_LED); }
void led_off(void) { *GPCLR0 = (1 << TEST_LED); }

void main(void) {
    init_led();
    
    while(1) {
        led_on();
        delay(1000000);
        led_off();
        delay(1000000);
    }
}
```

### Tag 4: libmetal Cache Management hinzufügen

```c
// In sys.c oder cache.c

void metal_cache_flush(void *addr, size_t size) {
    uintptr_t start = (uintptr_t)addr & ~63UL;
    uintptr_t end = ((uintptr_t)addr + size + 63) & ~63UL;
    
    for (uintptr_t va = start; va < end; va += 64) {
        asm volatile("dc cvac, %0" : : "r"(va));
    }
    
    asm volatile("dsb sy");
}

void metal_cache_invalidate(void *addr, size_t size) {
    uintptr_t start = (uintptr_t)addr & ~63UL;
    uintptr_t end = ((uintptr_t)addr + size + 63) & ~63UL;
    
    for (uintptr_t va = start; va < end; va += 64) {
        asm volatile("dc ivac, %0" : : "r"(va));
    }
    
    asm volatile("dsb sy");
    asm volatile("isb");
}
```

---

## 🔧 ZUSÄTZLICHE EMPFEHLUNGEN

### 1. Hardware Setup für Testing

**Benötigt:**
- RPi3 Model B
- MicroSD Card (16GB+)
- USB-UART Adapter (FTDI/CP2102)
- Breadboard + LED + 330Ω Resistor
- Jumper Wires

**Verbindungen:**
```
UART Debug:
  GPIO 14 (Pin 8)  → UART RX
  GPIO 15 (Pin 10) → UART TX
  GND (Pin 6)      → UART GND

Test LED:
  GPIO 17 (Pin 11) → 330Ω → LED+ → LED- → GND (Pin 6)
```

### 2. Bootloader Strategie

**Phase 1 Testing: Ohne U-Boot**
- Einfacher für erste Tests
- kernel8.img direkt booten
- cmdline.txt für maxcpus=3

**Phase 2 Production: Mit U-Boot**
- Mehr Kontrolle
- Firmware dynamisch laden
- Besseres Debugging

### 3. Alternative: RPi Zero W für LED Tests

**Falls verfügbar:**
- RPi Zero W hat ACT LED auf GPIO 47 direkt!
- Code-Kompatibilität testen
- Dann auf RPi3 migrieren

### 4. Debugging-Strategie NEU

**Level 1: LED Blink**
- Externe LED auf GPIO 17
- Zeigt: Code läuft

**Level 2: UART Output**
- UART2 auf GPIO 0/1
- Zeigt: Peripherals funktionieren

**Level 3: Mailbox Test**
- Core 0 ↔ Core 3
- Zeigt: Multi-Core funktioniert

**Level 4: OpenAMP IPC**
- Linux ↔ Bare-Metal
- Zeigt: Vollständige Integration

---

## 📋 NEUE Checkliste

### ✅ Hardware Vorbereitung
- [ ] RPi3 Model B vorhanden
- [ ] USB-UART Adapter da
- [ ] Breadboard + LED + Resistor besorgt
- [ ] Jumper Wires da

### ✅ Software Vorbereitung
- [ ] Externe LED Test-Code vorbereitet
- [ ] Cache-Management Code hinzugefügt
- [ ] Device Tree korrekt (no-map!)
- [ ] Memory Map dokumentiert

### ✅ Testing Approach
- [ ] Phase 1: Externe LED (GPIO 17)
- [ ] Phase 2: UART Debug
- [ ] Phase 3: Mailbox Test
- [ ] Phase 4: OpenAMP IPC

---

## 🎯 PRIORITÄTEN NEU

### MUSS (MVP)
1. ✅ Externe LED blinkt auf Core 3
2. ✅ UART Debug funktioniert
3. ✅ Mailbox Core 0 ↔ Core 3
4. ✅ Linux bootet mit 3 Cores

### SOLLTE
5. ✅ Cache-Coherency implementiert
6. ✅ Shared Memory korrekt
7. ✅ Device Tree polished

### KANN
8. ⭐ ACT LED via GPU Mailbox (later)
9. ⭐ Performance Optimization
10. ⭐ Power Management

---

## 📚 ZUSÄTZLICHE DOKUMENTATION

### Cache Management
- ARM Cortex-A53 TRM: Section 11 "L1 Memory System"
- ARMv8 ARM: Section D4.4 "Cache maintenance operations"

### GPIO Expander (für ACT LED später)
- RPi3 Schematic (community reverse-engineered)
- GPIO Virtual Tag: BCM2835 Mailbox Property Interface

### MMU Configuration
- ARM Cortex-A53 MPCore TRM
- Memory Attributes für Shared Memory

---

## ⚠️ BEKANNTE EINSCHRÄNKUNGEN

### Was NICHT geht (einfach)
1. ❌ ACT LED direkt über GPIO (needs Mailbox)
2. ❌ Hardware Cache Coherency (needs SW management)
3. ❌ Hot-Plug CPU (needs advanced setup)
4. ❌ Power LED control (expander)

### Was GUT geht
1. ✅ Externe LEDs über GPIO
2. ✅ UART für Debug
3. ✅ ARM Local Mailboxes
4. ✅ Shared Memory (mit Cache mgmt)
5. ✅ OpenAMP/RPMsg

---

---

## ⚠️ FEHLER #7: WFE Instruction Crash (2025-11-26)

### Das Problem
```c
// ❌ CRASH in bare-metal main loop!
while (1) {
    // ... heartbeat code ...
    asm volatile("wfe");  // ← CRASH nach erstem Heartbeat!
}
```

### Die Wahrheit
- `wfe` (Wait For Event) braucht korrekt konfigurierte Exception Handler
- Ohne Exception Handler führt jede Exception zum Systemabsturz
- Core 3 läuft ohne MMU/Exception Setup → `wfe` ist gefährlich

### Die Lösung
```c
// ✅ WORKAROUND: Busy-wait statt WFE
while (1) {
    // ... heartbeat code ...
    
    // Kurze Pause ohne WFE
    for (volatile int i = 0; i < 10000; i++) {
        asm volatile("nop");
    }
}
```

### Langfristige Lösung (TODO)
- Exception Handler implementieren
- Timer-basiertes Warten
- FreeRTOS Integration (hat eigenes Task-Scheduling)

---

## ⚠️ FEHLER #8: EL2 Register Access Crash (2025-11-26)

### Das Problem
```c
// ❌ CRASH bei Register-Zugriff!
static inline uint32_t read_currentel(void) {
    uint64_t val;
    asm volatile("mrs %0, CurrentEL" : "=r"(val));  // OK
    return (uint32_t)((val >> 2) & 0x3);
}

// ❌ Einige Register sind nicht von EL2 aus zugänglich
asm volatile("mrs %0, some_el1_register" : "=r"(val));  // CRASH!
```

### Die Wahrheit
- Core 3 läuft in **EL2** (Hypervisor Level), nicht EL1
- Viele System-Register sind EL-spezifisch
- Falsche Register-Zugriffe → Undefined Instruction Exception → Crash

### Die Lösung
```c
// ✅ WORKAROUND: cpu_info.c deaktiviert im Makefile
C_SRCS = \
    main.c \
    uart.c \
    timer.c \
    memory.c
    # cpu_info.c ← DEAKTIVIERT
```

### Langfristige Lösung (TODO)
- Register-Zugriffe für EL2 anpassen
- Oder zu EL1 wechseln vor Main-Code

---

## ⚠️ FEHLER #9: Uninitialisierter Shared Memory (2025-11-26)

### Das Problem
```c
// ❌ Boot Count zeigt Garbage-Wert!
g_status->boot_count++;  // 0x55555555 + 1 = 0x55555556
```

### Die Wahrheit
- Shared Memory ist beim Boot nicht initialisiert
- RAM enthält zufällige Werte (oft 0x55555555)
- Inkrementieren macht keinen Sinn

### Die Lösung
```c
// ✅ Erst Memory auf 0 setzen!
shared_status_t* shared_mem_init(void) {
    g_status = (shared_status_t *)SHARED_STATUS_ADDR;
    
    // Erst alles auf 0 setzen
    uint8_t *ptr = (uint8_t *)g_status;
    for (uint32_t i = 0; i < sizeof(shared_status_t); i++) {
        ptr[i] = 0;
    }
    
    // Dann initialisieren
    g_status->magic = FIRMWARE_MAGIC;
    g_status->boot_count = 1;  // Nicht inkrementieren!
    // ...
}
```

---

## ⚠️ FEHLER #10: Doppeltes "0x" in printf (2025-11-26)

### Das Problem
```c
// ❌ Gibt "0x0x20000000" aus!
uart_printf("Address: 0x%x\n", 0x20000000);
```

### Die Wahrheit
- `uart_put_hex32()` gibt bereits "0x" Prefix aus
- `%x` Format in printf ruft `uart_put_hex32()` auf
- Ergebnis: doppeltes "0x"

### Die Lösung
```c
// ✅ Option A: Kein "0x" im Format-String
uart_printf("Address: %x\n", 0x20000000);  // Gibt "0x20000000"

// ✅ Option B: Separater Aufruf
uart_puts("Address: ");
uart_put_hex32(0x20000000);
uart_puts("\n");
```

---

## 🚀 LOS GEHT'S - Mit Korrekturen!

**Start wieder bei Tag 1, aber mit:**
- ✅ Externe LED statt ACT LED
- ✅ Cache-Management vorbereitet
- ✅ Korrekte Memory-Map
- ✅ Realistische Erwartungen

**Du schaffst das trotzdem! 💪**

---

## 📞 Bei Fragen

**Dokumentation zeigen:**
- BCM2835 Peripherals PDF
- ARM Cortex-A53 TRM
- Dieser Errata!

**Mich fragen, wenn:**
- Cache-Coherency Probleme
- Mailbox nicht funktioniert
- Memory Map unklar
- Irgendwas anderes! 😊
