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

#define XSTR(S) STR(S)

#define TIMER_INTERVAL (TIME_MS(10))
#define NUM_WARMUP   (10)
#define NUM_SAMPLES  (50)
#define COL_SIZE        15   
#define SAMPLE_FORMAT   "%" XSTR(COL_SIZE) "d"
#define HEADER_FORMAT   "%" XSTR(COL_SIZE) "d"
volatile size_t sample_count;
volatile size_t irq_count;
uint64_t prev_time = 0;
uint64_t irqlat_samples[16][NUM_SAMPLES];
unsigned arrival_order[NUM_SAMPLES][16];
bool first;

#define L1_CACHE_SIZE   (32*1024)
#define CACHE_LINE_SIZE (64)
volatile uint8_t cache_l1[L1_CACHE_SIZE] __attribute__((aligned(L1_CACHE_SIZE)));


struct {
    unsigned id;
    unsigned prio;
} priorities[16];

void generic_handler(unsigned id) {
    uint64_t time = timer_get();
    irqlat_samples[irq_count][sample_count] = time - prev_time;
    arrival_order[sample_count][irq_count] = id;

    prev_time = time;
    irq_count+=1;

    uint32_t irqs = *((uint32_t*)0xa0000004);
    unsigned offset = id >= 136 ? (id-136+8) : (id-121);
    *((uint32_t*)0xa0000004) = irqs & ~(1 << offset);

    if(first) {
        first = false;
        *((uint32_t*)0xa0000004) |= 0xff00;
    }
}

void print_samples() {

    printf("--------------------------------\n");
    for(size_t i = 0; i < 16; i++) {
        printf(HEADER_FORMAT, i);
    }
    printf("\n");

    for(size_t i = 0; i < NUM_SAMPLES; i++) {
        for(size_t j = 0; j < 16; j++) {
            printf(SAMPLE_FORMAT, irqlat_samples[j][i]);
        }
        printf("\n");
    }
    
}

static inline unsigned get_prio(unsigned id) {
    for(int i = 0; i < 16; i++) {
        if (priorities[i].id == id) {
            return priorities[i].prio;
        }
    }
    return 0;
}

static inline void print_prios() {
    for(int i = 0; i < 16; i++) {
        printf("%d: %d\n", priorities[i].id, priorities[i].prio);
    }
}

void check_priority_order() {

    size_t passed = 0;
    size_t failed = 0;
    for (int j = 0; j < NUM_SAMPLES; j++) {
        unsigned prev_prio = 0x0;
        bool overall_prio_order_respected = true;
        for(int i = 0; i < 16; i++) {
            // printf("%d ", arrival_order[j][i]);
            bool prio_order_ok;
            if (get_prio(arrival_order[j][i]) >= prev_prio) prio_order_ok = true;
            else prio_order_ok = false;
            prev_prio = get_prio(arrival_order[j][i]);
            overall_prio_order_respected &= prio_order_ok;
        }
        if (overall_prio_order_respected) {
            passed++;
        } else {
            failed++;
        }
    }

    printf("\n");
    printf("priority order passed %d/%d times \n", passed, NUM_SAMPLES);
    printf("priority order failed %d/%d times \n", failed, NUM_SAMPLES);
}

void clear_caches() {
    for (size_t i = 0; i < L1_CACHE_SIZE; i+= CACHE_LINE_SIZE) {
        cache_l1[i] = i;
    }
    asm volatile("ic iallu\n\t");
}

void main(void){

    if(!cpu_is_master()) {
        return;
    }

    printf("Bao baremetal irq storm latency and priority test\n");

    gic_set_prio(121, 0xff);
    uint8_t prio_bits = gic_get_prio(121);
    // printf("prio bits 0x%x\n", prio_bits);

    size_t prio_i = 0;
    for(int i = 121; i < 121 + 8; i++) {
        irq_set_handler(i, generic_handler);
        irq_enable(i);
        unsigned prio = (15 - (i - 120)) << 4;
        irq_set_prio(i, prio);
        priorities[prio_i].id = i;
        priorities[prio_i++].prio = prio;
    }

    for(int i = 136; i < 136 + 8; i++) {
        irq_set_handler(i, generic_handler);
        irq_enable(i);
        unsigned prio = (8 - (i - 135)) << 4;
        irq_set_prio(i, prio);
        priorities[prio_i].id = i;
        priorities[prio_i++].prio = prio;
    }

    irq_set_prio(128, 0);
    priorities[7].prio = 0;
    // print_prios();

    printf("Press 's' to start...\n");
    while(uart_getchar() != 's');

    sample_count = 0;
    for (int i = 0; i < NUM_WARMUP; i++) {
        first = true;
        irq_count = 0;
        asm ("isb");
        prev_time = timer_get();
        *((uint32_t*)0xa0000004) = 0xff;

        while(irq_count < 16);
        clear_caches();
    }

    sample_count = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        first = true;
        irq_count = 0;
        asm ("isb");
        prev_time = timer_get();
        *((uint32_t*)0xa0000004) = 0xff;

        while(irq_count < 16);
        sample_count++;
        clear_caches();
    }

    check_priority_order();
    print_samples();

    while(1);
}
