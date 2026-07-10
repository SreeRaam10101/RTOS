#include <stdint.h>
#include "kernel.h"
#include "uart.h"

static void task0_entry(void) {
    for (;;) {
        uart_puts("task0\n");
        task_yield();
    }
}

static void task1_entry(void) {
    for (;;) {
        uart_puts("task1\n");
        task_yield();
    }
}

int main(void) {
    uart_init();
    kernel_init();

    task_create(task0_entry);
    task_create(task1_entry);

    kernel_start();

    for (;;) { }  /* unreachable */
}
