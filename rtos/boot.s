    .syntax unified
    .cpu cortex-m3
    .thumb

    .section .isr_vector, "a"
    .align 2
    .global _vectors
_vectors:
    .word _estack            /* initial MSP */
    .word Reset_Handler      /* Reset */
    .word Default_Handler    /* NMI */
    .word Default_Handler    /* HardFault */
    .word Default_Handler    /* MemManage */
    .word Default_Handler    /* BusFault */
    .word Default_Handler    /* UsageFault */
    .word 0                  /* reserved */
    .word 0                  /* reserved */
    .word 0                  /* reserved */
    .word 0                  /* reserved */
    .word Default_Handler    /* SVCall */
    .word Default_Handler    /* DebugMonitor */
    .word 0                  /* reserved */
    .word PendSV_Handler     /* PendSV */
    .word SysTick_Handler    /* SysTick */

    .section .text
    .thumb_func
    .global Reset_Handler
Reset_Handler:
    /* copy .data from flash to RAM */
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
copy_data:
    cmp r1, r2
    bcs copy_data_done
    ldr r3, [r0], #4
    str r3, [r1], #4
    b copy_data
copy_data_done:

    /* zero .bss */
    ldr r1, =_sbss
    ldr r2, =_ebss
    movs r3, #0
zero_bss:
    cmp r1, r2
    bcs zero_bss_done
    str r3, [r1], #4
    b zero_bss
zero_bss_done:

    bl main
hang:
    b hang

    .thumb_func
    .global Default_Handler
Default_Handler:
    b .
