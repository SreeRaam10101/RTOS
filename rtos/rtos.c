#include <stdint.h>
#include "kernel.h"
#include "sync.h"
#include "rtos.h"

#define MAX_PERIODIC_TASKS 4

typedef struct {
    tcb_t *tcb;
    uint32_t period_ticks;
    uint32_t deadline_ticks;
    uint32_t wcet_ticks;
    uint32_t next_release;
    uint32_t miss_count;
} periodic_info_t;

static periodic_info_t periodic_tasks[MAX_PERIODIC_TASKS];
static int num_periodic_tasks = 0;

static uint32_t last_u_scaled = 0;
static uint32_t last_bound_scaled = 0;

static periodic_info_t *find_periodic_info(tcb_t *tcb) {
    for (int i = 0; i < num_periodic_tasks; i++) {
        if (periodic_tasks[i].tcb == tcb) {
            return &periodic_tasks[i];
        }
    }
    return 0;
}

tcb_t *periodic_task_create(void (*entry)(void), uint8_t priority,
                             uint32_t period_ticks, uint32_t deadline_ticks,
                             uint32_t wcet_ticks) {
    if (num_periodic_tasks >= MAX_PERIODIC_TASKS) {
        return 0;
    }

    tcb_t *tcb = task_create(entry, priority);
    if (tcb == 0) {
        return 0;
    }

    tcb->period_ticks = period_ticks;
    tcb->deadline_ticks = deadline_ticks;

    periodic_info_t *info = &periodic_tasks[num_periodic_tasks];
    info->tcb = tcb;
    info->period_ticks = period_ticks;
    info->deadline_ticks = deadline_ticks;
    info->wcet_ticks = wcet_ticks;
    info->next_release = tick_count + period_ticks;
    info->miss_count = 0;
    num_periodic_tasks++;

    return tcb;
}

void task_wait_for_release(void) {
    periodic_info_t *info = find_periodic_info(current_tcb);

    uint32_t this_instance_release = info->next_release - info->period_ticks;
    uint32_t this_instance_deadline = this_instance_release + info->deadline_ticks;
    if (tick_count > this_instance_deadline) {
        info->miss_count++;
    }

    uint32_t next = info->next_release;
    info->next_release += info->period_ticks;
    block_until(next);
}

uint32_t periodic_get_miss_count(tcb_t *tcb) {
    periodic_info_t *info = find_periodic_info(tcb);
    return (info != 0) ? info->miss_count : 0;
}

int rms_check(void) {
    static const uint32_t rms_bound_x10000[] = { 0, 10000, 8284, 7798, 7568 };

    uint32_t u_scaled = 0;
    for (int i = 0; i < num_periodic_tasks; i++) {
        u_scaled += (periodic_tasks[i].wcet_ticks * 10000UL) / periodic_tasks[i].period_ticks;
    }

    uint32_t bound = 0;
    if (num_periodic_tasks >= 1 && num_periodic_tasks <= 4) {
        bound = rms_bound_x10000[num_periodic_tasks];
    }

    last_u_scaled = u_scaled;
    last_bound_scaled = bound;

    return (u_scaled <= bound) ? 1 : 0;
}

uint32_t rms_get_utilization_x10000(void) {
    return last_u_scaled;
}

uint32_t rms_get_bound_x10000(void) {
    return last_bound_scaled;
}
