/**
 * @file timer.h
 * @brief System Timer Treiber für RPi3
 * 
 * Der BCM2837 System Timer läuft mit 1 MHz (1 µs pro Tick).
 * Er ist ein 64-bit freier Zähler, der nie überläuft (in der Praxis).
 * 
 * Bei 1 MHz: 584.942 Jahre bis zum Überlauf!
 */

#ifndef TIMER_H
#define TIMER_H

#include "common.h"

/*============================================================================
 * Funktionen
 *============================================================================*/

/**
 * @brief Gibt den aktuellen Timer-Wert zurück (64-bit)
 * @return Timer-Wert in Mikrosekunden seit Boot
 */
uint64_t timer_get_ticks(void);

/**
 * @brief Gibt die Zeit seit Boot in Sekunden zurück
 * @return Sekunden seit Boot
 */
uint32_t timer_get_seconds(void);

/**
 * @brief Gibt die Zeit seit Boot in Millisekunden zurück
 * @return Millisekunden seit Boot
 */
uint32_t timer_get_millis(void);

/**
 * @brief Wartet eine bestimmte Anzahl Mikrosekunden
 * @param us Mikrosekunden zu warten
 */
void timer_delay_us(uint32_t us);

/**
 * @brief Wartet eine bestimmte Anzahl Millisekunden
 * @param ms Millisekunden zu warten
 */
void timer_delay_ms(uint32_t ms);

/**
 * @brief Wartet eine bestimmte Anzahl Sekunden
 * @param sec Sekunden zu warten
 */
void timer_delay_sec(uint32_t sec);

/**
 * @brief Formatiert einen Zeitstempel in HH:MM:SS.mmm Format
 * @param buffer Output-Buffer (mindestens 13 Bytes)
 * @param ticks Timer-Ticks (oder 0 für aktuellen Wert)
 */
void timer_format_timestamp(char *buffer, uint64_t ticks);

/**
 * @brief Formatiert Zeit in lesbares Format (z.B. "5m 23s")
 * @param buffer Output-Buffer (mindestens 20 Bytes)
 * @param seconds Sekunden
 */
void timer_format_uptime(char *buffer, uint32_t seconds);

#endif /* TIMER_H */

