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

#include <plat.h>

MEMORY {
    RAM (rwx)    : ORIGIN = PLAT_MEM_BASE, LENGTH = PLAT_MEM_SIZE    
}

ENTRY(_start)

SECTIONS {

    .start : {
        *(.start)
    } 

    .text :  {
        *(.text*)
    } 

    .rodata :  {
        *(.rodata*)
    }

    .data : {
        *(.data .data.*)
        PROVIDE(__global_pointer$ = . + 0x800);
        *(.sdata .sdata.* .sdata2.*)
    }

    .bss (NOLOAD) :  {	
        __bss_start = .;
        *(.bss* .sbss*) 
        *(COMMON)	
        __bss_end = .;
    }

    . = ALIGN(16);
    PROVIDE(_stack_base = .);
    . = . + 2M; /* 2M of total stack size */
    PROVIDE(_heap_base = .);
}
