/** 
 * Bao, a Lightweight Static Partitioning Hypervisor 
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Sandro Pinto <sandro.pinto@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details. 
 *
 */

#include "sysregs.h"

#define STACK_SIZE  0x4000

.section .start, "ax"
.global _start
_start:

    /* Valid Image header.  */
    /* HW reset vector.  */
    b skip_header
    nop

    /* text offset.  */
    .dword 0
    /* image size.  */
    .dword 0
    /* flags.  */
    .dword 8
    /* RES0  */
    .dword 0
    .dword 0
    .dword 0

    /* magic  */
    .dword 0x644d5241
    /* RES0  */
    .dword 0
    /* End of Image header.  */

skip_header:
    mrs x0, MPIDR_EL1
    and x0, x0, MPIDR_CPU_MASK

    /**
     * Check current exception level. If in:
     *     - el0 or el3, stop
     *     - el1, proceed
     *     - el2, jump to el1
     */
    mrs x1, currentEL
    lsr x1, x1, 2
    cmp x1, 0
    b.eq .
    cmp x1, 3
    b.eq .
    cmp x1, 1
    b.eq _enter_el1
    adr x1, boot_el2
    mov x2, 1
    str x2, [x1]
    mov x1, #0
    msr cntvoff_el2, x1
    mov x1, SPSR_EL1t | SPSR_F | SPSR_I | SPSR_A | SPSR_D
    msr spsr_el2, x1
    mov x1, HCR_RW_BIT
    msr hcr_el2, x1
    adr x1, _enter_el1
    msr elr_el2, x1
    eret

_enter_el1:
    adr x1, _exception_vector
    msr	VBAR_EL1, x1

    ldr x1, =MAIR_EL1_DFLT
    msr	MAIR_EL1, x1

    ldr x1, =0x0000000000802510
    msr TCR_EL1, x1

    adr x1, root_page_table
    msr TTBR0_EL1, x1

    // Enable floating point
    mov x1, #(3 << 20)
    msr CPACR_EL1, x1

    //TODO: invalidate caches, bp, .. ?

    tlbi	vmalle1
	dsb	nsh
	isb

    ldr x1, =(SCTLR_RES1 | SCTLR_M | SCTLR_C | SCTLR_I)
    msr SCTLR_EL1, x1

    tlbi	vmalle1
	dsb	nsh
	isb

.pushsection .data
.global master_cpu
master_cpu: .8byte 0x0
master_cpu_set: .8byte 0x0
.popsection
    adr     x1, master_cpu_set
    mov     x2, 1
1:
    ldaxr   x3, [x1]
    cbnz    x3, skip
    stlxr   w3, x2, [x1]
    cbnz    w3, 1b
    adr     x1, master_cpu
    str     x0, [x1]

    ldr x16, =__bss_start 
    ldr x17, =__bss_end   
    bl  clear

    .pushsection .data
    .align 3
wait_flag:
    .dword 0x0
    .popsection

    adr x1, wait_flag
    mov x2, #1
    str x2, [x1]

skip:
1:
    adr x1, wait_flag
    ldr x2, [x1]
    cbz x2, 1b

    mov x3, #SPSel_SP							
	msr SPSEL, x3	

    adr x1, _stack_base
    ldr x2, =STACK_SIZE
    add x1, x1, x2
#ifndef SINGLE_CORE
    madd x1, x0, x2, x1
#endif
    mov sp, x1
   
    //TODO: other c runtime init (ctors, etc...)

    b _init
    b _exit

.global psci_wake_up
psci_wake_up:
    b .

 .func clear
clear:
2:
	cmp	x16, x17			
	b.ge 1f				
	str	xzr, [x16], #8	
	b	2b				
1:
	ret
.endfunc
