#include <stdint.h>
#include "kernel.h"
#include "uart.h"
#include "rtos.h"

#define PERIOD_A 50
#define PERIOD_B 80

#if WORKLOAD == 1
#define WCET_A 13
#define WCET_B 20
#elif WORKLOAD == 2
#define WCET_A 21
#define WCET_B 40
#elif WORKLOAD == 3
#define WCET_A 24
#define WCET_B 38
#elif WORKLOAD == 4
#define WCET_A 30
#define WCET_B 45
#else
#error "WORKLOAD must be 1, 2, 3, or 4 -- see docs/superpowers/specs/2026-07-15-m6-edf-rms-comparison-design.md"
#endif

static void busy_wait(uint32_t work_ticks) {
    /* Spins until the SCHEDULER (via SysTick_Handler) has decremented this
       task's own remaining_wcet_ticks to zero -- i.e. until this task has
       actually held the CPU for work_ticks ticks, not merely until
       work_ticks of wall-clock time has passed. A preempted task's counter
       does not move while a higher-priority task runs, so this correctly
       simulates WCET consumption under preemption (see kernel.h/systick.c
       changes above). */
    current_tcb->remaining_wcet_ticks = work_ticks;
    while (current_tcb->remaining_wcet_ticks > 0) {
        /* simulate fixed-length periodic work */
    }
}

static void task_a_entry(void) {
    for (;;) {
        task_wait_for_release();
        busy_wait(WCET_A);
        uart_puts("PeriodicA: tick=");
        uart_put_uint32(tick_count);
        uart_puts(" misses=");
        uart_put_uint32(periodic_get_miss_count(current_tcb));
        uart_puts("\n");
    }
}

static void task_b_entry(void) {
    for (;;) {
        task_wait_for_release();
        busy_wait(WCET_B);
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

    /* deadline == period (implicit deadlines) for this comparison -- unlike
       M5's constrained-deadline demo -- so edf_check()'s U<=1 bound is
       exactly necessary-and-sufficient. See rtos.c's edf_check() doc
       comment and the M6 spec. */
    periodic_task_create(task_a_entry, 0, PERIOD_A, PERIOD_A, WCET_A);
    periodic_task_create(task_b_entry, 1, PERIOD_B, PERIOD_B, WCET_B);

#ifdef SCHED_EDF
    int schedulable = edf_check();
    uart_puts("EDF check: U=");
    uart_put_uint32(edf_get_utilization_x10000());
    uart_puts(" bound=");
    uart_put_uint32(edf_get_bound_x10000());
    uart_puts(schedulable ? " => SCHEDULABLE\n" : " => NOT SCHEDULABLE\n");
#else
    int schedulable = rms_check();
    uart_puts("RMS check: U=");
    uart_put_uint32(rms_get_utilization_x10000());
    uart_puts(" bound=");
    uart_put_uint32(rms_get_bound_x10000());
    uart_puts(schedulable ? " => SCHEDULABLE\n" : " => NOT SCHEDULABLE\n");
#endif

    systick_init();
    scheduler_start();

    for (;;) { }  /* unreachable */
}
