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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <cpu.h>
#include <wfi.h>
#include <spinlock.h>
#include <plat.h>
#include <irq.h>
#include <uart.h>
#include <timer.h>
#include <pmu.h>
#include <gic.h>


#define NO_OP_FAULT 0x10000000

void main(void){

    printf("Welcome to the No-Op Benchmark!\n");
    pmu_start();
    pmu_reset();
    char *what = (char *)NO_OP_FAULT; // No-Op Fault
    ccnt_t before, after;
    ccnt_t results[100];
    printf("START_DATA\n");
    for (int i = 0; i < 100; i++) {

        before = pmu_cycle_get();
        *what = '!'; 
        asm("nop");
        after = pmu_cycle_get();
        results[i] = after - before;
    }
    for (int i = 0; i < 100; i++) {
        printf("Cycles: %llu\n", results[i]);
    }
    printf("END_DATA\n");

    int *x = 0;
    *x = 4;
}

