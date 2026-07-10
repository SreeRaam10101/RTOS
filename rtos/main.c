#include <stdint.h>
#include "kernel.h"
#include "uart.h"

static void task_a_entry(void) {
    for (;;) {
        uart_puts("task_a\n");
    }
}

static void task_b_entry(void) {
    for (;;) {
        uart_puts("task_b\n");
    }
}

static void task_low_entry(void) {
    for (;;) {
        uart_puts("task_low\n");
    }
}

int main(void) {
    uart_init();
    kernel_init();
    scheduler_init();

    task_create(task_a_entry, 0);
    task_create(task_b_entry, 0);
    task_create(task_low_entry, 1);

    systick_init();
    scheduler_start();

    for (;;) { }  /* unreachable */
}
