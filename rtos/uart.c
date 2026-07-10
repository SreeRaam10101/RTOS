#include <stdint.h>
#include "uart.h"

#define UART0_BASE       0x40004000UL
#define UART0_DATA       (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_STATE      (*(volatile uint32_t *)(UART0_BASE + 0x04))
#define UART0_CTRL       (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART0_BAUDDIV    (*(volatile uint32_t *)(UART0_BASE + 0x10))

#define UART_STATE_TXFULL  (1u << 0)
#define UART_CTRL_TX_EN     (1u << 0)

#define SYSCLK_HZ    25000000UL
#define BAUD_RATE    115200UL

void uart_init(void) {
    UART0_BAUDDIV = SYSCLK_HZ / BAUD_RATE;
    UART0_CTRL = UART_CTRL_TX_EN;
}

void uart_putc(char c) {
    while (UART0_STATE & UART_STATE_TXFULL) { }
    UART0_DATA = (uint32_t)(uint8_t)c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s);
        s++;
    }
}

void uart_put_uint32(uint32_t v) {
    char buf[11];   /* "4294967295" (10 digits) + NUL */
    int i = 10;
    buf[i] = '\0';
    if (v == 0) {
        buf[--i] = '0';
    } else {
        while (v > 0) {
            buf[--i] = (char)('0' + (v % 10));
            v /= 10;
        }
    }
    uart_puts(&buf[i]);
}
