/* main.c */
/* Wir entfernen <stdint.h>, da wir -nostdinc nutzen */
/* Auf ARM64 ist ein 'int' 32-bit breit, also definieren wir es manuell: */
typedef unsigned int uint32_t;

/* Adresse im reservierten Bare-Metal Bereich */
#define SHARED_RAM_ADDR  ((volatile uint32_t*)0x20001000)

void main(void) {
    /* 1. Initialisierungssignatur schreiben: "LIFE" (0x4C494645) */
    *SHARED_RAM_ADDR = 0x4C494645;

    /* Counter Variable */
    uint32_t counter = 0;

    while(1) {
        /* 2. Zähler an die nächste Adresse schreiben (0x20001004) */
        *(SHARED_RAM_ADDR + 1) = counter++;

        /* 3. Delay Loop */
        for(volatile int i = 0; i < 2000000; i++) {
            asm volatile("nop");
        }
    }
}
