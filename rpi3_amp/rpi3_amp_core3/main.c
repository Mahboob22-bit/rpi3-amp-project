/**
 * @file main.c
 * @brief RPi3 AMP Core 3 Bare-Metal Firmware - Hauptprogramm
 * 
 * VEREINFACHTE VERSION - schrittweises Debugging
 */

#include "common.h"
#include "uart.h"
#include "timer.h"
#include "memory.h"

/* CPU Info vorerst deaktiviert - verursacht Crash */
/* #include "cpu_info.h" */

/*============================================================================
 * Konfiguration
 *============================================================================*/

#define HEARTBEAT_INTERVAL_MS   5000    /* 5 Sekunden */

/*============================================================================
 * Einfache CPU-ID Funktion (sicher)
 *============================================================================*/

static uint32_t get_core_id(void) {
    uint64_t mpidr;
    asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    return mpidr & 0x3;
}

/*============================================================================
 * Banner
 *============================================================================*/

static void print_banner(void) {
    uart_puts("\n");
    uart_puts("╔════════════════════════════════════════════════════════════╗\n");
    uart_puts("║                                                            ║\n");
    uart_puts("║   ██████╗ ██████╗ ██╗██████╗      █████╗ ███╗   ███╗██████╗║\n");
    uart_puts("║   ██╔══██╗██╔══██╗██║╚════██╗    ██╔══██╗████╗ ████║██╔══██║\n");
    uart_puts("║   ██████╔╝██████╔╝██║ █████╔╝    ███████║██╔████╔██║██████╔║\n");
    uart_puts("║   ██╔══██╗██╔═══╝ ██║ ╚═══██╗    ██╔══██║██║╚██╔╝██║██╔═══╝║\n");
    uart_puts("║   ██║  ██║██║     ██║██████╔╝    ██║  ██║██║ ╚═╝ ██║██║    ║\n");
    uart_puts("║   ╚═╝  ╚═╝╚═╝     ╚═╝╚═════╝     ╚═╝  ╚═╝╚═╝     ╚═╝╚═╝    ║\n");
    uart_puts("║                                                            ║\n");
    uart_puts("║        Asymmetric Multiprocessing - Core 3 Firmware        ║\n");
    uart_puts("║                                                            ║\n");
    uart_puts("╚════════════════════════════════════════════════════════════╝\n");
    uart_puts("\n");
}

/*============================================================================
 * Heartbeat
 *============================================================================*/

static void print_heartbeat(uint32_t count) {
    char timestamp[16];
    char uptime[24];
    timer_format_timestamp(timestamp, 0);
    timer_format_uptime(uptime, timer_get_seconds());
    
    uart_puts("\n");
    uart_puts("┌──────────────────────────────────────────┐\n");
    uart_printf("│ HEARTBEAT #%u\n", count);
    uart_puts("├──────────────────────────────────────────┤\n");
    uart_printf("│ Time     : %s\n", timestamp);
    uart_printf("│ Uptime   : %s\n", uptime);
    
    shared_status_t *status = shared_mem_get_status();
    if (status) {
        uart_printf("│ HB Count : %u\n", status->heartbeat_counter);
        uart_puts("│ Magic    : ");
        uart_put_hex32(status->magic);
        uart_puts("\n");
    }
    
    uart_puts("└──────────────────────────────────────────┘\n");
}

/*============================================================================
 * Hauptprogramm
 *============================================================================*/

void main(void) {
    uint32_t heartbeat_count = 0;
    uint64_t last_heartbeat = 0;
    uint32_t core_id;
    
    /* UART initialisieren */
    uart_init();
    
    /* Banner */
    print_banner();
    
    uart_puts("Initializing Core 3...\n\n");
    
    /* Core ID prüfen */
    core_id = get_core_id();
    uart_printf("Core ID: %u\n", core_id);
    
    if (core_id != 3) {
        uart_puts("WARNING: Not running on Core 3!\n");
    }
    
    /* Boot Info */
    uart_puts("\n");
    uart_puts("╔════════════════════════════════════════╗\n");
    uart_puts("║           BOOT INFORMATION             ║\n");
    uart_puts("╠════════════════════════════════════════╣\n");
    uart_puts("║ Load Address  : ");
    uart_put_hex32(AMP_CODE_BASE);
    uart_puts("\n");
    uart_printf("║ FW Version    : %u.%u.%u\n", 
                (FIRMWARE_VERSION >> 16) & 0xFF,
                (FIRMWARE_VERSION >> 8) & 0xFF,
                FIRMWARE_VERSION & 0xFF);
    uart_puts("║ Build Date    : " __DATE__ " " __TIME__ "\n");
    uart_puts("╚════════════════════════════════════════╝\n");
    
    /* Shared Memory initialisieren */
    uart_puts("\nInitializing shared memory...\n");
    shared_status_t *status = shared_mem_init();
    
    if (status && status->magic == FIRMWARE_MAGIC) {
        uart_puts("OK: Shared memory initialized at ");
        uart_put_hex32(SHARED_STATUS_ADDR);
        uart_puts("\n");
        uart_puts("Magic: ");
        uart_put_hex32(status->magic);
        uart_puts(" (valid)\n");
        uart_printf("Boot count: %u\n", status->boot_count);
    } else {
        uart_puts("ERROR: Failed to initialize shared memory!\n");
    }
    
    /* Memory Test überspringen für jetzt */
    uart_puts("\nSkipping memory test for now.\n");
    
    /* Status setzen */
    shared_mem_set_state(CORE3_STATE_RUNNING);
    shared_mem_set_debug("Core 3 running OK");
    
    uart_puts("\n");
    uart_puts("════════════════════════════════════════════════════════════════\n");
    uart_puts("  STARTUP COMPLETE - Entering main loop\n");
    uart_printf("  Heartbeat interval: %u ms\n", HEARTBEAT_INTERVAL_MS);
    uart_puts("  Linux can read status from: 0x20A00000\n");
    uart_puts("════════════════════════════════════════════════════════════════\n");
    
    /* Hauptschleife */
    while (1) {
        uint64_t now = timer_get_ticks();
        
        /* Heartbeat */
        if ((now - last_heartbeat) >= (HEARTBEAT_INTERVAL_MS * 1000ULL)) {
            last_heartbeat = now;
            heartbeat_count++;
            
            /* Shared Memory aktualisieren */
            shared_mem_heartbeat();
            
            /* Ausgabe */
            print_heartbeat(heartbeat_count);
        }
        
        /* Kurze Pause - KEIN wfe, das verursacht Crash! */
        for (volatile int i = 0; i < 10000; i++) {
            asm volatile("nop");
        }
    }
}
