#include <stdint.h>
#include "uart.h"

static void delay(volatile uint32_t count) {
    while (count--) { }
}

int main(void) {
    uart_init();
    uart_puts("M0: boot OK\n");

    for (;;) {
        uart_puts("M0: task0 alive\n");
        delay(2000000);
    }
}
