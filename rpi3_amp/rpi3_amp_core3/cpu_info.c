/**
 * @file cpu_info.c
 * @brief CPU-Information Implementierung
 */

#include "cpu_info.h"
#include "uart.h"

/*============================================================================
 * ARM System Register Zugriff (inline assembly)
 *============================================================================*/

static inline uint64_t read_mpidr_el1(void) {
    uint64_t val;
    asm volatile("mrs %0, mpidr_el1" : "=r"(val));
    return val;
}

static inline uint64_t read_midr_el1(void) {
    uint64_t val;
    asm volatile("mrs %0, midr_el1" : "=r"(val));
    return val;
}

static inline uint32_t read_currentel(void) {
    uint64_t val;
    asm volatile("mrs %0, CurrentEL" : "=r"(val));
    return (uint32_t)((val >> 2) & 0x3);
}

static inline uint64_t read_cntpct_el0(void) {
    uint64_t val;
    asm volatile("mrs %0, cntpct_el0" : "=r"(val));
    return val;
}

static inline uint32_t read_cntfrq_el0(void) {
    uint64_t val;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(val));
    return (uint32_t)val;
}

/*============================================================================
 * CPU Namen Tabelle
 *============================================================================*/

typedef struct {
    uint32_t part_number;
    const char *name;
} cpu_name_entry_t;

static const cpu_name_entry_t cpu_names[] = {
    { 0xD03, "Cortex-A53" },
    { 0xD04, "Cortex-A35" },
    { 0xD05, "Cortex-A55" },
    { 0xD07, "Cortex-A57" },
    { 0xD08, "Cortex-A72" },
    { 0xD09, "Cortex-A73" },
    { 0xD0A, "Cortex-A75" },
    { 0xD0B, "Cortex-A76" },
    { 0xD0C, "Neoverse-N1" },
    { 0xD40, "Neoverse-V1" },
    { 0xD41, "Cortex-A78" },
    { 0xD44, "Cortex-X1" },
    { 0, NULL }
};

/*============================================================================
 * Implementierung
 *============================================================================*/

void cpu_get_info(cpu_info_t *info) {
    uint64_t mpidr = read_mpidr_el1();
    uint64_t midr = read_midr_el1();
    
    info->mpidr = mpidr;
    info->midr = midr;
    
    /* MPIDR: Affinity Level 0 = Core ID */
    info->core_id = mpidr & 0xFF;
    info->cluster_id = (mpidr >> 8) & 0xFF;
    
    /* CurrentEL */
    info->exception_level = read_currentel();
    
    /* MIDR Felder */
    info->implementer = (midr >> 24) & 0xFF;
    info->variant = (midr >> 20) & 0xF;
    info->architecture = (midr >> 16) & 0xF;
    info->part_number = (midr >> 4) & 0xFFF;
    info->revision = midr & 0xF;
}

uint32_t cpu_get_core_id(void) {
    return read_mpidr_el1() & 0x3;
}

uint32_t cpu_get_exception_level(void) {
    return read_currentel();
}

bool cpu_is_core3(void) {
    return (cpu_get_core_id() == 3);
}

uint64_t cpu_get_counter(void) {
    return read_cntpct_el0();
}

uint32_t cpu_get_counter_freq(void) {
    return read_cntfrq_el0();
}

const char* cpu_get_name(uint32_t part_number) {
    for (int i = 0; cpu_names[i].name != NULL; i++) {
        if (cpu_names[i].part_number == part_number) {
            return cpu_names[i].name;
        }
    }
    return "Unknown";
}

void cpu_print_info(void) {
    cpu_info_t info;
    cpu_get_info(&info);
    
    uart_puts("\n");
    uart_puts("╔════════════════════════════════════════╗\n");
    uart_puts("║           CPU INFORMATION              ║\n");
    uart_puts("╠════════════════════════════════════════╣\n");
    
    /* CPU Typ */
    uart_puts("║ CPU Type    : ");
    uart_puts(cpu_get_name(info.part_number));
    uart_puts("\n");
    
    /* Core ID */
    uart_puts("║ Core ID     : ");
    uart_put_uint(info.core_id);
    if (info.core_id == 3) {
        uart_puts(" (AMP Remote)");
    }
    uart_puts("\n");
    
    /* Cluster ID */
    uart_puts("║ Cluster ID  : ");
    uart_put_uint(info.cluster_id);
    uart_puts("\n");
    
    /* Exception Level */
    uart_puts("║ Exception   : EL");
    uart_put_uint(info.exception_level);
    switch (info.exception_level) {
        case 1: uart_puts(" (OS/Kernel)"); break;
        case 2: uart_puts(" (Hypervisor)"); break;
        case 3: uart_puts(" (Secure Monitor)"); break;
        default: uart_puts(" (User)"); break;
    }
    uart_puts("\n");
    
    /* Implementer */
    uart_puts("║ Implementer : ");
    uart_put_hex32(info.implementer);
    if (info.implementer == 0x41) {
        uart_puts(" (ARM Ltd)");
    }
    uart_puts("\n");
    
    /* Part Number */
    uart_puts("║ Part Number : ");
    uart_put_hex32(info.part_number);
    uart_puts("\n");
    
    /* Revision */
    uart_puts("║ Revision    : r");
    uart_put_uint(info.variant);
    uart_puts("p");
    uart_put_uint(info.revision);
    uart_puts("\n");
    
    /* Raw Registers */
    uart_puts("╠════════════════════════════════════════╣\n");
    uart_puts("║ MPIDR_EL1   : ");
    uart_put_hex64(info.mpidr);
    uart_puts("\n");
    uart_puts("║ MIDR_EL1    : ");
    uart_put_hex64(info.midr);
    uart_puts("\n");
    
    /* Counter Info */
    uint32_t freq = cpu_get_counter_freq();
    uart_puts("║ Counter Frq : ");
    uart_put_uint(freq / 1000000);
    uart_puts(" MHz\n");
    
    uart_puts("╚════════════════════════════════════════╝\n");
}

