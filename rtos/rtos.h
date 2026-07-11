#ifndef RTOS_H
#define RTOS_H

#include <stdint.h>
#include "kernel.h"

tcb_t *periodic_task_create(void (*entry)(void), uint8_t priority,
                             uint32_t period_ticks, uint32_t deadline_ticks,
                             uint32_t wcet_ticks);
void task_wait_for_release(void);
uint32_t periodic_get_miss_count(tcb_t *tcb);

int rms_check(void);
uint32_t rms_get_utilization_x10000(void);
uint32_t rms_get_bound_x10000(void);

#endif
