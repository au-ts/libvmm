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
#define NUM_SAMPLES  (50)
#define NUM_WARMUPS  (10)
#define COL_SIZE        20   
#define SAMPLE_FORMAT   "%" XSTR(COL_SIZE) "d"
#define HEADER_FORMAT   "%" XSTR(COL_SIZE) "s"
volatile size_t sample_count;
uint64_t next_tick;
uint64_t curr_time;

uint64_t irqlat_samples[NUM_SAMPLES];
uint64_t irqlat_end_samples[NUM_SAMPLES];

#define L1_CACHE_SIZE   (32*1024)
#define CACHE_LINE_SIZE (64)
volatile uint8_t cache_l1[L1_CACHE_SIZE] __attribute__((aligned(L1_CACHE_SIZE)));

#if 1

const size_t sample_events[] = {
    L1I_CACHE_REFILL ,
    L1I_CACHE_REFILL | EL2_ONLY,
    L1D_CACHE_REFILL,
    L1D_CACHE_REFILL | EL2_ONLY,
    L2D_CACHE_REFILL,
    L2D_CACHE_REFILL | EL2_ONLY,
    L1I_TLB_REFILL ,
    L1I_TLB_REFILL | EL2_ONLY,
    L1D_TLB_REFILL,
    L1D_TLB_REFILL | EL2_ONLY,
    MEM_ACCESS,
    MEM_ACCESS | EL2_ONLY,
    BUS_ACCESS,
    BUS_ACCESS | EL2_ONLY,
    EXC_TAKEN,
    EXC_TAKEN | EL2_ONLY,
    EXC_IRQ,
    EXC_IRQ | EL2_ONLY,
    INST_RETIRED,
    INST_RETIRED | EL2_ONLY,
};

const size_t sample_events_size = sizeof(sample_events)/sizeof(size_t);
unsigned long pmu_samples[sizeof(sample_events)/sizeof(size_t)][NUM_SAMPLES];
size_t pmu_used_counters = 0;

void pmu_setup_counters(size_t n, const size_t events[]){
    pmu_used_counters = n < pmu_num_counters()? n : pmu_num_counters();
    for(size_t i = 0; i < pmu_used_counters; i++){
        pmu_counter_set_event(i, events[i]);
        pmu_counter_enable(i);
    }
    pmu_cycle_enable(true);
}


void pmu_sample() {
    size_t n = pmu_num_counters();
    for(int i = 0; i < n; i++){
        pmu_samples[i][sample_count] = pmu_counter_get(i);
    }
     pmu_samples[31][sample_count] = pmu_cycle_get();
}

void pmu_setup(size_t start, size_t n) {
    pmu_setup_counters(n, &sample_events[start]);
    pmu_reset();
    pmu_start();
}

static inline void pmu_print_header() {
    for (size_t i = 0; i < pmu_used_counters; i++) {
        uint32_t event = pmu_counter_get_event(i);
        char const * descr =  pmu_event_descr[event & 0xffff]; 
        descr = descr ? descr : "";
        uint32_t priv_code = (event >> 24) & 0xc8;
        const char * priv = priv_code == 0xc8 ? "_el2" : 
                            priv_code == 0x08 ? "_el1+2" :
                            "_el1";
        char buf[COL_SIZE];
        snprintf(buf, COL_SIZE-1, "%s%s", descr, priv);
        printf(HEADER_FORMAT, buf);
    }
    printf(HEADER_FORMAT, "cycles");
}

static inline void pmu_print_samples(size_t i) {
    for (size_t j = 0; j < pmu_used_counters; j++) {
        printf(SAMPLE_FORMAT, pmu_samples[j][i]);
    }
    printf(SAMPLE_FORMAT, pmu_samples[31][i]);
}

# else 

void pmu_setup_counters(size_t n, unsigned long events[]) { }
void pmu_sample() { }
void pmu_setup() { }
void pmu_print_header() { }
void pmu_print_samples(size_t i) { }

#endif

void timer_handler(unsigned id){

    timer_disable();
    curr_time = timer_get();
    irqlat_samples[sample_count] = curr_time - next_tick;
    gic_eoi(id);
    for(volatile size_t i = 0; i < 15; i++);
    curr_time = timer_get();
    irqlat_end_samples[sample_count] = curr_time - next_tick;

    pmu_sample();

    sample_count += 1;
    if (sample_count < NUM_SAMPLES) {
        next_tick = timer_set(TIMER_INTERVAL);
        pmu_reset();
    } else {
        timer_disable();
        pmu_stop();
    }

    asm volatile("ic iallu\n\t");
}

void print_samples() {

    printf("--------------------------------\n");
    printf(HEADER_FORMAT, "irqlat");
    printf(HEADER_FORMAT, "irqlat-end");
    pmu_print_header();
    printf("\n");

    for(size_t i = 0; i < NUM_SAMPLES; i++) {
        printf(SAMPLE_FORMAT, irqlat_samples[i]);
        printf(SAMPLE_FORMAT, irqlat_end_samples[i]);
        pmu_print_samples(i);
        printf("\n");
    }
    
}

void main(void){

    if(!cpu_is_master()) {
        return;
    }

    printf("Bao baremetal irq latency test\n");
    printf("timer freq: %d\n", TIMER_FREQ);

    irq_set_handler(TIMER_IRQ_ID, timer_handler);
    irq_enable(TIMER_IRQ_ID);
    irq_set_prio(TIMER_IRQ_ID, IRQ_MAX_PRIO);

    sample_count = 0;
    next_tick = timer_set(TIMER_INTERVAL);
    while(sample_count < NUM_WARMUPS);

    while(1) {
        printf("Press 's' to start...\n");
        while(uart_getchar() != 's');

        size_t i = 0;
        while(i < sample_events_size) {
            sample_count = 0;
            next_tick = timer_set(TIMER_INTERVAL);
            pmu_setup(i, sample_events_size - i);

            while(sample_count < NUM_SAMPLES) {
                for (size_t i = 0; i < L1_CACHE_SIZE; i+= CACHE_LINE_SIZE) {
                    cache_l1[i] = i;
                }
            }

            print_samples();
            i += pmu_num_counters();
        }
    }
}
