#include <stdint.h>
#include "kernel.h"
#include "queue.h"

void queue_init(queue_t *q, void *buf, uint32_t capacity, uint32_t item_size) {
    q->buf = buf;
    q->capacity = capacity;
    q->item_size = item_size;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->overflow_count = 0;
    sem_init(&q->items_available, 0);
}

int queue_push(queue_t *q, const void *item) {
    uint32_t s = critical_enter();
    if (q->count >= q->capacity) {
        q->overflow_count++;
        critical_exit(s);
        return 0;
    }
    uint8_t *slot = (uint8_t *)q->buf + (q->tail * q->item_size);
    const uint8_t *src = (const uint8_t *)item;
    for (uint32_t i = 0; i < q->item_size; i++) {
        slot[i] = src[i];
    }
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    critical_exit(s);

    /* sem_post() is itself guarded by critical_enter()/critical_exit(), so
       it is safe to call here whether queue_push() runs in an ISR (e.g.
       Timer0_IRQHandler) or in task context -- same convention already
       used by SysTick_Handler calling delay_tick_check(). */
    sem_post(&q->items_available);
    return 1;
}

void queue_pop(queue_t *q, void *out) {
    sem_wait(&q->items_available);

    uint32_t s = critical_enter();
    uint8_t *slot = (uint8_t *)q->buf + (q->head * q->item_size);
    uint8_t *dst = (uint8_t *)out;
    for (uint32_t i = 0; i < q->item_size; i++) {
        dst[i] = slot[i];
    }
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    critical_exit(s);
}
