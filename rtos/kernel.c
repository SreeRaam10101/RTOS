#include <stdint.h>
#include "kernel.h"

#define MAX_TASKS 4
#define TASK_STACK_WORDS 256

#define SCB_ICSR   (*(volatile uint32_t *)0xE000ED04)
#define SCB_SHPR3  (*(volatile uint32_t *)0xE000ED20)
#define ICSR_PENDSVSET (1UL << 28)

static tcb_t task_table[MAX_TASKS];
static uint32_t task_stacks[MAX_TASKS][TASK_STACK_WORDS] __attribute__((aligned(8)));
static int num_tasks = 0;

tcb_t *current_tcb = 0;
tcb_t *ready_queue[MAX_PRIORITY_LEVELS] = {0};

void kernel_init(void) {
    /* PendSV must be the lowest-priority exception: a context switch must
       never preempt another exception's critical section. SysTick is left
       at its power-on-reset priority (0, the highest), so it can always
       pend PendSV and is never blocked by it. */
    SCB_SHPR3 |= (0xFFUL << 16);
}

tcb_t *task_create(void (*entry)(void), uint8_t priority) {
    if (num_tasks >= MAX_TASKS || priority >= MAX_PRIORITY_LEVELS) {
        return 0;
    }

    tcb_t *tcb = &task_table[num_tasks];
    uint32_t *stack_top = &task_stacks[num_tasks][TASK_STACK_WORDS];
    uint32_t *sp = stack_top;

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
    tcb->priority = priority;
    tcb->base_priority = priority;
    tcb->period_ticks = 0;
    tcb->deadline_ticks = 0;
    tcb->wake_tick = 0;

    /* Prepend into this priority's ready list. Creation order doesn't need
       to matter for fairness -- scheduler_next() rotates at runtime. */
    tcb->next = ready_queue[priority];
    ready_queue[priority] = tcb;

    num_tasks++;
    return tcb;
}

void task_yield(void) {
    SCB_ICSR = ICSR_PENDSVSET;
}
