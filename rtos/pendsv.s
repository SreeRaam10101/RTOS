    .syntax unified
    .cpu cortex-m3
    .thumb

    .section .text
    .thumb_func
    .global PendSV_Handler
PendSV_Handler:
    mrs r0, psp
    stmdb r0!, {r4-r11}

    ldr r1, =current_tcb
    ldr r2, [r1]
    str r0, [r2]

    push {lr}              /* preserve EXC_RETURN across the C call below */
    bl scheduler_next
    pop {lr}

    ldr r1, =current_tcb
    ldr r2, [r1]
    ldr r0, [r2]
    ldmia r0!, {r4-r11}
    msr psp, r0

    bx lr

    .thumb_func
    .global scheduler_start
scheduler_start:
    bl scheduler_next       /* NEW: pick the correct task by priority, not creation order */

    ldr r0, =current_tcb
    ldr r0, [r0]           /* r0 = current_tcb (set by task_create) */
    ldr r0, [r0]           /* r0 = current_tcb->sp (first struct member) */

    ldmia r0!, {r4-r11}     /* pop the software frame; r0 now points at hw frame */
    msr psp, r0

    movs r0, #2             /* CONTROL: SPSEL=1 (use PSP), nPRIV=0 */
    msr control, r0
    isb

    pop {r0-r3, r12, lr}     /* now using PSP: pop the first 6 hw-frame words */
    pop {r1}                 /* r1 = PC (task entry point) */
    add sp, sp, #4            /* discard xPSR word */
    cpsie i                   /* ensure interrupts enabled */
    bx r1                     /* jump into the first task — never returns */
