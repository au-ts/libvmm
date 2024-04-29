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

#include <page_tables.h>

.section .page_tables, "aw"
.balign PAGE_SIZE

l1_page_table:

    //0x00000000 - 0x7FFFFFFF   RAM
    .set ADDR, 0
    .rept   2
    .8byte (PTE_SUPERPAGE | PTE_MEM_FLAGS) + ADDR
    .set ADDR, ADDR + L1_BLOCK_SIZE
    .endr

     //0x80000000 - 0xFFFFFFFF  DEVICES
    .rept   2
    .8byte (PTE_SUPERPAGE | PTE_DEV_FLAGS) + ADDR
    .set ADDR, ADDR + L1_BLOCK_SIZE
    .endr

    .balign PAGE_SIZE

.global root_page_table
root_page_table:
    .8byte (PTE_TABLE) + l1_page_table
    .balign PAGE_SIZE