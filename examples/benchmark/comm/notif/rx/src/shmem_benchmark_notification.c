#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <wfi.h>
#include <irq.h>
#include <timer.h>
#include <pmu.h>

#define RX
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

volatile bool local_received = false;
volatile size_t pmu_count = 0, iter_count = 0;

void shmem_benchmark_notification_handler(unsigned _id) {

    notification_ctrl->timer_end = timer_get();
    notification_ctrl->received = true;
    local_received = true;
    
    // time_samples[iter_count][sample_count] = 
    //     notification_ctrl->timer_end - notification_ctrl->timer_start;
    // pmu_sample(pmu_count);
    // pmu_reset();
    // sample_count++;
    // if (sample_count >= NUM_SAMPLES) {
    //     sample_count = 0;
    //     iter_count++;
    //     pmu_count += pmu_num_counters();
    //     pmu_setup(pmu_count, pmu_num_counters());
    // }
    asm("ic iallu");
    // extern void nop_sled();
    // nop_sled();
    shmem_clear_event();
}

void shmem_benchmark_notification() {   

    notification_ctrl->finished_rx = false;
    notification_ctrl->finished_tx = false;

    printf("rx: Inter-VM Notification Latency Benchmark\n");

    shmem_init();
    irq_set_handler(SHMEM_IRQ, shmem_benchmark_notification_handler);
    irq_set_prio(SHMEM_IRQ, IRQ_MAX_PRIO);
    irq_enable(SHMEM_IRQ);

    // sample_count = 0;
    // pmu_setup(0, pmu_num_counters());
    while (!notification_ctrl->finished_tx) {
        for (size_t i = 0; i < L1_CACHE_SIZE; i+= CACHE_LINE_SIZE) {
            cache_l1[0][i] = i;
        }
        // while(!local_received && !notification_ctrl->finished_tx);
        // local_received = false;
    }
    // print_samples();

    irq_disable(SHMEM_IRQ);
    notification_ctrl->finished_rx = true;
}
