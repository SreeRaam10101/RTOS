#include <stdint.h>
#include "gpio.h"

/* CMSDK AHB GPIO0 base on mps2-an385. QEMU models this peripheral as an
   unimplemented stub (create_unimplemented_device("cmsdk-ahb-gpio", ...) in
   hw/arm/mps2.c) -- writes are accepted but produce no observable, readable
   state in QEMU, so this offset is a placeholder consistent with a typical
   CMSDK-style GPIO DATA register, not something QEMU can confirm. A real
   Pico hardware port replaces this entire file with RP2040 SIO
   GPIO_OUT/GPIO_OUT_SET register access -- a completely different
   peripheral family -- so no address here is expected to carry over. */
#define GPIO0_BASE 0x40010000UL
#define GPIO0_DATA (*(volatile uint32_t *)(GPIO0_BASE + 0x00))

void gpio_set_alert_pin(int on) {
    if (on) {
        GPIO0_DATA |= 1u;
    } else {
        GPIO0_DATA &= ~1u;
    }
}
