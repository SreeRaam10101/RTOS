#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

#define MAX_PRIORITY_LEVELS 4

typedef enum { TASK_READY, TASK_RUNNING, TASK_BLOCKED, TASK_UNUSED } task_state_t;

typedef struct tcb {
    uint32_t *sp;
    task_state_t state;
    uint8_t priority;
    uint8_t base_priority;
    uint8_t id;
    uint32_t period_ticks;
    uint32_t deadline_ticks;
    uint32_t wake_tick;
    uint32_t abs_deadline;
    volatile uint32_t remaining_wcet_ticks;   /* CPU ticks left to simulate; decremented by SysTick, not by the task itself */
    struct tcb *next;
} tcb_t;

extern tcb_t *current_tcb;
#ifdef SCHED_EDF
tcb_t *ready_dequeue_min_deadline(void);
#else
extern tcb_t *ready_queue[MAX_PRIORITY_LEVELS];
#endif
extern volatile uint32_t tick_count;

void kernel_init(void);
tcb_t *task_create(void (*entry)(void), uint8_t priority);
void task_yield(void);

void ready_enqueue(tcb_t *tcb);
void ready_remove(tcb_t *tcb);

uint32_t critical_enter(void);
void critical_exit(uint32_t saved_primask);

void scheduler_init(void);
void scheduler_next(void);
void scheduler_start(void);

void systick_init(void);

#endif
