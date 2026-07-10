#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

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

void kernel_init(void);
tcb_t *task_create(void (*entry)(void));
void kernel_start(void);
void task_yield(void);
void scheduler_next(void);

#endif
