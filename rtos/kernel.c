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
    SCB_SHPR3 |= (0xFFUL << 16);
}

uint32_t critical_enter(void) {
    uint32_t primask;
    __asm__ volatile ("mrs %0, primask" : "=r" (primask));
    __asm__ volatile ("cpsid i");
    return primask;
}

void critical_exit(uint32_t saved_primask) {
    __asm__ volatile ("msr primask, %0" : : "r" (saved_primask));
}

void ready_enqueue(tcb_t *tcb) {
    tcb->state = TASK_READY;
    tcb->next = 0;
    if (ready_queue[tcb->priority] == 0) {
        ready_queue[tcb->priority] = tcb;
        return;
    }
    tcb_t *tail = ready_queue[tcb->priority];
    while (tail->next != 0) {
        tail = tail->next;
    }
    tail->next = tcb;
}

void ready_remove(tcb_t *tcb) {
    tcb_t **pp = &ready_queue[tcb->priority];
    while (*pp != 0 && *pp != tcb) {
        pp = &(*pp)->next;
    }
    if (*pp == tcb) {
        *pp = tcb->next;
        tcb->next = 0;
    }
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
    tcb->priority = priority;
    tcb->base_priority = priority;
    tcb->id = (uint8_t)num_tasks;
    tcb->period_ticks = 0;
    tcb->deadline_ticks = 0;
    tcb->wake_tick = 0;
    tcb->abs_deadline = 0;

    ready_enqueue(tcb);   /* sets state = TASK_READY, appends to the tail */

    num_tasks++;
    return tcb;
}

void task_yield(void) {
    SCB_ICSR = ICSR_PENDSVSET;
}
