/**
 * @file timer.c
 * @brief System Timer Implementierung für RPi3
 * 
 * BCM2837 System Timer @ 1 MHz
 */

#include "timer.h"

/*============================================================================
 * System Timer Register
 *============================================================================*/

#define SYSTIMER_CS     REG32(SYSTIMER_BASE + 0x00)  /* Control/Status */
#define SYSTIMER_CLO    REG32(SYSTIMER_BASE + 0x04)  /* Counter Low 32 bits */
#define SYSTIMER_CHI    REG32(SYSTIMER_BASE + 0x08)  /* Counter High 32 bits */
#define SYSTIMER_C0     REG32(SYSTIMER_BASE + 0x0C)  /* Compare 0 */
#define SYSTIMER_C1     REG32(SYSTIMER_BASE + 0x10)  /* Compare 1 */
#define SYSTIMER_C2     REG32(SYSTIMER_BASE + 0x14)  /* Compare 2 */
#define SYSTIMER_C3     REG32(SYSTIMER_BASE + 0x18)  /* Compare 3 */

/*============================================================================
 * Implementierung
 *============================================================================*/

uint64_t timer_get_ticks(void) {
    uint32_t hi, lo, hi_check;
    
    /* 
     * Lese High und Low Teil atomar.
     * Da der Zähler während des Lesens weiterlaufen kann,
     * prüfen wir ob High sich geändert hat.
     */
    do {
        hi = SYSTIMER_CHI;
        lo = SYSTIMER_CLO;
        hi_check = SYSTIMER_CHI;
    } while (hi != hi_check);
    
    return ((uint64_t)hi << 32) | lo;
}

uint32_t timer_get_seconds(void) {
    return (uint32_t)(timer_get_ticks() / 1000000ULL);
}

uint32_t timer_get_millis(void) {
    return (uint32_t)(timer_get_ticks() / 1000ULL);
}

void timer_delay_us(uint32_t us) {
    uint64_t start = timer_get_ticks();
    while ((timer_get_ticks() - start) < us) {
        /* Busy wait */
    }
}

void timer_delay_ms(uint32_t ms) {
    timer_delay_us(ms * 1000);
}

void timer_delay_sec(uint32_t sec) {
    timer_delay_us(sec * 1000000);
}

void timer_format_timestamp(char *buffer, uint64_t ticks) {
    if (ticks == 0) {
        ticks = timer_get_ticks();
    }
    
    uint32_t total_ms = (uint32_t)(ticks / 1000);
    uint32_t ms = total_ms % 1000;
    uint32_t total_sec = total_ms / 1000;
    uint32_t sec = total_sec % 60;
    uint32_t total_min = total_sec / 60;
    uint32_t min = total_min % 60;
    uint32_t hour = total_min / 60;
    
    /* Format: HH:MM:SS.mmm */
    buffer[0] = '0' + (hour / 10) % 10;
    buffer[1] = '0' + hour % 10;
    buffer[2] = ':';
    buffer[3] = '0' + min / 10;
    buffer[4] = '0' + min % 10;
    buffer[5] = ':';
    buffer[6] = '0' + sec / 10;
    buffer[7] = '0' + sec % 10;
    buffer[8] = '.';
    buffer[9] = '0' + ms / 100;
    buffer[10] = '0' + (ms / 10) % 10;
    buffer[11] = '0' + ms % 10;
    buffer[12] = '\0';
}

void timer_format_uptime(char *buffer, uint32_t seconds) {
    uint32_t sec = seconds % 60;
    uint32_t min = (seconds / 60) % 60;
    uint32_t hour = (seconds / 3600) % 24;
    uint32_t days = seconds / 86400;
    
    int i = 0;
    
    if (days > 0) {
        /* Tage */
        if (days >= 10) buffer[i++] = '0' + days / 10;
        buffer[i++] = '0' + days % 10;
        buffer[i++] = 'd';
        buffer[i++] = ' ';
    }
    
    if (hour > 0 || days > 0) {
        /* Stunden */
        if (hour >= 10) buffer[i++] = '0' + hour / 10;
        buffer[i++] = '0' + hour % 10;
        buffer[i++] = 'h';
        buffer[i++] = ' ';
    }
    
    if (min > 0 || hour > 0 || days > 0) {
        /* Minuten */
        if (min >= 10) buffer[i++] = '0' + min / 10;
        buffer[i++] = '0' + min % 10;
        buffer[i++] = 'm';
        buffer[i++] = ' ';
    }
    
    /* Sekunden (immer anzeigen) */
    if (sec >= 10) buffer[i++] = '0' + sec / 10;
    buffer[i++] = '0' + sec % 10;
    buffer[i++] = 's';
    buffer[i] = '\0';
}

