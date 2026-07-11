#include <stdint.h>
#include "kernel.h"
#include "uart.h"
#include "rtos.h"

static void periodic_a_entry(void) {
    for (;;) {
        task_wait_for_release();
        uart_puts("PeriodicA: tick=");
        uart_put_uint32(tick_count);
        uart_puts(" misses=");
        uart_put_uint32(periodic_get_miss_count(current_tcb));
        uart_puts("\n");
    }
}

static void periodic_b_entry(void) {
    uint32_t iteration = 0;
    for (;;) {
        task_wait_for_release();
        iteration++;
        uint32_t work_ticks = (iteration % 3 == 0) ? 70 : 5;
        uint32_t start = tick_count;
        while (tick_count - start < work_ticks) {
            /* simulate variable-length periodic work; every 3rd iteration
               deliberately exceeds the 60-tick deadline to demonstrate the
               deadline-miss counter as a real runtime safety net, distinct
               from rms_check()'s static/offline guarantee. */
        }
        uart_puts("PeriodicB: tick=");
        uart_put_uint32(tick_count);
        uart_puts(" misses=");
        uart_put_uint32(periodic_get_miss_count(current_tcb));
        uart_puts("\n");
    }
}

int main(void) {
    uart_init();
    kernel_init();
    scheduler_init();

    periodic_task_create(periodic_a_entry, 0, 50, 40, 5);
    periodic_task_create(periodic_b_entry, 1, 80, 60, 10);

    int schedulable = rms_check();
    uart_puts("RMS check: U=");
    uart_put_uint32(rms_get_utilization_x10000());
    uart_puts(" bound=");
    uart_put_uint32(rms_get_bound_x10000());
    uart_puts(schedulable ? " => SCHEDULABLE\n" : " => NOT SCHEDULABLE\n");

    systick_init();
    scheduler_start();

    for (;;) { }  /* unreachable */
}
