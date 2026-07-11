#include <stdint.h>
#include "kernel.h"
#include "sync.h"

#ifndef PRIORITY_INHERITANCE_ENABLED
#define PRIORITY_INHERITANCE_ENABLED 1
#endif

/* ---- delay() : sorted-ascending-by-wake_tick wait-list ---- */

static tcb_t *delay_list = 0;

static void delay_list_insert(tcb_t *tcb) {
    tcb_t **pp = &delay_list;
    while (*pp != 0 && (*pp)->wake_tick <= tcb->wake_tick) {
        pp = &(*pp)->next;
    }
    tcb->next = *pp;
    *pp = tcb;
}

void delay(uint32_t ticks) {
    uint32_t s = critical_enter();
    current_tcb->wake_tick = tick_count + ticks;
    current_tcb->state = TASK_BLOCKED;
    delay_list_insert(current_tcb);
    critical_exit(s);
    task_yield();
}

void delay_tick_check(void) {
    uint32_t s = critical_enter();
    while (delay_list != 0 && delay_list->wake_tick <= tick_count) {
        tcb_t *tcb = delay_list;
        delay_list = delay_list->next;
        tcb->next = 0;
        ready_enqueue(tcb);
    }
    critical_exit(s);
}

/* ---- shared helper: unlink the highest-priority (lowest priority
   number) waiter from a wait_queue, or return 0 if empty ---- */

static tcb_t *wait_queue_pop_highest(tcb_t **wait_queue) {
    if (*wait_queue == 0) {
        return 0;
    }
    tcb_t **pp = wait_queue;
    tcb_t **best = wait_queue;
    while (*pp != 0) {
        if ((*pp)->priority < (*best)->priority) {
            best = pp;
        }
        pp = &(*pp)->next;
    }
    tcb_t *tcb = *best;
    *best = tcb->next;
    tcb->next = 0;
    return tcb;
}

static void boost_priority(tcb_t *tcb, uint8_t new_priority) {
    if (new_priority >= tcb->priority) {
        return;   /* only boost to a numerically lower (higher) priority */
    }
    if (tcb->state == TASK_READY) {
        ready_remove(tcb);
        tcb->priority = new_priority;
        ready_enqueue(tcb);
    } else {
        tcb->priority = new_priority;
    }
}

static void restore_priority(tcb_t *tcb) {
    if (tcb->priority == tcb->base_priority) {
        return;
    }
    if (tcb->state == TASK_READY) {
        ready_remove(tcb);
        tcb->priority = tcb->base_priority;
        ready_enqueue(tcb);
    } else {
        tcb->priority = tcb->base_priority;
    }
}

/* ---- semaphore ---- */

void sem_init(semaphore_t *sem, int initial_count) {
    sem->count = initial_count;
    sem->wait_queue = 0;
}

void sem_wait(semaphore_t *sem) {
    uint32_t s = critical_enter();
    if (sem->count > 0) {
        sem->count--;
        critical_exit(s);
        return;
    }
    current_tcb->state = TASK_BLOCKED;
    current_tcb->next = sem->wait_queue;
    sem->wait_queue = current_tcb;
    critical_exit(s);
    task_yield();
}

void sem_post(semaphore_t *sem) {
    uint32_t s = critical_enter();
    tcb_t *woken = wait_queue_pop_highest(&sem->wait_queue);
    if (woken != 0) {
        ready_enqueue(woken);
    } else {
        sem->count++;
    }
    critical_exit(s);
}

/* ---- mutex ---- */

void mutex_init(mutex_t *m) {
    m->count = 1;
    m->wait_queue = 0;
    m->owner = 0;
}

void mutex_lock(mutex_t *m) {
    uint32_t s = critical_enter();
    if (m->count > 0) {
        m->count--;
        m->owner = current_tcb;
        critical_exit(s);
        return;
    }
#if PRIORITY_INHERITANCE_ENABLED
    if (m->owner != 0 && m->owner->priority > current_tcb->priority) {
        boost_priority(m->owner, current_tcb->priority);
    }
#endif
    current_tcb->state = TASK_BLOCKED;
    current_tcb->next = m->wait_queue;
    m->wait_queue = current_tcb;
    critical_exit(s);
    task_yield();
    /* ownership was already assigned to us by mutex_unlock() before we
       were woken -- nothing left to do here. */
}

void mutex_unlock(mutex_t *m) {
    uint32_t s = critical_enter();
    if (current_tcb->priority != current_tcb->base_priority) {
        restore_priority(current_tcb);
    }
    m->owner = 0;
    tcb_t *woken = wait_queue_pop_highest(&m->wait_queue);
    if (woken != 0) {
        m->owner = woken;   /* direct hand-off, count stays at 0 */
        ready_enqueue(woken);
    } else {
        m->count = 1;
    }
    critical_exit(s);
}
