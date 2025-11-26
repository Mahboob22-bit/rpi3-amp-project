/**
 * @file uart.h
 * @brief UART0 (PL011) Treiber für RPi3 Core 3 Bare-Metal
 * 
 * Verwendet UART0 auf GPIO 14/15 für Debug-Ausgaben.
 * Hinweis: Wenn Linux läuft, muss die Console auf UART0 deaktiviert sein!
 */

#ifndef UART_H
#define UART_H

#include "common.h"

/*============================================================================
 * Funktionen
 *============================================================================*/

/**
 * @brief Initialisiert UART0 mit 115200 Baud, 8N1
 */
void uart_init(void);

/**
 * @brief Sendet ein einzelnes Zeichen
 * @param c Das zu sendende Zeichen
 */
void uart_putc(char c);

/**
 * @brief Sendet einen String (null-terminiert)
 * @param str Der zu sendende String
 */
void uart_puts(const char *str);

/**
 * @brief Sendet eine 32-bit Zahl als Hex
 * @param val Der Wert
 */
void uart_put_hex32(uint32_t val);

/**
 * @brief Sendet eine 64-bit Zahl als Hex
 * @param val Der Wert
 */
void uart_put_hex64(uint64_t val);

/**
 * @brief Sendet eine Dezimalzahl (unsigned)
 * @param val Der Wert
 */
void uart_put_uint(uint32_t val);

/**
 * @brief Sendet einen formatierten String (vereinfachtes printf)
 * 
 * Unterstützte Formate:
 *   %s - String
 *   %d - Dezimal (signed)
 *   %u - Dezimal (unsigned)
 *   %x - Hex (lowercase)
 *   %X - Hex (uppercase)
 *   %% - Prozentzeichen
 * 
 * @param fmt Format-String
 * @param ... Argumente
 */
void uart_printf(const char *fmt, ...);

/**
 * @brief Sendet eine neue Zeile
 */
void uart_newline(void);

#endif /* UART_H */

