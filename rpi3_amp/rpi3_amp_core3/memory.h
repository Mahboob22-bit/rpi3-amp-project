/**
 * @file memory.h
 * @brief Memory Test und Shared Memory Management für AMP
 * 
 * Verwaltet den Shared Memory Bereich für IPC zwischen Linux und Core 3.
 * Bietet Memory-Tests zur Verifizierung der Speicherintegrität.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

/*============================================================================
 * Shared Memory Status Struktur
 * 
 * Diese Struktur liegt am Anfang des Shared Memory (0x20A00000)
 * und kann von Linux und Core 3 gelesen/geschrieben werden.
 *============================================================================*/

typedef struct {
    /* Header - Magic und Version zur Validierung */
    uint32_t magic;             /* FIRMWARE_MAGIC = "RP3A" */
    uint32_t version;           /* Firmware Version */
    
    /* Core 3 Status */
    uint32_t core3_state;       /* Aktueller Zustand (siehe CORE3_STATE_*) */
    uint32_t boot_count;        /* Anzahl Neustarts */
    uint64_t boot_time;         /* Boot-Zeitstempel (Timer Ticks) */
    uint64_t uptime_ticks;      /* Aktuelle Uptime (wird regelmäßig aktualisiert) */
    
    /* Heartbeat */
    uint32_t heartbeat_counter; /* Inkrementierende Zähler */
    uint32_t heartbeat_interval_ms;  /* Intervall in ms */
    
    /* Memory Test Ergebnisse */
    uint32_t memtest_status;    /* 0 = nicht gelaufen, 1 = OK, 2 = Fehler */
    uint32_t memtest_errors;    /* Anzahl Fehler */
    uint32_t memtest_bytes;     /* Getestete Bytes */
    
    /* IPC Statistiken */
    uint32_t messages_sent;     /* Von Core 3 gesendet */
    uint32_t messages_received; /* Von Core 3 empfangen */
    
    /* Reserviert für zukünftige Erweiterungen */
    uint32_t reserved[8];
    
    /* Debug String (null-terminiert) */
    char debug_message[128];
    
} shared_status_t;

/* Core 3 Zustände */
#define CORE3_STATE_BOOT        0   /* Booting */
#define CORE3_STATE_INIT        1   /* Initializing */
#define CORE3_STATE_RUNNING     2   /* Running (normal) */
#define CORE3_STATE_MEMTEST     3   /* Memory Test läuft */
#define CORE3_STATE_ERROR       4   /* Fehler aufgetreten */
#define CORE3_STATE_HALTED      5   /* Angehalten */

/*============================================================================
 * Memory Test Patterns
 *============================================================================*/

#define MEMTEST_PATTERN_ZEROS       0x00000000
#define MEMTEST_PATTERN_ONES        0xFFFFFFFF
#define MEMTEST_PATTERN_AA          0xAAAAAAAA
#define MEMTEST_PATTERN_55          0x55555555
#define MEMTEST_PATTERN_WALKING1    0x00000001
#define MEMTEST_PATTERN_DEADBEEF    0xDEADBEEF

/*============================================================================
 * Funktionen
 *============================================================================*/

/**
 * @brief Initialisiert das Shared Memory
 * @return Pointer zur Status-Struktur
 */
shared_status_t* shared_mem_init(void);

/**
 * @brief Aktualisiert die Uptime im Shared Memory
 */
void shared_mem_update_uptime(void);

/**
 * @brief Inkrementiert den Heartbeat Counter
 */
void shared_mem_heartbeat(void);

/**
 * @brief Setzt den Core 3 Status
 * @param state Neuer Zustand
 */
void shared_mem_set_state(uint32_t state);

/**
 * @brief Setzt eine Debug-Nachricht
 * @param msg Die Nachricht
 */
void shared_mem_set_debug(const char *msg);

/**
 * @brief Gibt den Pointer zur Status-Struktur zurück
 * @return Pointer zur shared_status_t
 */
shared_status_t* shared_mem_get_status(void);

/**
 * @brief Führt einen vollständigen Memory-Test durch
 * @param start_addr Startadresse
 * @param size Größe in Bytes
 * @param verbose true für UART-Ausgabe
 * @return Anzahl Fehler (0 = OK)
 */
uint32_t memory_test_full(uintptr_t start_addr, uint32_t size, bool verbose);

/**
 * @brief Schneller Memory-Test (nur Basis-Pattern)
 * @param start_addr Startadresse
 * @param size Größe in Bytes
 * @return Anzahl Fehler (0 = OK)
 */
uint32_t memory_test_quick(uintptr_t start_addr, uint32_t size);

/**
 * @brief Walking-Ones Test
 * @param start_addr Startadresse
 * @param size Größe in Bytes
 * @return Anzahl Fehler
 */
uint32_t memory_test_walking_ones(uintptr_t start_addr, uint32_t size);

/**
 * @brief Pattern-Test mit einem bestimmten Muster
 * @param start_addr Startadresse
 * @param size Größe in Bytes
 * @param pattern Das zu verwendende Muster
 * @return Anzahl Fehler
 */
uint32_t memory_test_pattern(uintptr_t start_addr, uint32_t size, uint32_t pattern);

/**
 * @brief Gibt die Memory-Map aus
 */
void memory_print_map(void);

/**
 * @brief Gibt den Shared Memory Status aus
 */
void memory_print_status(void);

#endif /* MEMORY_H */

