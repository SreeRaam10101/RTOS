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
    scheduler_init();

    task_create(task0_entry, 0);
    task_create(task1_entry, 0);

    systick_init();
    scheduler_start();

    for (;;) { }  /* unreachable */
}
