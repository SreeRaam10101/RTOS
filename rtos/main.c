#include <stdint.h>
#include "kernel.h"
#include "uart.h"
#include "sync.h"

static mutex_t shared_mutex;
static volatile uint32_t medium_run_count = 0;

#define WORK_TICKS 200

static void high_entry(void) {
    delay(5);
    for (;;) {
        mutex_lock(&shared_mutex);
        uart_puts("High: acquired\n");
        mutex_unlock(&shared_mutex);
        delay(300);
    }
}

static void medium_entry(void) {
    delay(2);
    for (;;) {
        medium_run_count++;
        if (medium_run_count % 5000 == 0) {
            uart_puts("Medium: running\n");
        }
    }
}

static void low_entry(void) {
    for (;;) {
        mutex_lock(&shared_mutex);
        uart_puts("Low: acquired\n");
        uint32_t start = tick_count;
        while (tick_count - start < WORK_TICKS) {
            /* simulate holding the mutex while doing bounded work --
               a real busy spin, not delay(), so this task stays
               TASK_RUNNING/TASK_READY and is only removable by priority
               preemption, never by voluntarily yielding. */
        }
        uart_puts("Low: releasing\n");
        mutex_unlock(&shared_mutex);
        uart_puts("Low: priority_after_unlock=");
        uart_put_uint32(current_tcb->priority);
        uart_puts("\n");
        delay(50);
    }
}

int main(void) {
    uart_init();
    kernel_init();
    scheduler_init();

    mutex_init(&shared_mutex);

    task_create(high_entry, 0);
    task_create(medium_entry, 1);
    task_create(low_entry, 2);

    systick_init();
    scheduler_start();

    for (;;) { }  /* unreachable */
}
