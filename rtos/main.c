#include <stdint.h>
#include "kernel.h"
#include "uart.h"
#include "sync.h"

static semaphore_t data_ready;
static mutex_t counter_mutex;
static uint32_t shared_counter = 0;

static void producer_entry(void) {
    for (;;) {
        delay(100);
        sem_post(&data_ready);
    }
}

static void consumer_entry(void) {
    for (;;) {
        sem_wait(&data_ready);
        mutex_lock(&counter_mutex);
        shared_counter++;
        uart_puts("consumer: shared_counter=");
        uart_put_uint32(shared_counter);
        uart_puts("\n");
        mutex_unlock(&counter_mutex);
    }
}

int main(void) {
    uart_init();
    kernel_init();
    scheduler_init();

    sem_init(&data_ready, 0);
    mutex_init(&counter_mutex);

    task_create(producer_entry, 0);
    task_create(consumer_entry, 0);

    systick_init();
    scheduler_start();

    for (;;) { }  /* unreachable */
}
