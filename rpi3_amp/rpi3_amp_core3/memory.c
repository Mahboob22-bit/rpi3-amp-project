/**
 * @file memory.c
 * @brief Memory Management und Tests Implementierung
 */

#include "memory.h"
#include "uart.h"
#include "timer.h"

/*============================================================================
 * Private Variablen
 *============================================================================*/

static shared_status_t *g_status = NULL;

/*============================================================================
 * String Hilfsfunktionen
 *============================================================================*/

static void str_copy(char *dest, const char *src, uint32_t max_len) {
    uint32_t i;
    for (i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/*============================================================================
 * Shared Memory Implementierung
 *============================================================================*/

shared_status_t* shared_mem_init(void) {
    g_status = (shared_status_t *)SHARED_STATUS_ADDR;
    
    /* Erst den gesamten Bereich auf 0 setzen */
    uint8_t *ptr = (uint8_t *)g_status;
    for (uint32_t i = 0; i < sizeof(shared_status_t); i++) {
        ptr[i] = 0;
    }
    
    /* Dann Struktur initialisieren */
    g_status->magic = FIRMWARE_MAGIC;
    g_status->version = FIRMWARE_VERSION;
    g_status->core3_state = CORE3_STATE_INIT;
    g_status->boot_count = 1;  /* Einfach auf 1 setzen */
    g_status->boot_time = timer_get_ticks();
    g_status->uptime_ticks = 0;
    g_status->heartbeat_counter = 0;
    g_status->heartbeat_interval_ms = 1000;  /* Default: 1 Sekunde */
    g_status->memtest_status = 0;
    g_status->memtest_errors = 0;
    g_status->memtest_bytes = 0;
    g_status->messages_sent = 0;
    g_status->messages_received = 0;
    
    str_copy(g_status->debug_message, "Core 3 initialized", sizeof(g_status->debug_message));
    
    /* Memory Barrier sicherstellen */
    DSB();
    
    return g_status;
}

void shared_mem_update_uptime(void) {
    if (g_status) {
        g_status->uptime_ticks = timer_get_ticks() - g_status->boot_time;
        DSB();
    }
}

void shared_mem_heartbeat(void) {
    if (g_status) {
        g_status->heartbeat_counter++;
        shared_mem_update_uptime();
    }
}

void shared_mem_set_state(uint32_t state) {
    if (g_status) {
        g_status->core3_state = state;
        DSB();
    }
}

void shared_mem_set_debug(const char *msg) {
    if (g_status) {
        str_copy(g_status->debug_message, msg, sizeof(g_status->debug_message));
        DSB();
    }
}

shared_status_t* shared_mem_get_status(void) {
    return g_status;
}

/*============================================================================
 * Memory Tests
 *============================================================================*/

uint32_t memory_test_pattern(uintptr_t start_addr, uint32_t size, uint32_t pattern) {
    volatile uint32_t *ptr = (volatile uint32_t *)start_addr;
    uint32_t words = size / 4;
    uint32_t errors = 0;
    
    /* Schreibe Pattern */
    for (uint32_t i = 0; i < words; i++) {
        ptr[i] = pattern;
    }
    
    DSB();
    
    /* Lese und verifiziere */
    for (uint32_t i = 0; i < words; i++) {
        if (ptr[i] != pattern) {
            errors++;
        }
    }
    
    return errors;
}

uint32_t memory_test_walking_ones(uintptr_t start_addr, uint32_t size) {
    volatile uint32_t *ptr = (volatile uint32_t *)start_addr;
    uint32_t words = size / 4;
    uint32_t errors = 0;
    
    /* Teste jede Bit-Position */
    for (int bit = 0; bit < 32; bit++) {
        uint32_t pattern = 1U << bit;
        
        /* Schreibe */
        for (uint32_t i = 0; i < words; i++) {
            ptr[i] = pattern;
        }
        
        DSB();
        
        /* Verifiziere */
        for (uint32_t i = 0; i < words; i++) {
            if (ptr[i] != pattern) {
                errors++;
            }
        }
    }
    
    return errors;
}

uint32_t memory_test_quick(uintptr_t start_addr, uint32_t size) {
    uint32_t errors = 0;
    
    errors += memory_test_pattern(start_addr, size, MEMTEST_PATTERN_ZEROS);
    errors += memory_test_pattern(start_addr, size, MEMTEST_PATTERN_ONES);
    errors += memory_test_pattern(start_addr, size, MEMTEST_PATTERN_AA);
    errors += memory_test_pattern(start_addr, size, MEMTEST_PATTERN_55);
    
    return errors;
}

uint32_t memory_test_full(uintptr_t start_addr, uint32_t size, bool verbose) {
    uint32_t total_errors = 0;
    uint32_t test_errors;
    
    if (g_status) {
        shared_mem_set_state(CORE3_STATE_MEMTEST);
    }
    
    if (verbose) {
        uart_puts("\n");
        uart_puts("╔════════════════════════════════════════╗\n");
        uart_puts("║           MEMORY TEST                  ║\n");
        uart_puts("╠════════════════════════════════════════╣\n");
        uart_printf("║ Start Addr  : %x\n", start_addr);
        uart_printf("║ Size        : %u bytes\n", size);
        uart_puts("╠════════════════════════════════════════╣\n");
    }
    
    /* Test 1: All Zeros */
    if (verbose) uart_puts("║ Test 1: All Zeros...    ");
    test_errors = memory_test_pattern(start_addr, size, MEMTEST_PATTERN_ZEROS);
    total_errors += test_errors;
    if (verbose) {
        if (test_errors == 0) uart_puts("PASS\n");
        else uart_printf("FAIL (%u)\n", test_errors);
    }
    
    /* Test 2: All Ones */
    if (verbose) uart_puts("║ Test 2: All Ones...     ");
    test_errors = memory_test_pattern(start_addr, size, MEMTEST_PATTERN_ONES);
    total_errors += test_errors;
    if (verbose) {
        if (test_errors == 0) uart_puts("PASS\n");
        else uart_printf("FAIL (%u)\n", test_errors);
    }
    
    /* Test 3: 0xAA */
    if (verbose) uart_puts("║ Test 3: 0xAAAAAAAA...   ");
    test_errors = memory_test_pattern(start_addr, size, MEMTEST_PATTERN_AA);
    total_errors += test_errors;
    if (verbose) {
        if (test_errors == 0) uart_puts("PASS\n");
        else uart_printf("FAIL (%u)\n", test_errors);
    }
    
    /* Test 4: 0x55 */
    if (verbose) uart_puts("║ Test 4: 0x55555555...   ");
    test_errors = memory_test_pattern(start_addr, size, MEMTEST_PATTERN_55);
    total_errors += test_errors;
    if (verbose) {
        if (test_errors == 0) uart_puts("PASS\n");
        else uart_printf("FAIL (%u)\n", test_errors);
    }
    
    /* Test 5: Walking Ones (nur für kleine Bereiche) */
    if (size <= 0x10000) {  /* Max 64 KB für walking ones */
        if (verbose) uart_puts("║ Test 5: Walking Ones... ");
        test_errors = memory_test_walking_ones(start_addr, size);
        total_errors += test_errors;
        if (verbose) {
            if (test_errors == 0) uart_puts("PASS\n");
            else uart_printf("FAIL (%u)\n", test_errors);
        }
    }
    
    /* Test 6: Address as Data */
    if (verbose) uart_puts("║ Test 6: Addr as Data... ");
    {
        volatile uint32_t *ptr = (volatile uint32_t *)start_addr;
        uint32_t words = size / 4;
        test_errors = 0;
        
        /* Schreibe Adresse als Daten */
        for (uint32_t i = 0; i < words; i++) {
            ptr[i] = start_addr + (i * 4);
        }
        
        DSB();
        
        /* Verifiziere */
        for (uint32_t i = 0; i < words; i++) {
            if (ptr[i] != start_addr + (i * 4)) {
                test_errors++;
            }
        }
        
        total_errors += test_errors;
        if (verbose) {
            if (test_errors == 0) uart_puts("PASS\n");
            else uart_printf("FAIL (%u)\n", test_errors);
        }
    }
    
    /* Ergebnis */
    if (verbose) {
        uart_puts("╠════════════════════════════════════════╣\n");
        uart_puts("║ Result      : ");
        if (total_errors == 0) {
            uart_puts("ALL TESTS PASSED ✓\n");
        } else {
            uart_printf("FAILED (%u errors)\n", total_errors);
        }
        uart_puts("╚════════════════════════════════════════╝\n");
    }
    
    /* Status aktualisieren */
    if (g_status) {
        g_status->memtest_status = (total_errors == 0) ? 1 : 2;
        g_status->memtest_errors = total_errors;
        g_status->memtest_bytes = size;
        shared_mem_set_state(CORE3_STATE_RUNNING);
    }
    
    return total_errors;
}

/*============================================================================
 * Debug-Ausgaben
 *============================================================================*/

void memory_print_map(void) {
    uart_puts("\n");
    uart_puts("╔════════════════════════════════════════╗\n");
    uart_puts("║           MEMORY MAP                   ║\n");
    uart_puts("╠════════════════════════════════════════╣\n");
    uart_puts("║ Linux RAM       : 0x00000000-0x1FFFFFFF\n");
    uart_puts("║                   (512 MB)             ║\n");
    uart_puts("╠────────────────────────────────────────╣\n");
    uart_puts("║ AMP Code/Data   : 0x20000000-0x209FFFFF\n");
    uart_puts("║                   (10 MB)              ║\n");
    uart_puts("╠────────────────────────────────────────╣\n");
    uart_puts("║ Shared Memory   : 0x20A00000-0x20BFFFFF\n");
    uart_puts("║                   (2 MB)               ║\n");
    uart_puts("║   - Status      : 0x20A00000 (4 KB)    ║\n");
    uart_puts("║   - Data        : 0x20A01000 (4 KB)    ║\n");
    uart_puts("║   - Memtest     : 0x20A02000 (64 KB)   ║\n");
    uart_puts("╠────────────────────────────────────────╣\n");
    uart_puts("║ Peripherals     : 0x3F000000-0x3FFFFFFF\n");
    uart_puts("║                   (16 MB)              ║\n");
    uart_puts("╠────────────────────────────────────────╣\n");
    uart_puts("║ ARM Local       : 0x40000000-0x40000FFF\n");
    uart_puts("║                   (4 KB)               ║\n");
    uart_puts("╚════════════════════════════════════════╝\n");
}

void memory_print_status(void) {
    if (!g_status) {
        uart_puts("Shared memory not initialized!\n");
        return;
    }
    
    char timestamp[16];
    timer_format_timestamp(timestamp, g_status->uptime_ticks);
    
    uart_puts("\n");
    uart_puts("╔════════════════════════════════════════╗\n");
    uart_puts("║         SHARED MEMORY STATUS           ║\n");
    uart_puts("╠════════════════════════════════════════╣\n");
    uart_printf("║ Magic         : %x", g_status->magic);
    if (g_status->magic == FIRMWARE_MAGIC) {
        uart_puts(" (valid)\n");
    } else {
        uart_puts(" (INVALID!)\n");
    }
    uart_printf("║ Version       : %u.%u.%u\n", 
                (g_status->version >> 16) & 0xFF,
                (g_status->version >> 8) & 0xFF,
                g_status->version & 0xFF);
    uart_printf("║ State         : %u", g_status->core3_state);
    switch (g_status->core3_state) {
        case CORE3_STATE_BOOT:    uart_puts(" (BOOT)\n"); break;
        case CORE3_STATE_INIT:    uart_puts(" (INIT)\n"); break;
        case CORE3_STATE_RUNNING: uart_puts(" (RUNNING)\n"); break;
        case CORE3_STATE_MEMTEST: uart_puts(" (MEMTEST)\n"); break;
        case CORE3_STATE_ERROR:   uart_puts(" (ERROR)\n"); break;
        case CORE3_STATE_HALTED:  uart_puts(" (HALTED)\n"); break;
        default:                  uart_puts(" (UNKNOWN)\n"); break;
    }
    uart_printf("║ Boot Count    : %u\n", g_status->boot_count);
    uart_printf("║ Uptime        : %s\n", timestamp);
    uart_printf("║ Heartbeat     : %u\n", g_status->heartbeat_counter);
    uart_puts("╠────────────────────────────────────────╣\n");
    uart_puts("║ Memtest       : ");
    switch (g_status->memtest_status) {
        case 0: uart_puts("Not run\n"); break;
        case 1: uart_printf("PASS (%u bytes)\n", g_status->memtest_bytes); break;
        case 2: uart_printf("FAIL (%u errors)\n", g_status->memtest_errors); break;
    }
    uart_puts("╠────────────────────────────────────────╣\n");
    uart_printf("║ Debug         : %s\n", g_status->debug_message);
    uart_puts("╚════════════════════════════════════════╝\n");
}

