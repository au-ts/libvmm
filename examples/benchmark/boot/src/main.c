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


#include <stdio.h>
#include <sysregs.h>
#include <timer.h>

uint8_t buf[(((unsigned long long)IMG_SZ_KB) * 1024) - (78 * 1024)] __attribute__((section(".data")));

uint64_t boot_time = 0xdeadbeef;

void main(void){

    // add a small delay to not collide with the hypervisors ouput
    timer_wait(TIME_MS(500));

    printf("boottime-baremetal %ld\n", boot_time);

}
