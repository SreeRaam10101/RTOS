#include <stdint.h>
#include "queue.h"
#include "adc.h"

#define TIMER0_BASE      0x40000000UL
#define TIMER0_CTRL      (*(volatile uint32_t *)(TIMER0_BASE + 0x0))
#define TIMER0_VALUE     (*(volatile uint32_t *)(TIMER0_BASE + 0x4))
#define TIMER0_RELOAD    (*(volatile uint32_t *)(TIMER0_BASE + 0x8))
#define TIMER0_INTSTATUS (*(volatile uint32_t *)(TIMER0_BASE + 0xC))

#define TIMER_CTRL_EN        (1UL << 0)
#define TIMER_CTRL_IRQEN     (1UL << 3)
#define TIMER_INTSTATUS_IRQ  (1UL << 0)

#define NVIC_ISER0    (*(volatile uint32_t *)0xE000E100)
#define TIMER0_IRQN   8

#define SYSCLK_HZ 25000000UL
#define TICK_HZ   1000UL
#define SAMPLE_PERIOD_TICKS 20   /* ADC "conversion complete" fires every 20 ticks (20ms) */

/* Deterministic synthetic waveform standing in for a real ADC conversion
   result -- QEMU's mps2-an385 does not model a real ADC peripheral. Values
   are chosen so a 3-sample moving average (see main.c's Filter task)
   crosses main.c's ALERT_THRESHOLD twice per 8-sample cycle, making the
   alert path exactly reproducible for tests/m7_test.sh. Porting to real
   Pico hardware later replaces just the array read below with a real ADC
   register read -- everything else in this ISR (queue_push, NVIC/timer
   setup) carries over unchanged. */
static const uint16_t ADC_WAVEFORM[] = { 20, 21, 22, 24, 45, 46, 25, 20 };
#define ADC_WAVEFORM_LEN 8

static queue_t *g_sample_q = 0;
static uint32_t g_waveform_index = 0;

void Timer0_IRQHandler(void) {
    TIMER0_INTSTATUS = TIMER_INTSTATUS_IRQ;   /* write-1-to-clear */

    uint16_t sample = ADC_WAVEFORM[g_waveform_index];
    g_waveform_index = (g_waveform_index + 1) % ADC_WAVEFORM_LEN;

    if (g_sample_q != 0) {
        queue_push(g_sample_q, &sample);
    }
}

void adc_init(queue_t *sample_queue) {
    g_sample_q = sample_queue;

    TIMER0_RELOAD = (SYSCLK_HZ / TICK_HZ) * SAMPLE_PERIOD_TICKS - 1;
    TIMER0_VALUE = TIMER0_RELOAD;
    TIMER0_CTRL = TIMER_CTRL_EN | TIMER_CTRL_IRQEN;

    NVIC_ISER0 = (1UL << TIMER0_IRQN);
}
