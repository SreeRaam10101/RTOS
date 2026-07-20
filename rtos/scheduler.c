#include <stdint.h>
#include "kernel.h"

#define IDLE_PRIORITY (MAX_PRIORITY_LEVELS - 1)

static void idle_entry(void) {
    for (;;) {
        __asm__ volatile ("wfi");
    }
}

void scheduler_init(void) {
    tcb_t *idle = task_create(idle_entry, IDLE_PRIORITY);
    idle->abs_deadline = 0xFFFFFFFF;  /* EDF: always picked last; unused field under RMS builds */
}

#ifdef SCHED_EDF
void scheduler_next(void) {
    if (current_tcb != 0 && current_tcb->state == TASK_RUNNING) {
        ready_enqueue(current_tcb);
    }
    current_tcb = ready_dequeue_min_deadline();
    current_tcb->state = TASK_RUNNING;
}
#else
static int highest_ready_priority(void) {
    for (int p = 0; p < MAX_PRIORITY_LEVELS; p++) {
        if (ready_queue[p] != 0) {
            return p;
        }
    }
    return -1;  /* unreachable: the idle task always occupies the last slot */
}

void scheduler_next(void) {
    if (current_tcb != 0 && current_tcb->state == TASK_RUNNING) {
        /* still runnable, just preempted (by SysTick or an explicit yield):
           go back to the tail of its own priority level. */
        ready_enqueue(current_tcb);
    }
    /* if current_tcb->state == TASK_BLOCKED, the blocking primitive that
       put it there already removed it from ready_queue and changed its
       state before pending PendSV -- nothing to do here for that case.
       (current_tcb == 0 only on the very first call, from scheduler_start
       before any task has ever run -- also nothing to do.) */

    int p = highest_ready_priority();
    current_tcb = ready_queue[p];
    ready_remove(current_tcb);
    current_tcb->state = TASK_RUNNING;
}
#endif
