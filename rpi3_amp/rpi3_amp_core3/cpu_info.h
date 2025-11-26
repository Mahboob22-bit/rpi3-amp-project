/**
 * @file cpu_info.h
 * @brief CPU-Information und Register-Zugriff für ARMv8-A
 * 
 * Liest verschiedene System-Register aus, um CPU-Informationen
 * wie Core-ID, Exception Level und Features anzuzeigen.
 */

#ifndef CPU_INFO_H
#define CPU_INFO_H

#include "common.h"

/*============================================================================
 * CPU Info Struktur
 *============================================================================*/

typedef struct {
    uint32_t core_id;           /* CPU Core ID (0-3) */
    uint32_t cluster_id;        /* Cluster ID */
    uint32_t exception_level;   /* Aktuelles EL (1, 2 oder 3) */
    uint32_t implementer;       /* CPU Implementer (ARM = 0x41) */
    uint32_t variant;           /* Variante */
    uint32_t architecture;      /* Architektur */
    uint32_t part_number;       /* Part Number (Cortex-A53 = 0xD03) */
    uint32_t revision;          /* Revision */
    uint64_t mpidr;             /* Full MPIDR_EL1 */
    uint64_t midr;              /* Full MIDR_EL1 */
} cpu_info_t;

/*============================================================================
 * Funktionen
 *============================================================================*/

/**
 * @brief Liest alle CPU-Informationen aus
 * @param info Pointer zur cpu_info_t Struktur
 */
void cpu_get_info(cpu_info_t *info);

/**
 * @brief Gibt alle CPU-Informationen über UART aus
 */
void cpu_print_info(void);

/**
 * @brief Gibt die Core-ID zurück (0-3)
 * @return Core-ID
 */
uint32_t cpu_get_core_id(void);

/**
 * @brief Gibt das aktuelle Exception Level zurück
 * @return Exception Level (1, 2 oder 3)
 */
uint32_t cpu_get_exception_level(void);

/**
 * @brief Gibt den Namen des CPU-Typs zurück
 * @param part_number Part Number aus MIDR
 * @return String mit CPU-Namen
 */
const char* cpu_get_name(uint32_t part_number);

/**
 * @brief Prüft ob wir auf Core 3 laufen
 * @return true wenn Core 3, sonst false
 */
bool cpu_is_core3(void);

/**
 * @brief Liest den System Counter (CNTPCT_EL0)
 * @return Counter-Wert
 */
uint64_t cpu_get_counter(void);

/**
 * @brief Liest die Counter-Frequenz (CNTFRQ_EL0)
 * @return Frequenz in Hz
 */
uint32_t cpu_get_counter_freq(void);

#endif /* CPU_INFO_H */

