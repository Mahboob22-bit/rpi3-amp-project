/**
 * @file read_shared_mem.c
 * @brief Linux-Tool zum Lesen des Core 3 Shared Memory Status
 * 
 * Dieses Tool läuft auf Linux (Cores 0-2) und liest den Status
 * des Core 3 bare-metal Programms aus dem Shared Memory.
 * 
 * Kompilieren (auf dem RPi3):
 *   gcc -o read_shared_mem read_shared_mem.c
 * 
 * Ausführen:
 *   sudo ./read_shared_mem
 *   sudo ./read_shared_mem -w    # Watch mode (kontinuierlich)
 * 
 * @author RPi3 AMP Project
 * @date 2025-11-26
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

/*============================================================================
 * Shared Memory Definitionen (muss mit Core 3 übereinstimmen!)
 *============================================================================*/

#define SHARED_MEM_BASE     0x20A00000
#define SHARED_STATUS_ADDR  SHARED_MEM_BASE
#define PAGE_SIZE           4096

#define FIRMWARE_MAGIC      0x52503341  /* "RP3A" */

/* Core 3 Zustände */
#define CORE3_STATE_BOOT        0
#define CORE3_STATE_INIT        1
#define CORE3_STATE_RUNNING     2
#define CORE3_STATE_MEMTEST     3
#define CORE3_STATE_ERROR       4
#define CORE3_STATE_HALTED      5

/* Shared Status Struktur - MUSS mit Core 3 übereinstimmen! */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t core3_state;
    uint32_t boot_count;
    uint64_t boot_time;
    uint64_t uptime_ticks;
    uint32_t heartbeat_counter;
    uint32_t heartbeat_interval_ms;
    uint32_t memtest_status;
    uint32_t memtest_errors;
    uint32_t memtest_bytes;
    uint32_t messages_sent;
    uint32_t messages_received;
    uint32_t reserved[8];
    char debug_message[128];
} shared_status_t;

/*============================================================================
 * Hilfsfunktionen
 *============================================================================*/

const char* state_to_string(uint32_t state) {
    switch (state) {
        case CORE3_STATE_BOOT:    return "BOOT";
        case CORE3_STATE_INIT:    return "INIT";
        case CORE3_STATE_RUNNING: return "RUNNING";
        case CORE3_STATE_MEMTEST: return "MEMTEST";
        case CORE3_STATE_ERROR:   return "ERROR";
        case CORE3_STATE_HALTED:  return "HALTED";
        default:                  return "UNKNOWN";
    }
}

void format_uptime(char *buf, size_t len, uint64_t ticks) {
    /* System Timer ist 1 MHz = 1 µs pro Tick */
    uint64_t total_sec = ticks / 1000000ULL;
    uint32_t sec = total_sec % 60;
    uint32_t min = (total_sec / 60) % 60;
    uint32_t hour = (total_sec / 3600) % 24;
    uint32_t days = total_sec / 86400;
    
    if (days > 0) {
        snprintf(buf, len, "%ud %02u:%02u:%02u", days, hour, min, sec);
    } else {
        snprintf(buf, len, "%02u:%02u:%02u", hour, min, sec);
    }
}

void print_status(volatile shared_status_t *status) {
    char uptime_str[32];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    format_uptime(uptime_str, sizeof(uptime_str), status->uptime_ticks);
    
    printf("\033[2J\033[H");  /* Clear screen */
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║           RPi3 AMP - Core 3 Status Monitor                   ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Linux Time    : %s                        ║\n", time_str);
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    
    /* Magic prüfen */
    if (status->magic != FIRMWARE_MAGIC) {
        printf("║ ⚠️  WARNING: Invalid magic (0x%08X) - Core 3 not running?   ║\n", 
               status->magic);
        printf("║ Expected magic: 0x%08X                                     ║\n", 
               FIRMWARE_MAGIC);
        printf("╚══════════════════════════════════════════════════════════════╝\n");
        return;
    }
    
    printf("║ Magic         : 0x%08X ✓ (valid)                          ║\n", status->magic);
    printf("║ FW Version    : %u.%u.%u                                          ║\n",
           (status->version >> 16) & 0xFF,
           (status->version >> 8) & 0xFF,
           status->version & 0xFF);
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ State         : %-8s", state_to_string(status->core3_state));
    if (status->core3_state == CORE3_STATE_RUNNING) {
        printf(" ✅");
    } else if (status->core3_state == CORE3_STATE_ERROR) {
        printf(" ❌");
    }
    printf("                                      ║\n");
    printf("║ Boot Count    : %-10u                                    ║\n", status->boot_count);
    printf("║ Uptime        : %-16s                              ║\n", uptime_str);
    printf("║ Heartbeat     : %-10u                                    ║\n", status->heartbeat_counter);
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Memory Test   : ");
    switch (status->memtest_status) {
        case 0: printf("Not run                                        ║\n"); break;
        case 1: printf("PASS (%u bytes)                           ║\n", status->memtest_bytes); break;
        case 2: printf("FAIL (%u errors)                              ║\n", status->memtest_errors); break;
    }
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ IPC Stats     : TX=%u, RX=%u                               ║\n", 
           status->messages_sent, status->messages_received);
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Debug Msg     : %-44s ║\n", status->debug_message);
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Press Ctrl+C to exit.\n");
}

/*============================================================================
 * Hauptprogramm
 *============================================================================*/

int main(int argc, char *argv[]) {
    int watch_mode = 0;
    int fd;
    void *map_base;
    volatile shared_status_t *status;
    
    /* Argumente prüfen */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--watch") == 0) {
            watch_mode = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [-w|--watch]\n", argv[0]);
            printf("\n");
            printf("Reads Core 3 shared memory status from 0x%08X\n", SHARED_STATUS_ADDR);
            printf("\n");
            printf("Options:\n");
            printf("  -w, --watch    Continuous monitoring mode\n");
            printf("  -h, --help     Show this help\n");
            printf("\n");
            printf("Requires root privileges (uses /dev/mem)\n");
            return 0;
        }
    }
    
    /* /dev/mem öffnen */
    fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd < 0) {
        perror("Failed to open /dev/mem");
        printf("Note: This tool requires root privileges.\n");
        printf("Try: sudo %s\n", argv[0]);
        return 1;
    }
    
    /* Shared Memory mappen */
    map_base = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, 
                    SHARED_STATUS_ADDR & ~(PAGE_SIZE - 1));
    if (map_base == MAP_FAILED) {
        perror("Failed to mmap");
        close(fd);
        return 1;
    }
    
    /* Pointer zur Status-Struktur */
    status = (volatile shared_status_t *)((char *)map_base + 
              (SHARED_STATUS_ADDR & (PAGE_SIZE - 1)));
    
    printf("RPi3 AMP - Core 3 Shared Memory Reader\n");
    printf("Mapped address: 0x%08X\n\n", SHARED_STATUS_ADDR);
    
    if (watch_mode) {
        printf("Watch mode enabled. Press Ctrl+C to stop.\n\n");
        while (1) {
            print_status(status);
            usleep(500000);  /* 500ms Update-Intervall */
        }
    } else {
        /* Einmalige Ausgabe */
        printf("╔══════════════════════════════════════════════════════════════╗\n");
        printf("║           RPi3 AMP - Core 3 Status                           ║\n");
        printf("╠══════════════════════════════════════════════════════════════╣\n");
        
        if (status->magic != FIRMWARE_MAGIC) {
            printf("║ ⚠️  Invalid magic: 0x%08X (expected 0x%08X)              ║\n", 
                   status->magic, FIRMWARE_MAGIC);
            printf("║ Core 3 firmware may not be running.                          ║\n");
        } else {
            char uptime_str[32];
            format_uptime(uptime_str, sizeof(uptime_str), status->uptime_ticks);
            
            printf("║ Magic         : 0x%08X ✓                                  ║\n", status->magic);
            printf("║ Version       : %u.%u.%u                                          ║\n",
                   (status->version >> 16) & 0xFF,
                   (status->version >> 8) & 0xFF,
                   status->version & 0xFF);
            printf("║ State         : %-8s                                      ║\n", 
                   state_to_string(status->core3_state));
            printf("║ Boot Count    : %-10u                                    ║\n", status->boot_count);
            printf("║ Uptime        : %-16s                              ║\n", uptime_str);
            printf("║ Heartbeat     : %-10u                                    ║\n", status->heartbeat_counter);
            printf("║ Memtest       : %s                                         ║\n",
                   status->memtest_status == 1 ? "PASS" : 
                   status->memtest_status == 2 ? "FAIL" : "N/A ");
            printf("║ Debug         : %-44s ║\n", status->debug_message);
        }
        printf("╚══════════════════════════════════════════════════════════════╝\n");
    }
    
    /* Aufräumen */
    munmap(map_base, PAGE_SIZE);
    close(fd);
    
    return 0;
}

