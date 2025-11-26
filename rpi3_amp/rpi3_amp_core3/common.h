/**
 * @file common.h
 * @brief Gemeinsame Definitionen für RPi3 AMP Core 3 Firmware
 * 
 * Dieses File enthält alle Hardware-Adressen und gemeinsam genutzten
 * Typdefinitionen für die modulare Firmware.
 * 
 * HINWEIS: Da wir -nostdinc verwenden, definieren wir alle Typen selbst!
 */

#ifndef COMMON_H
#define COMMON_H

/*============================================================================
 * Standard-Typdefinitionen (kein stdint.h in bare-metal!)
 *============================================================================*/

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

typedef unsigned long       uintptr_t;
typedef long                intptr_t;

typedef unsigned int        size_t;

typedef unsigned int        uint;
typedef int                 bool;
#define true                1
#define false               0

/* NULL Definition */
#ifndef NULL
#define NULL ((void *)0)
#endif

/*============================================================================
 * RPi3 BCM2837 Hardware-Adressen
 *============================================================================*/

/* Peripheral Base - KRITISCH: RPi3 = 0x3F000000, RPi4 = 0xFE000000! */
#define PERIPHERAL_BASE     0x3F000000

/* ARM Local Peripherals - Für Mailboxes und lokale Interrupts */
#define ARM_LOCAL_BASE      0x40000000

/* GPIO */
#define GPIO_BASE           (PERIPHERAL_BASE + 0x200000)

/* UART0 (PL011) - Hauptsächlicher Debug-UART */
#define UART0_BASE          (PERIPHERAL_BASE + 0x201000)

/* System Timer - 1 MHz Zähler */
#define SYSTIMER_BASE       (PERIPHERAL_BASE + 0x003000)

/*============================================================================
 * AMP Memory Map
 *============================================================================*/

/* Core 3 Code/Data Bereich */
#define AMP_CODE_BASE       0x20000000
#define AMP_CODE_SIZE       0x00A00000  /* 10 MB */

/* Shared Memory für IPC (Linux ↔ Core 3) */
#define SHARED_MEM_BASE     0x20A00000
#define SHARED_MEM_SIZE     0x00200000  /* 2 MB */

/*============================================================================
 * Shared Memory Layout
 *============================================================================*/

/* Status-Struktur am Anfang des Shared Memory */
#define SHARED_STATUS_ADDR      (SHARED_MEM_BASE + 0x0000)
#define SHARED_STATUS_SIZE      0x1000  /* 4 KB */

/* Daten-Bereich für IPC Messages */
#define SHARED_DATA_ADDR        (SHARED_MEM_BASE + 0x1000)
#define SHARED_DATA_SIZE        0x1000  /* 4 KB */

/* Memory Test Bereich */
#define SHARED_MEMTEST_ADDR     (SHARED_MEM_BASE + 0x2000)
#define SHARED_MEMTEST_SIZE     0x10000 /* 64 KB */

/*============================================================================
 * Magic Numbers und Versionen
 *============================================================================*/

#define FIRMWARE_MAGIC      0x52503341  /* "RP3A" in Little Endian */
#define FIRMWARE_VERSION    0x00010000  /* v1.0.0 */

/*============================================================================
 * Hilfsmakros
 *============================================================================*/

/* Volatile Pointer für MMIO */
#define REG32(addr)         (*(volatile uint32_t *)(addr))
#define REG64(addr)         (*(volatile uint64_t *)(addr))

/* Memory Barriers */
#define DMB()               asm volatile("dmb sy" ::: "memory")
#define DSB()               asm volatile("dsb sy" ::: "memory")
#define ISB()               asm volatile("isb" ::: "memory")

#endif /* COMMON_H */
