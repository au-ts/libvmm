
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <wfi.h>
#include <irq.h>
#include <timer.h>
#include <pmu.h>
#include <uart.h>

#define TX
#include "../../../comm.h"

#define print(args...) \
    spin_lock(&print_lock); \
    printf(args); \
    spin_unlock(&print_lock);

#define STR(str) #str
#define XSTR(S) STR(S)

#define TIMER_INTERVAL (TIME_MS(10))
#define COL_SIZE        20   
#define SAMPLE_FORMAT   "%" XSTR(COL_SIZE) "d"
#define HEADER_FORMAT   "%" XSTR(COL_SIZE) "s"
volatile size_t sample_count;

#define L1_CACHE_SIZE   (32*1024)
#define CACHE_LINE_SIZE (64)
volatile uint8_t cache_l1[2][L1_CACHE_SIZE] __attribute__((aligned(L1_CACHE_SIZE)));

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

uint64_t time_samples[sizeof(sample_events)/sizeof(size_t)][NUM_SAMPLES];

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


void print_samples() {

    for(size_t i = 0; i < sample_events_size; i+= pmu_num_counters()) {
        printf("--------------------------------\n");
        printf(HEADER_FORMAT, "irqlat");
        pmu_print_header(i);
        printf("\n");
        for (size_t j = 0; j < NUM_SAMPLES; j++) {
            printf(SAMPLE_FORMAT, time_samples[i/pmu_num_counters()][j]);
            pmu_print_samples(i, j);
            printf("\n");
        }
    }
    
}



void shmem_benchmark_notification() {

    printf("tx: Inter-VM Notification Latency Benchmark\n");

    shmem_init();

    printf("Press 's' to start...\n");
    while(uart_getchar() != 's');
    
    for (size_t i = 0; i < NUM_WARMUPS; i++) {
        notification_ctrl->received = false;
        shmem_send_event();
        while(!notification_ctrl->received);
        timer_wait(TIME_MS(10));
    }

    size_t iter_count = 0;
    size_t events_count = 0;
    while (events_count < sample_events_size) {

        pmu_setup(events_count, sample_events_size - events_count);

        sample_count = 0;
        while (sample_count < NUM_SAMPLES) {

            notification_ctrl->received = false;
            pmu_reset();
            notification_ctrl->timer_start = timer_get();

            shmem_send_event();
            while(!notification_ctrl->received);

            time_samples[iter_count][sample_count] = timer_get() - notification_ctrl->timer_start;
            pmu_sample(events_count);

            sample_count++;

            timer_wait(TIME_MS(10));
        }

        events_count += pmu_num_counters();
        iter_count++;
    }

    print_samples();

    notification_ctrl->finished_tx = true;

    while(!notification_ctrl->finished_rx);
}

