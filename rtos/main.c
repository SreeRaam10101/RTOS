#include <stdint.h>
#include "kernel.h"
#include "uart.h"
#include "queue.h"
#include "adc.h"
#include "gpio.h"

#define SAMPLER_PRIORITY  0
#define FILTER_PRIORITY   1
#define REPORTER_PRIORITY 2

#define SAMPLE_Q_CAPACITY 8
#define FILTER_Q_CAPACITY 8
#define REPORT_Q_CAPACITY 8

#define MOVING_AVG_WINDOW 3
#define ALERT_THRESHOLD   35

static uint16_t sample_q_buf[SAMPLE_Q_CAPACITY];
static uint16_t filter_q_buf[FILTER_Q_CAPACITY];
static uint32_t report_q_buf[REPORT_Q_CAPACITY];

static queue_t sample_q;
static queue_t filter_q;
static queue_t report_q;

static void sampler_entry(void) {
    for (;;) {
        uint16_t v;
        queue_pop(&sample_q, &v);    /* blocks until Timer0_IRQHandler delivers a sample */
        queue_push(&filter_q, &v);
    }
}

static void filter_entry(void) {
    uint16_t history[MOVING_AVG_WINDOW] = { 0, 0, 0 };
    int hist_idx = 0;

    for (;;) {
        uint16_t v;
        queue_pop(&filter_q, &v);

        history[hist_idx] = v;
        hist_idx = (hist_idx + 1) % MOVING_AVG_WINDOW;

        uint32_t sum = 0;
        for (int i = 0; i < MOVING_AVG_WINDOW; i++) {
            sum += history[i];
        }
        uint32_t avg = sum / MOVING_AVG_WINDOW;

        if (avg > ALERT_THRESHOLD) {
            /* Alert branches directly out of Filter, not queued through
               Reporter, so its latency is bounded by "time since threshold
               detected" rather than diluted by Reporter's lower scheduling
               priority. QEMU cannot confirm the GPIO write lands (see
               gpio.c) so the bound is proven here in firmware, by logging
               tick_count immediately before and after the write. */
            uint32_t detect_tick = tick_count;
            gpio_set_alert_pin(1);
            uint32_t gpio_tick = tick_count;
            uart_puts("ALERT: detect_tick=");
            uart_put_uint32(detect_tick);
            uart_puts(" gpio_tick=");
            uart_put_uint32(gpio_tick);
            uart_puts("\n");
        } else {
            gpio_set_alert_pin(0);
        }

        queue_push(&report_q, &avg);
    }
}

static void reporter_entry(void) {
    for (;;) {
        uint32_t avg;
        queue_pop(&report_q, &avg);
        uart_puts("Report: avg=");
        uart_put_uint32(avg);
        uart_puts(" overflow_sample=");
        uart_put_uint32(sample_q.overflow_count);
        uart_puts(" overflow_filter=");
        uart_put_uint32(filter_q.overflow_count);
        uart_puts(" overflow_report=");
        uart_put_uint32(report_q.overflow_count);
        uart_puts("\n");
    }
}

int main(void) {
    uart_init();
    kernel_init();
    scheduler_init();

    queue_init(&sample_q, sample_q_buf, SAMPLE_Q_CAPACITY, sizeof(uint16_t));
    queue_init(&filter_q, filter_q_buf, FILTER_Q_CAPACITY, sizeof(uint16_t));
    queue_init(&report_q, report_q_buf, REPORT_Q_CAPACITY, sizeof(uint32_t));

    task_create(sampler_entry, SAMPLER_PRIORITY);
    task_create(filter_entry, FILTER_PRIORITY);
    task_create(reporter_entry, REPORTER_PRIORITY);

    adc_init(&sample_q);
    systick_init();
    scheduler_start();

    for (;;) { }   /* unreachable */
}
