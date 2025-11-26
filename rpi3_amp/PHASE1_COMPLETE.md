# ✅ Phase 1 Abgeschlossen - Linux wiederherstellen & AMP vorbereiten

**Datum:** 2025-11-23
**Status:** ✅ **ERFOLGREICH ABGESCHLOSSEN**

---

## 🎯 Phase 1 Ziele

- ✅ Linux Kernel wiederhergestellt
- ✅ Bare-Metal Binary gesichert
- ✅ **KRITISCH:** UART2-Fehler in Dokumentation entdeckt und korrigiert!
- ✅ UART-Konflikt analysiert → Lösung: UART0 exklusiv für Bare-Metal
- ✅ Bare-Metal Code für AMP angepasst (0x20000000)
- ✅ Detaillierte Konfigurationsanleitungen erstellt

---

## 📊 Durchgeführte Arbeiten

### 1. UART2-Recherche - Kritischer Fund! 🚨

**Problem entdeckt:**
- Projekt-Dokumentation erwähnte UART2 (GPIO 0/1) für Bare-Metal
- **UART2 existiert NICHT auf RPi3 (BCM2837)!**
- UART2-5 gibt es nur auf RPi4 (BCM2711)

**RPi3 hat nur:**
- UART0 (PL011) - GPIO 14/15, ALT0
- UART1 (Mini UART) - GPIO 14/15, ALT5

**Korrigierte Dokumentation:**
- ✅ `CLAUDE.md` - UART-Sektion aktualisiert
- ✅ `ERRATA_CRITICAL_FIXES.md` - Detaillierte Korrektur mit RPi3 vs RPi4 Vergleich
- ✅ `rpi3_amp_documentation.md` - Debugging-Sektion korrigiert

### 2. UART-Konflikt Analyse & Lösung

**Problem:**
```
Linux (Core 0-2) → UART0 Console
Bare-Metal (Core 3) → UART0 Debug
                ↓
           KONFLIKT!
```

**Evaluierte Optionen:**
- **Option A:** UART0 exklusiv für Bare-Metal → ✅ GEWÄHLT
- **Option B:** UART1 (Mini UART) für Bare-Metal → ⚠️ Instabil
- **Option C:** Kein UART für Bare-Metal → ❌ Schlechtes Debugging

**Gewählte Lösung:**
- Linux UART Console deaktivieren (`cmdline.txt`)
- Bare-Metal nutzt UART0 exklusiv
- Linux Debugging über SSH

### 3. Konfigurationsanleitungen erstellt

**Neue Datei:** `rpi3_amp/AMP_CONFIGURATION_GUIDE.md`

**Inhalt:**
- Schritt-für-Schritt SD-Karte Konfiguration
- `cmdline.txt` Anpassung (UART Console deaktivieren, `maxcpus=3`)
- `config.txt` Anpassung (`enable_uart=1`)
- Hardware-Verkabelung UART
- Troubleshooting-Tipps
- Test-Prozeduren

### 4. Bare-Metal Code für AMP angepasst

**Neues Verzeichnis:** `rpi3_amp/rpi3_amp_core3/`

**Änderungen:**

#### link.ld
```diff
-. = 0x80000;          /* Standalone Boot */
+. = 0x20000000;       /* AMP Reserved Memory */
```

#### boot.S
```diff
-cbz     x0, core0_start    /* Core 0 */
+cmp     x0, #3              /* Core 3 only */
+bne     core_halt
```

#### main.c
```diff
-"*** UART0 Bare-Metal Test ***"
+"*** RPi3 AMP - Core 3 Bare-Metal ***"
+"Running at: 0x20000000 (AMP Reserved)"
```

**Build-Ergebnis:**
```bash
$ make
✅ kernel8.elf  - ELF mit Symbolen
✅ kernel8.img  - Raw Binary
✅ core3_amp.bin - AMP-Ready Binary

$ size kernel8.elf
   text    data     bss     dec     hex filename
    844       0    4096    4940    134c kernel8.elf

$ objdump -h kernel8.elf | grep .text
.text  0000034c  0000000020000000  # ✅ Läuft bei 0x20000000!
```

### 5. Dokumentation erstellt

**Neue/Aktualisierte Dateien:**

| Datei | Zweck | Status |
|-------|-------|--------|
| `AMP_CONFIGURATION_GUIDE.md` | Linux Config für AMP | ✅ Neu |
| `rpi3_amp_core3/README.md` | AMP Bare-Metal Doku | ✅ Neu |
| `CLAUDE.md` | UART-Korrektur | ✅ Aktualisiert |
| `ERRATA_CRITICAL_FIXES.md` | UART2-Fehler dokumentiert | ✅ Aktualisiert |
| `rpi3_amp_documentation.md` | Debugging-Sektion | ✅ Aktualisiert |

### 6. Hardware-Tests durchgeführt & Problem gelöst! ✅

**SD-Karte konfiguriert:**
- ✅ `cmdline.txt`: `console=` entfernt, `maxcpus=3` hinzugefügt
- ✅ `config.txt`: `dtoverlay=disable-bt`, `enable_uart=1`

**Problem entdeckt:** Linux Messages erschienen trotzdem auf UART!

**Ursache gefunden:** systemd `serial-getty@ttyAMA0.service` startete automatisch

**Lösung implementiert:**
```bash
sudo systemctl stop serial-getty@ttyAMA0.service
sudo systemctl disable serial-getty@ttyAMA0.service
sudo systemctl mask serial-getty@ttyAMA0.service
```

**Verifiziert nach Reboot:**
```
✅ Cores: 3 (0, 1, 2) - Core 3 isoliert
✅ UART Device existiert (/dev/ttyAMA0)
✅ Serial Getty: masked & disabled
✅ UART ist frei (kein Prozess nutzt es)
✅ Bluetooth: inaktiv
```

**Status:** 🎉 **Phase 1 WIRKLICH abgeschlossen!**

---

## 📁 Datei-Übersicht

```
rpi3_amp_project/
├── CLAUDE.md                           # ✅ UART-Sektion korrigiert
├── ERRATA_CRITICAL_FIXES.md            # ✅ UART2-Fehler dokumentiert
├── rpi3_amp_documentation.md           # ✅ Debugging aktualisiert
└── rpi3_amp/
    ├── AMP_CONFIGURATION_GUIDE.md      # ✅ NEU - Linux Config
    ├── PHASE1_COMPLETE.md              # ✅ NEU - Diese Datei
    ├── rpi3_uart_test/                 # Original standalone Test
    │   ├── core3_uart_test.bin         # ✅ Gesichert
    │   └── ...
    └── rpi3_amp_core3/                 # ✅ NEU - AMP Version
        ├── README.md                   # Detaillierte Doku
        ├── boot.S                      # Core 3 Filter
        ├── link.ld                     # 0x20000000
        ├── main.c                      # AMP Messages
        ├── Makefile                    # Build-System
        ├── kernel8.elf                 # ✅ Build Output
        └── core3_amp.bin               # ✅ AMP-Ready Binary
```

---

## 🧪 Nächste Schritte (Phase 2)

### Sofort durchführbar (auf SD-Karte):

**1. Linux UART Console deaktivieren**
```bash
# cmdline.txt bearbeiten:
# ENTFERNEN: console=serial0,115200 console=tty1
# HINZUFÜGEN: maxcpus=3
```

**2. UART aktivieren**
```bash
# config.txt bearbeiten:
# HINZUFÜGEN: enable_uart=1
```

**3. Test: Linux bootet?**
```bash
# Nach Boot via SSH:
cat /proc/cpuinfo | grep processor
# Erwartung: 0, 1, 2 (nicht 3)
```

### Als nächstes (Phase 2 - Device Tree):

- [ ] Device Tree Overlay erstellen (Memory Reservation)
- [ ] Overlay kompilieren und installieren
- [ ] Memory Reservation verifizieren (`/proc/iomem`)
- [ ] UIO Device für Shared Memory vorbereiten

### Später (Phase 3 - Core Wakeup):

- [ ] `core3_launcher` Tool schreiben (C)
- [ ] Core 3 per Mailbox aufwecken
- [ ] Testen: Linux + Bare-Metal parallel!

---

## 🎓 Lessons Learned

### 1. **Dokumentation blindlings vertrauen ist gefährlich!**

Die Original-Dokumentation erwähnte UART2 für RPi3, obwohl das Hardware-technisch unmöglich ist. **Immer verifizieren!**

### 2. **Hardware-Unterschiede RPi3 vs RPi4 sind signifikant**

| Feature | RPi3 | RPi4 |
|---------|------|------|
| UARTs | 2 | 6 |
| IRQ Controller | ARM Local | GIC-400 |
| Peripheral Base | 0x3F000000 | 0xFE000000 |

→ **Portierung ist nicht trivial!**

### 3. **AMP benötigt sorgfältige Planung**

- UART-Konflikte müssen gelöst werden
- Memory Reservation ist kritisch
- Core-Isolation muss korrekt konfiguriert werden

### 4. **systemd kann cmdline.txt überschreiben!**

Auch mit korrekter `cmdline.txt` (ohne `console=serial0`) startete systemd automatisch `serial-getty@ttyAMA0.service`!

**Lösung:**
- Services müssen explizit **masked** werden (nicht nur disabled)
- `systemctl mask` erstellt Symlink zu `/dev/null`
- Verhindert dass der Service jemals gestartet wird

**Wichtig für AMP:** Hardware-Konfiguration alleine reicht nicht - systemd muss auch konfiguriert werden!

### 5. **Bluetooth auf RPi3 blockiert UART0!**

- Standard: Bluetooth nutzt UART0 (PL011)
- Standard: Mini UART auf GPIO 14/15 (instabil)
- **Lösung:** `dtoverlay=disable-bt` mappt UART0 auf GPIO 14/15
- Für AMP: Bluetooth ist nicht kritisch → deaktivieren OK!

---

## ✅ Checkliste Phase 1

### Abgeschlossen:
- [x] Linux Kernel wiederhergestellt
- [x] Bare-Metal Binary gesichert (`core3_uart_test.bin`, `core3_amp.bin`)
- [x] UART2-Fehler entdeckt und dokumentiert
- [x] UART-Konflikt analysiert und Lösung gewählt
- [x] Dokumentation korrigiert (CLAUDE.md, ERRATA, etc.)
- [x] AMP Configuration Guide erstellt (mit systemd fixes)
- [x] Bare-Metal Code für 0x20000000 angepasst
- [x] Build erfolgreich (core3_amp.bin)
- [x] README für AMP Core 3 erstellt
- [x] **SD-Karte konfiguriert** (cmdline.txt, config.txt)
- [x] **Bluetooth deaktiviert** (dtoverlay=disable-bt)
- [x] **serial-getty Service deaktiviert** (masked)
- [x] **Linux Boot mit maxcpus=3 getestet** ✅
- [x] **UART0 frei verifiziert** ✅

### Bereit für Phase 2:
- [x] SD-Karte konfiguriert ✅
- [x] Linux Boot mit maxcpus=3 funktioniert ✅
- [x] UART0 ist frei ✅
- [ ] Device Tree Overlay erstellen ⏳ NEXT!

---

## 📈 Fortschritt Week 1

```
Week 1 Roadmap:
├── Tag 1-2: Setup & Bare-Metal Test     ✅ DONE
├── Tag 3: Linux + Bare-Metal Koexistenz ⏸️ IN PROGRESS
│   ├── Phase 1: Linux wiederherstellen  ✅ DONE
│   ├── Phase 2: Device Tree Overlay     ⏳ NEXT
│   └── Phase 3: Core 3 Wakeup           ⏳ TODO
├── Tag 4-5: libmetal Portierung         ⏳ TODO
└── Tag 6-7: OpenAMP platform_info       ⏳ TODO
```

**Aktueller Stand:** Phase 1 ✅ → Phase 2 ⏳

---

## 🎉 Erfolge

1. ✅ **Kritischen Dokumentationsfehler entdeckt** (UART2)
2. ✅ **UART-Konflikt elegant gelöst**
3. ✅ **AMP-ready Code erstellt** (0x20000000)
4. ✅ **Umfassende Dokumentation geschrieben**
5. ✅ **Build-System funktioniert**

---

## 📝 Notizen für Phase 2

**Device Tree Overlay Anforderungen:**

```dts
reserved-memory {
    amp_code@20000000 {
        reg = <0x20000000 0x00A00000>;  // 10 MB Code
        no-map;
    };
    amp_shared@20A00000 {
        reg = <0x20A00000 0x00200000>;  // 2 MB Shared
        no-map;
    };
};
```

**Wichtig:**
- `no-map` verhindert dass Linux den Speicher nutzt
- Adressen müssen mit `link.ld` übereinstimmen
- Größe checken: `core3_amp.bin` ist ~1 KB, passt locker in 10 MB

---

**Phase 1 Status:** ✅ **COMPLETE**
**Bereit für:** Phase 2 - Device Tree Configuration
**Nächster Schritt:** SD-Karte konfigurieren und Linux Boot testen

---

*Dokumentiert von Claude Code - 2025-11-23*
