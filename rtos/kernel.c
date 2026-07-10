#include <stdint.h>
#include "kernel.h"

#define MAX_TASKS 4
#define TASK_STACK_WORDS 256

#define SCB_ICSR   (*(volatile uint32_t *)0xE000ED04)
#define SCB_SHPR3  (*(volatile uint32_t *)0xE000ED20)
#define ICSR_PENDSVSET (1UL << 28)

static tcb_t task_table[MAX_TASKS];
static uint32_t task_stacks[MAX_TASKS][TASK_STACK_WORDS];
static int num_tasks = 0;

tcb_t *current_tcb = 0;

void kernel_init(void) {
    /* PendSV must be the lowest-priority exception: a context switch must
       never preempt another exception's critical section. */
    SCB_SHPR3 |= (0xFFUL << 16);
}

tcb_t *task_create(void (*entry)(void)) {
    if (num_tasks >= MAX_TASKS) {
        return 0;
    }

    tcb_t *tcb = &task_table[num_tasks];
    uint32_t *stack_top = &task_stacks[num_tasks][TASK_STACK_WORDS];
    uint32_t *sp = stack_top;

    /* Build a fake exception-return frame so the first switch-in looks
       identical, to the restore code, to resuming after a real switch.
       Hardware frame (low->high addr): R0,R1,R2,R3,R12,LR,PC,xPSR.
       We write top-down, so the first word we write lands at the
       highest address in the frame (xPSR), the last at the lowest (R0). */
    sp -= 1; *sp = 0x01000000;        /* xPSR: Thumb bit set */
    sp -= 1; *sp = (uint32_t)entry;   /* PC: task entry point */
    sp -= 1; *sp = 0xFFFFFFFD;        /* LR: dummy, tasks never return */
    sp -= 1; *sp = 0;                 /* R12 */
    sp -= 1; *sp = 0;                 /* R3 */
    sp -= 1; *sp = 0;                 /* R2 */
    sp -= 1; *sp = 0;                 /* R1 */
    sp -= 1; *sp = 0;                 /* R0 */
    sp -= 8;                          /* R4-R11: software frame, values irrelevant */

    tcb->sp = sp;
    tcb->state = TASK_READY;
    tcb->priority = 0;
    tcb->base_priority = 0;
    tcb->period_ticks = 0;
    tcb->deadline_ticks = 0;
    tcb->wake_tick = 0;
    tcb->next = 0;

    if (current_tcb == 0) {
        current_tcb = tcb;   /* first task created becomes the initial task */
    }

    num_tasks++;
    return tcb;
}

void task_yield(void) {
    SCB_ICSR = ICSR_PENDSVSET;
}

/* M1-only placeholder: naive round-robin over every created task.
   Replaced by the real priority-array scheduler in M2's scheduler.c. */
void scheduler_next(void) {
    int idx = 0;
    for (int i = 0; i < num_tasks; i++) {
        if (&task_table[i] == current_tcb) {
            idx = i;
            break;
        }
    }
    current_tcb = &task_table[(idx + 1) % num_tasks];
}
