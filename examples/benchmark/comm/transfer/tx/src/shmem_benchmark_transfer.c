
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
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

#define L1_CACHE_SIZE   (32*1024)
#define CACHE_LINE_SIZE (64)
volatile uint8_t cache_l1[2][L1_CACHE_SIZE] __attribute__((aligned(L1_CACHE_SIZE)));

const size_t sample_events[] = {
    // L1I_CACHE_REFILL ,
    // L1I_CACHE_REFILL | EL2_ONLY,
    L1D_CACHE_REFILL,
    L1D_CACHE_REFILL | EL2_ONLY,
    L2D_CACHE_REFILL,
    L2D_CACHE_REFILL | EL2_ONLY,
    L1I_TLB_REFILL ,
    L1I_TLB_REFILL | EL2_ONLY,
    L1D_TLB_REFILL,
    L1D_TLB_REFILL | EL2_ONLY,
    // MEM_ACCESS,
    // MEM_ACCESS | EL2_ONLY,
    // BUS_ACCESS,
    // BUS_ACCESS | EL2_ONLY,
    // EXC_TAKEN,
    // EXC_TAKEN | EL2_ONLY,
    // EXC_IRQ,
    // EXC_IRQ | EL2_ONLY,
    // INST_RETIRED,
    // INST_RETIRED | EL2_ONLY,
};

const size_t sample_events_size = sizeof(sample_events)/sizeof(size_t);
unsigned long pmu_samples[NUM_BUFFER_SIZES][sizeof(sample_events)/sizeof(size_t)][NUM_SAMPLES];
unsigned long cycle_samples[NUM_BUFFER_SIZES][sizeof(sample_events)/sizeof(size_t)][NUM_SAMPLES];
volatile size_t pmu_used_counters = 0;

void pmu_setup_counters(size_t n, const size_t events[]){
    pmu_used_counters = n < pmu_num_counters()? n : pmu_num_counters();
    for(size_t i = 0; i < pmu_used_counters; i++){
        pmu_counter_set_event(i, events[i]);
        pmu_counter_enable(i);
    }
    pmu_cycle_enable(true);
}

void pmu_sample(size_t start, size_t sample_count, size_t transfer) {
    size_t n = pmu_used_counters;
    for(int i = 0; i < pmu_used_counters; i++){
        pmu_samples[transfer][start+i][sample_count] = pmu_counter_get(i);
    }
     cycle_samples[transfer][start/pmu_num_counters()][sample_count] = pmu_cycle_get();
}

void pmu_setup(size_t start, size_t n) {
    pmu_setup_counters(n, &sample_events[start]);
    pmu_reset();
    pmu_start();
}

static char* pmu_get_event_name(size_t start, size_t i) {
    uint32_t event = sample_events[start + i];
    char const * descr =  pmu_event_descr[event & 0xffff]; 
    descr = descr ? descr : "";
    uint32_t priv_code = (event >> 24) & 0xc8;
    const char * priv = priv_code == 0xc8 ? "_el2" : 
                        priv_code == 0x08 ? "_el1+2" :
                        "_el1";
    static char buf[COL_SIZE];
    snprintf(buf, COL_SIZE-1, "%s%s", descr, priv);
    return buf;
}

static inline void print_pmu_counters(size_t start, size_t n) {
    for(size_t i = 0; i < NUM_TRANSFER_SIZES; i++) {
        printf(HEADER_FORMAT, transfer_names[i]);
    }
    printf("\n");
    for (size_t j = 0; j < NUM_SAMPLES; j++) {
        for(size_t i = 0; i < NUM_TRANSFER_SIZES; i++) {
            printf(SAMPLE_FORMAT, pmu_samples[i][start+n][j]);
        }
        printf("\n");
    }
}

uint64_t transfer_times[NUM_BUFFER_SIZES][NUM_SAMPLES];

void print_transfer_times() {

    for(size_t i = 0; i < NUM_BUFFER_SIZES; i++) {
        printf(HEADER_FORMAT, buffer_names[i]);
    }
    printf("\n");
    for (size_t j = 0; j < NUM_SAMPLES; j++) {
        for(size_t i = 0; i < NUM_BUFFER_SIZES; i++) {
            printf(SAMPLE_FORMAT, transfer_times[i][j]);
        }
        printf("\n");
    }
    
}

void transfer(bool polling) {

    srand((unsigned) timer_get());
    unsigned long val = rand();

#ifdef SAMPLE_PMU_METRICS
    size_t counter_start = 0;
    while (counter_start < sample_events_size) {
#endif
    size_t sample_count = 0;
    for(size_t buf_size_index = 0; buf_size_index < NUM_BUFFER_SIZES; buf_size_index++) {

        const size_t buf_size = buffer_sizes[buf_size_index];
        const size_t transfer_size = 16 * 1024 * 1024;
        transfer_ctrl->buf_size = buf_size;
        transfer_ctrl->transfer_size = transfer_size;
        if (polling) transfer_ctrl->start_polling = true;

        size_t multiple_cycles = transfer_size > buf_size;
        size_t num_cycles = multiple_cycles ? transfer_size/buf_size : 1;
        size_t cycle_length = multiple_cycles ? buf_size : transfer_size;

        for (size_t i = 0; i < NUM_WARMUPS; i++) {
            for (size_t i = 0; i < num_cycles; i++) {
                volatile unsigned long *ptr = SHMEM_DATA_BASE;
                for (size_t j = 0; j < cycle_length/sizeof(val); j++) {
                    *ptr++ = val;
                }
                transfer_ctrl->received = false;
                if (polling) transfer_ctrl->sent = true;
                else shmem_send_event();
                while(!transfer_ctrl->received);
            }
        }

        for (sample_count = 0; sample_count < NUM_SAMPLES; sample_count++) {
#ifdef SAMPLE_PMU_METRICS
            pmu_setup(counter_start, sample_events_size - counter_start);
#endif

            uint64_t time_start = timer_get();
            for (size_t i = 0; i < num_cycles; i++) {
                volatile unsigned long *ptr = SHMEM_DATA_BASE;
                for (size_t j = 0; j < cycle_length/sizeof(val); j++) {
                    *ptr++ = val;
                }
                transfer_ctrl->received = false;
                if (polling) transfer_ctrl->sent = true;
                else shmem_send_event();
                while(!transfer_ctrl->received);
            }

            uint64_t total_time = timer_get() - time_start;
            transfer_times[buf_size_index][sample_count] = total_time;
#ifdef SAMPLE_PMU_METRICS
            pmu_sample(counter_start, sample_count, transfer_size_index);
#endif
        }
    }
        
        char *mode = polling ? "polling" : "interrupt";
        printf("<- %s, %s, time\n", mode, "16M");
        print_transfer_times();
        printf("->\n");

#ifdef SAMPLE_PMU_METRICS
        size_t left_counters = sample_events_size - counter_start;
        size_t counter_num = left_counters < pmu_num_counters() ? left_counters : pmu_num_counters();
        for (size_t i = 0; i < counter_num; i++) {
            char *event = pmu_get_event_name(counter_start, i);
            printf("<- %s, %s, %s\n", mode, buffer_names[buf_size_index], event);
            print_pmu_counters(counter_start, i);
            printf("->\n");
        }

        counter_start += pmu_num_counters();

    }
#endif
}


void shmem_benchmark_transfer() {

    printf("tx: Inter-VM Transfer Benchmark\n");

    transfer_ctrl->start_polling = false;
    transfer_ctrl->sent = false;
    notification_ctrl->finished_tx = false;

    volatile void *ptr = SHMEM_DATA_BASE;
    for (; ptr < (SHMEM_DATA_BASE+SHMEM_DATA_SIZE); ptr += 0x1000) {
        *((volatile int*)ptr) = 0; 
    }

    shmem_init();

    while (true) {
        printf("Press 's' to start...\n");
        while(uart_getchar() != 's');
        transfer(true);
        transfer(false);
    }

}
