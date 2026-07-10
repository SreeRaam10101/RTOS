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
    uint32_t period_ticks;
    uint32_t deadline_ticks;
    uint32_t wake_tick;
    struct tcb *next;
} tcb_t;

extern tcb_t *current_tcb;
extern tcb_t *ready_queue[MAX_PRIORITY_LEVELS];
extern volatile uint32_t tick_count;

void kernel_init(void);
tcb_t *task_create(void (*entry)(void), uint8_t priority);
void task_yield(void);

void scheduler_init(void);
void scheduler_next(void);
void scheduler_start(void);

void systick_init(void);

#endif
