#include <stdint.h>
#include "kernel.h"

#define IDLE_PRIORITY (MAX_PRIORITY_LEVELS - 1)

static void idle_entry(void) {
    for (;;) {
        __asm__ volatile ("wfi");
    }
}

void scheduler_init(void) {
    /* Lowest priority, created first so the ready-queue is never empty.
       Nothing in M2 ever removes a task from its ready-queue slot (no
       blocking primitives yet), so this invariant holds unconditionally
       for this milestone. */
    task_create(idle_entry, IDLE_PRIORITY);
}

static int highest_ready_priority(void) {
    for (int p = 0; p < MAX_PRIORITY_LEVELS; p++) {
        if (ready_queue[p] != 0) {
            return p;
        }
    }
    return -1;  /* unreachable in M2: the idle task always occupies the last slot */
}

void scheduler_next(void) {
    int p = highest_ready_priority();
    tcb_t *head = ready_queue[p];

    if (head->next != 0) {
        /* More than one task at this priority: rotate the list so the
           current head moves to the tail before we reselect. This is what
           makes every SysTick-driven switch actually share the CPU between
           equal-priority tasks, instead of the same task always winning. */
        ready_queue[p] = head->next;
        tcb_t *tail = ready_queue[p];
        while (tail->next != 0) {
            tail = tail->next;
        }
        tail->next = head;
        head->next = 0;
    }

    current_tcb = ready_queue[p];
}
