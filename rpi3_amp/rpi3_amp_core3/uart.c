/**
 * @file uart.c
 * @brief UART0 (PL011) Implementierung für RPi3
 * 
 * Hinweis: Kein stdarg.h verfügbar in bare-metal!
 * Wir verwenden ein vereinfachtes printf ohne va_list.
 */

#include "uart.h"

/*============================================================================
 * UART0 Register
 *============================================================================*/

#define UART0_DR        REG32(UART0_BASE + 0x00)  /* Data Register */
#define UART0_FR        REG32(UART0_BASE + 0x18)  /* Flag Register */
#define UART0_IBRD      REG32(UART0_BASE + 0x24)  /* Integer Baud Rate */
#define UART0_FBRD      REG32(UART0_BASE + 0x28)  /* Fractional Baud Rate */
#define UART0_LCRH      REG32(UART0_BASE + 0x2C)  /* Line Control */
#define UART0_CR        REG32(UART0_BASE + 0x30)  /* Control Register */
#define UART0_ICR       REG32(UART0_BASE + 0x44)  /* Interrupt Clear */

/* GPIO Register für UART Pins */
#define GPFSEL1         REG32(GPIO_BASE + 0x04)
#define GPPUD           REG32(GPIO_BASE + 0x94)
#define GPPUDCLK0       REG32(GPIO_BASE + 0x98)

/* Flag Register Bits */
#define UART_FR_TXFF    (1 << 5)  /* TX FIFO Full */
#define UART_FR_RXFE    (1 << 4)  /* RX FIFO Empty */

/*============================================================================
 * Private Hilfsfunktionen
 *============================================================================*/

static void delay_cycles(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++) {
        asm volatile("nop");
    }
}

/*============================================================================
 * Öffentliche Funktionen
 *============================================================================*/

void uart_init(void) {
    uint32_t ra;

    /* 1. UART0 deaktivieren */
    UART0_CR = 0;

    /* 2. GPIO 14 und 15 als UART0 (ALT0) konfigurieren */
    ra = GPFSEL1;
    ra &= ~((7 << 12) | (7 << 15));  /* Clear GPIO 14 und 15 */
    ra |= (4 << 12) | (4 << 15);     /* ALT0 für beide */
    GPFSEL1 = ra;

    /* 3. Pull-up/down deaktivieren */
    GPPUD = 0;
    delay_cycles(150);
    GPPUDCLK0 = (1 << 14) | (1 << 15);
    delay_cycles(150);
    GPPUDCLK0 = 0;

    /* 4. Alle Interrupts clearen */
    UART0_ICR = 0x7FF;

    /* 5. Baudrate: 115200 @ 48 MHz UART Clock
     * Divider = 48000000 / (16 * 115200) = 26.041666...
     * IBRD = 26, FBRD = 0.041666 * 64 = 2.666 ≈ 3
     */
    UART0_IBRD = 26;
    UART0_FBRD = 3;

    /* 6. Line Control: 8 Bit, keine Parity, 1 Stop Bit, FIFO aktivieren */
    UART0_LCRH = (3 << 5) | (1 << 4);  /* 8N1 + FIFO enable */

    /* 7. UART aktivieren (TX + RX) */
    UART0_CR = (1 << 0) | (1 << 8) | (1 << 9);
    
    DSB();
}

void uart_putc(char c) {
    /* Warten bis TX FIFO nicht voll ist */
    while (UART0_FR & UART_FR_TXFF);
    UART0_DR = c;
}

void uart_puts(const char *str) {
    while (*str) {
        if (*str == '\n') {
            uart_putc('\r');
        }
        uart_putc(*str++);
    }
}

void uart_newline(void) {
    uart_putc('\r');
    uart_putc('\n');
}

void uart_put_hex32(uint32_t val) {
    static const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xF]);
    }
}

void uart_put_hex64(uint64_t val) {
    static const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xF]);
    }
}

void uart_put_uint(uint32_t val) {
    char buf[12];
    int i = 0;
    
    if (val == 0) {
        uart_putc('0');
        return;
    }
    
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

/*============================================================================
 * Printf Implementierung mit GCC __builtin_va_* (kein stdarg.h nötig)
 *============================================================================*/

void uart_printf(const char *fmt, ...) {
    /* 
     * Vereinfachte Implementierung ohne echtes varargs:
     * Verwendet GCC built-in für aarch64 zur Argument-Extraktion
     */
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's': {
                    const char *s = __builtin_va_arg(args, const char *);
                    uart_puts(s ? s : "(null)");
                    break;
                }
                case 'd': {
                    int32_t v = __builtin_va_arg(args, int32_t);
                    if (v < 0) {
                        uart_putc('-');
                        v = -v;
                    }
                    uart_put_uint((uint32_t)v);
                    break;
                }
                case 'u': {
                    uint32_t v = __builtin_va_arg(args, uint32_t);
                    uart_put_uint(v);
                    break;
                }
                case 'x':
                case 'X': {
                    uint32_t v = __builtin_va_arg(args, uint32_t);
                    uart_put_hex32(v);
                    break;
                }
                case '%':
                    uart_putc('%');
                    break;
                default:
                    uart_putc('%');
                    if (*fmt) uart_putc(*fmt);
                    break;
            }
            if (*fmt) fmt++;
        } else {
            if (*fmt == '\n') {
                uart_putc('\r');
            }
            uart_putc(*fmt++);
        }
    }
    
    __builtin_va_end(args);
}
