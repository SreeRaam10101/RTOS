#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>
#include "kernel.h"

void delay(uint32_t ticks);
void delay_tick_check(void);

typedef struct {
    int count;
    tcb_t *wait_queue;
} semaphore_t;

void sem_init(semaphore_t *sem, int initial_count);
void sem_wait(semaphore_t *sem);
void sem_post(semaphore_t *sem);

typedef struct {
    int count;
    tcb_t *wait_queue;
    tcb_t *owner;
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

#endif
