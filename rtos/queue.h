#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>
#include "sync.h"

typedef struct {
    void *buf;                       /* caller-provided backing array */
    uint32_t capacity;                /* max number of items */
    uint32_t item_size;               /* bytes per item */
    uint32_t head;                    /* next slot to pop */
    uint32_t tail;                    /* next slot to push */
    uint32_t count;                   /* items currently queued */
    semaphore_t items_available;      /* blocks a consumer when empty */
    uint32_t overflow_count;          /* incremented, not silently dropped, when full */
} queue_t;

void queue_init(queue_t *q, void *buf, uint32_t capacity, uint32_t item_size);
int  queue_push(queue_t *q, const void *item);   /* ISR-safe; non-blocking; returns 0 if full */
void queue_pop(queue_t *q, void *out);           /* blocks calling task until an item is available */

#endif
