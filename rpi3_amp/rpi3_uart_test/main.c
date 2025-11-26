// UART0 Test (GPIO 14/15 - Physical Pins 8/10)
// Nutzt dieselben Pins wie Linux Console - einfacher zu testen!

#define PERIPHERAL_BASE   0x3F000000
#define GPIO_BASE         (PERIPHERAL_BASE + 0x200000)
#define UART0_BASE        (PERIPHERAL_BASE + 0x201000)  // UART0!

// GPIO Register
#define GPFSEL1           ((volatile unsigned int*)(GPIO_BASE + 0x04))
#define GPPUD             ((volatile unsigned int*)(GPIO_BASE + 0x94))
#define GPPUDCLK0         ((volatile unsigned int*)(GPIO_BASE + 0x98))

// UART0 Register
#define UART0_DR          ((volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR          ((volatile unsigned int*)(UART0_BASE + 0x18))
#define UART0_IBRD        ((volatile unsigned int*)(UART0_BASE + 0x24))
#define UART0_FBRD        ((volatile unsigned int*)(UART0_BASE + 0x28))
#define UART0_LCRH        ((volatile unsigned int*)(UART0_BASE + 0x2C))
#define UART0_CR          ((volatile unsigned int*)(UART0_BASE + 0x30))
#define UART0_ICR         ((volatile unsigned int*)(UART0_BASE + 0x44))

#define UART_FR_TXFF      (1 << 5)

void delay(unsigned int count) {
    for (volatile unsigned int i = 0; i < count; i++) {
        asm volatile("nop");
    }
}

void uart0_init(void) {
    unsigned int ra;

    // 1. UART0 disable
    *UART0_CR = 0;

    // 2. GPIO 14 und 15 als UART0 (ALT0)
    ra = *GPFSEL1;
    ra &= ~((7 << 12) | (7 << 15));  // Clear GPIO 14 und 15
    ra |= (4 << 12) | (4 << 15);     // ALT0 für beide
    *GPFSEL1 = ra;

    // 3. Pull-up/down disable
    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    delay(150);
    *GPPUDCLK0 = 0;

    // 4. Clear interrupts
    *UART0_ICR = 0x7FF;

    // 5. Baudrate: 115200 @ 48MHz
    *UART0_IBRD = 26;
    *UART0_FBRD = 3;

    // 6. Line Control: 8N1
    *UART0_LCRH = (3 << 5);

    // 7. Enable UART
    *UART0_CR = (1 << 0) | (1 << 8) | (1 << 9);
}

void uart0_putc(char c) {
    while (*UART0_FR & UART_FR_TXFF);
    *UART0_DR = c;
}

void uart0_puts(const char *str) {
    while (*str) {
        if (*str == '\n') uart0_putc('\r');
        uart0_putc(*str++);
    }
}

void main(void) {
    uart0_init();

    uart0_puts("\n\n*** UART0 Bare-Metal Test ***\n");
    uart0_puts("GPIO 14/15 (Pins 8/10)\n");
    uart0_puts("If you see this, UART works!\n\n");

    unsigned int counter = 0;
    while(1) {
        uart0_puts("Message #");
        // Simple number output
        char buf[16];
        int i = 0;
        unsigned int n = counter;
        if (n == 0) buf[i++] = '0';
        else {
            char tmp[16];
            int j = 0;
            while (n > 0) {
                tmp[j++] = '0' + (n % 10);
                n /= 10;
            }
            while (j > 0) buf[i++] = tmp[--j];
        }
        buf[i] = '\0';
        uart0_puts(buf);
        uart0_puts("\n");

        counter++;
        delay(1000000);
    }
}
