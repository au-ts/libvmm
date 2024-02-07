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

#define print(args...) \
    spin_lock(&print_lock); \
    printf(args); \
    spin_unlock(&print_lock);

#define XSTR(S) STR(S)

#define TIMER_INTERVAL (TIME_MS(10))
#define NUM_SAMPLES  (50)
#define NUM_WARMUPS  (10)
#define COL_SIZE        20   
#define SAMPLE_FORMAT   "%" XSTR(COL_SIZE) "d"
#define HEADER_FORMAT   "%" XSTR(COL_SIZE) "s"
volatile size_t sample_count;

#define L1_CACHE_SIZE   (32*1024)
#define CACHE_LINE_SIZE (64)
volatile uint8_t cache_l1[2][L1_CACHE_SIZE] __attribute__((aligned(L1_CACHE_SIZE)));

__attribute__((section(".data"))) volatile uint64_t boot_el2 = 0;

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
unsigned long cycle_samples[sizeof(sample_events)/sizeof(size_t)][NUM_SAMPLES];
unsigned long pmu_dst_samples[sizeof(sample_events)/sizeof(size_t)][NUM_SAMPLES];
unsigned long cycle_dst_samples[sizeof(sample_events)/sizeof(size_t)][NUM_SAMPLES];
volatile size_t pmu_used_counters = 0;

void pmu_setup_counters(size_t n, const size_t events[]){
    pmu_used_counters = n < pmu_num_counters()? n : pmu_num_counters();
    for(size_t i = 0; i < pmu_used_counters; i++){
        pmu_counter_set_event(i, events[i]);
        pmu_counter_enable(i);
    }
    pmu_cycle_enable(true);
}


void pmu_sample(size_t start) {
    size_t n = pmu_used_counters;
    for(int i = 0; i < pmu_used_counters; i++){
        pmu_samples[start+i][sample_count] = pmu_counter_get(i);
    }
     cycle_samples[start/pmu_num_counters()][sample_count] = pmu_cycle_get();
}

void pmu_dst_sample(size_t start) {
    size_t n = pmu_used_counters;
    for(int i = 0; i < pmu_used_counters; i++){
        pmu_dst_samples[start+i][sample_count] = pmu_counter_get(i);
    }
     cycle_dst_samples[start/pmu_num_counters()][sample_count] = pmu_cycle_get();
}

void pmu_setup(size_t start, size_t n) {
    pmu_setup_counters(n, &sample_events[start]);
    pmu_reset();
    pmu_start();
}

static inline void pmu_print_header(size_t start) {
    size_t left_counters = sample_events_size - start;
    size_t n = left_counters < pmu_num_counters() ? left_counters : pmu_num_counters();
    for (size_t i = 0; i < n; i++) {
        uint32_t event = sample_events[start + i];
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

static inline void pmu_print_samples(size_t start, size_t i) {
    size_t left_counters = sample_events_size - start;
    size_t n = left_counters < pmu_num_counters() ? left_counters : pmu_num_counters();
    for (size_t j = 0; j < n; j++) {
        printf(SAMPLE_FORMAT, pmu_samples[start+j][i]);
    }
    printf(SAMPLE_FORMAT, cycle_samples[start/pmu_num_counters()][i]);
}

static inline void pmu_print_dst_samples(size_t start, size_t i) {
    size_t left_counters = sample_events_size - start;
    size_t n = left_counters < pmu_num_counters() ? left_counters : pmu_num_counters();
    for (size_t j = 0; j < n; j++) {
        printf(SAMPLE_FORMAT, pmu_dst_samples[start+j][i]);
    }
    printf(SAMPLE_FORMAT, cycle_dst_samples[start/pmu_num_counters()][i]);
}


volatile uint64_t timer_prev, timer_end;

size_t sample_ow_count = 0, sample_rtt_count = 0;
volatile bool ipi_received;

spinlock_t cpu_set_lock = SPINLOCK_INITVAL, print_lock = SPINLOCK_INITVAL;
volatile unsigned cpu_src = -1, cpu_dst = -1;
volatile bool cpu_src_set = false, cpu_dst_set = false;
volatile bool master_done = false;
volatile bool measure_finished_dst, measure_finished_src;

uint64_t ipi_lat_samples[sizeof(sample_events)/sizeof(size_t)][NUM_SAMPLES];
uint64_t sgi_lat_time[sizeof(sample_events)/sizeof(size_t)][NUM_SAMPLES];
uint64_t ipi_lat_dst_samples[sizeof(sample_events)/sizeof(size_t)][NUM_SAMPLES];

void print_samples() {

    for(size_t i = 0; i < sample_events_size; i+= pmu_num_counters()) {
        printf("--------------------------------\n");
        printf(HEADER_FORMAT, "trap-lat");
        printf(HEADER_FORMAT, "sgi-lat");
        pmu_print_header(i);
        printf("\n");
        for (size_t j = 0; j < NUM_SAMPLES; j++) {
            printf(SAMPLE_FORMAT, ipi_lat_samples[i/pmu_num_counters()][j]);
            printf(SAMPLE_FORMAT, sgi_lat_time[i/pmu_num_counters()][j]);
            pmu_print_samples(i, j);
            printf("\n");
        }
    }
    
}

void print_dst_samples() {
    printf("----------------------------------------TARGET----------------------------------------------------\n");
    for(size_t i = 0; i < sample_events_size; i+= pmu_num_counters()) {
        printf("--------------------------------\n");
        printf(HEADER_FORMAT, "ipi-lat");
        pmu_print_header(i);
        printf("\n");
        for (size_t j = 0; j < NUM_SAMPLES; j++) {
            printf(SAMPLE_FORMAT, ipi_lat_dst_samples[i/pmu_num_counters()][j]);
            pmu_print_dst_samples(i, j);
            printf("\n");
        }
    }
    
}

size_t i;
size_t j;
volatile bool wait_for_dst_sample;
volatile bool wait_for_src_sample;
volatile bool back_waiting = false;

uint64_t sgi_time_get() {
    register uint64_t x0 asm("x0") = 43;
    register uint64_t x16 asm("x16") = 43;
    asm volatile("hvc #0x4a48" : "=r"(x0) : "r"(x0), "r"(x16));
    return x0;
}

void ipi_handler(unsigned id) {

    timer_end = timer_get();

    back_waiting = false;
    ipi_received = true;
    fence_sync_write();
    asm volatile("sev");

    pmu_dst_sample(i);

    ipi_lat_dst_samples[j][sample_count] = timer_end - timer_prev;

    if (sample_count + 1 >= NUM_SAMPLES) {
        size_t ii = i + pmu_num_counters();
        pmu_setup(ii, sample_events_size - ii);
    }

    pmu_reset();

    while(wait_for_src_sample);
    wait_for_src_sample =true;

    asm volatile("ic iallu\n\t");
    // asm volatile("tlbi aside1is, %0" :: "r"(0));
    wait_for_dst_sample = false;
}

void main(void){

    if (cpu_is_master()) {
        print("Bao baremetal ipi latency test\n");
        master_done = true;
    }

    while(!master_done);

    bool left_out = false;

    spin_lock(&cpu_set_lock);
    if (!cpu_src_set) {
        cpu_src_set = true;
        cpu_src = get_cpuid();
    } else if (!cpu_dst_set) {
        cpu_dst_set = true;
        cpu_dst = get_cpuid();
    } else {
        left_out = true;
    }
    spin_unlock(&cpu_set_lock);

    if (left_out) {
        return;
    }

    while(!(cpu_src_set && cpu_dst_set));

    uint64_t init_time = timer_get();
    print("time: %d\n", init_time);

    if(cpu_dst == get_cpuid()) {
        pmu_setup(0, sample_events_size );
        irq_set_handler(IPI_IRQ_ID, ipi_handler);
        irq_enable(IPI_IRQ_ID);
        irq_set_prio(IPI_IRQ_ID, IRQ_MAX_PRIO);
        while(1) { 
            for (size_t i = 0; i < L1_CACHE_SIZE; i+= CACHE_LINE_SIZE) {
                cache_l1[0][i] = i;
            }
            back_waiting = true;
            wfi(); 
        }
    }

    bool warming_up;
    while(1) {
        print("Press 's' to start...\n");
        while(uart_getchar() != 's');

        i = 0;
        j = 0;
        wait_for_dst_sample = true;
        wait_for_src_sample = true;
        warming_up = true;
        while(i < sample_events_size) {

            pmu_setup(i, sample_events_size - i);

            sample_count = 0;
            while (sample_count < NUM_SAMPLES) {

                ipi_received = false;
                while(!back_waiting);

                pmu_reset();
                timer_prev = timer_get();

                gic_send_sgi(cpu_dst, IPI_IRQ_ID);

                if(!warming_up) {
                    uint64_t timer_end_local = timer_get();
                    pmu_sample(i);
                    if (boot_el2) {
                        sgi_lat_time[j][sample_count] = 0;
                    } else {
                        sgi_lat_time[j][sample_count] = sgi_time_get() - timer_prev;
                    }
                    ipi_lat_samples[j][sample_count] = timer_end_local - timer_prev;
                }

                while(!ipi_received) {
                    asm volatile("wfe");
                }                

                asm volatile("ic iallu\n\t");
                for (size_t i = 0; i < L1_CACHE_SIZE; i+= CACHE_LINE_SIZE) {
                    cache_l1[1][i] = i;
                }

                wait_for_src_sample = false;
                while(wait_for_dst_sample);
                wait_for_dst_sample = true;

                sample_count += 1;

                if(warming_up && sample_count >= NUM_WARMUPS) {
                    warming_up = false;
                    sample_count = 0;
                }
                
                timer_wait(TIME_MS(10));
            }
            j += 1;
            i += pmu_num_counters();
        }
        pmu_stop();
        print_samples();
        print_dst_samples();
    }

    /**
     * Another version of the benchmark where why try to measure RTT latency
     * by polling the IPI pending bit. However this is not the best idea as
     * each pending read results in a trap to the hypervisor and is not the 
     * actual path the interrupt normally takes.
     */

    /**
     * We can't disable SGIs in gic-400 so we set the priority to the minimum
     * so that the interrupt is always masked.
     */
    // irq_set_prio(IPI_IRQ_ID, IRQ_MIN_PRIO);

    // if(cpu_dst == get_cpuid()) {
        // sample_ow_count = 0;
        // while(sample_ow_count < NUM_SAMPLES) {
        //     while (!irq_get_pend(IPI_IRQ_ID));
        //     // print("dst received interrupt\n");
        //     ipi_lat_samples[sample_ow_count++] = timer_get() - timer_prev;
        //     irq_send_ipi(1ull << cpu_src);
        //     irq_clear_pend(IPI_IRQ_ID);
        //     measure_finished_dst = true;
        //     fence_sync();
        //     while(!measure_finished_src);
        // }
    // }

    // sample_rtt_count = 0;
    // while (sample_rtt_count < NUM_SAMPLES) {
        // measure_finished_src = false;
        // measure_finished_dst = false;
        // timer_prev = timer_get();
        // irq_send_ipi(1ull << cpu_dst);
        // while(!irq_get_pend(IPI_IRQ_ID));
        // // print("src received interrupt\n");
        // ipi_rtt_lat_samples[sample_rtt_count++] = timer_get() - timer_prev;
        // irq_clear_pend(IPI_IRQ_ID);
        // measure_finished_src = true;
        // fence_sync();
        // while(!measure_finished_dst);
    // }
    // for (size_t i = 0; i < NUM_SAMPLES; i++) {
    //     printf("%d, %d\n", ipi_lat_samples[i], ipi_rtt_lat_samples[i]);
    // }
}
