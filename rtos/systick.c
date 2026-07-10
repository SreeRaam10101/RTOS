#include <stdint.h>
#include "kernel.h"

#define SYST_CSR   (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR   (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018)

#define SYST_CSR_ENABLE     (1UL << 0)
#define SYST_CSR_TICKINT    (1UL << 1)
#define SYST_CSR_CLKSOURCE  (1UL << 2)

#define SYSCLK_HZ 25000000UL
#define TICK_HZ   1000UL

#define SCB_ICSR       (*(volatile uint32_t *)0xE000ED04)
#define ICSR_PENDSVSET (1UL << 28)

volatile uint32_t tick_count = 0;

void systick_init(void) {
    SYST_RVR = (SYSCLK_HZ / TICK_HZ) - 1;   /* 24999: 1ms at 25MHz */
    SYST_CVR = 0;                            /* writing clears the counter */
    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;
}

void SysTick_Handler(void) {
    tick_count++;
    /* Pend a switch on every tick, unconditionally. If the top ready
       priority has only one task, scheduler_next()'s rotation is a no-op
       and this is a cheap same-task resume; if there are several, this is
       what shares the CPU between them without either task ever yielding. */
    SCB_ICSR = ICSR_PENDSVSET;
}
