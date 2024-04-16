/*
 * Copyright 2022, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <microkit.h>
#include <sel4/benchmark_track_types.h>
#include <sel4/benchmark_utilisation_types.h>
#include <sddf/benchmark/bench.h>
#include <sddf/benchmark/sel4bench.h>
#include <sddf/util/fence.h>
#include <sddf/util/util.h>
#include <sddf/util/printf.h>

#define CONFIG_BENCHMARK_TRACK_UTILISATION
#define LOG_BUFFER_CAP 7

/* Notification channels and TCB CAP offsets - ensure these align with .system file! */
#define START 1
#define STOP 2
#define INIT 3

#define PD_TOTAL        0
#define PD_CLIENT_ID    1
#define PD_VIRT_ID      2
#define PD_SND_DRV_ID   3

uintptr_t uart_base;
uintptr_t cyclecounters_vaddr;

ccnt_t counter_values[8];
counter_bitfield_t benchmark_bf;

#ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES
benchmark_track_kernel_entry_t *log_buffer;
#endif

char *counter_names[] = {
    "L1 i-cache misses",
    "L1 d-cache misses",
    "L1 i-tlb misses",
    "L1 d-tlb misses",
    "Instructions",
    "Branch mispredictions",
};

#define PD_ID_COUNT 3
int pd_ids[PD_ID_COUNT] = {
    PD_CLIENT_ID,
    PD_VIRT_ID,
    PD_SND_DRV_ID
};

event_id_t benchmarking_events[] = {
    SEL4BENCH_EVENT_CACHE_L1I_MISS,
    SEL4BENCH_EVENT_CACHE_L1D_MISS,
    SEL4BENCH_EVENT_TLB_L1I_MISS,
    SEL4BENCH_EVENT_TLB_L1D_MISS,
    SEL4BENCH_EVENT_EXECUTE_INSTRUCTION,
    SEL4BENCH_EVENT_BRANCH_MISPREDICT,
};

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
static void microkit_benchmark_start(void)
{
    seL4_BenchmarkResetThreadUtilisation(TCB_CAP);
    seL4_BenchmarkResetThreadUtilisation(BASE_TCB_CAP + PD_CLIENT_ID);
    seL4_BenchmarkResetThreadUtilisation(BASE_TCB_CAP + PD_VIRT_ID);
    seL4_BenchmarkResetThreadUtilisation(BASE_TCB_CAP + PD_SND_DRV_ID);
    seL4_BenchmarkResetLog();
}

static void microkit_benchmark_stop(uint64_t *total, uint64_t* number_schedules, uint64_t *kernel, uint64_t *entries)
{
    seL4_BenchmarkFinalizeLog();
    seL4_BenchmarkGetThreadUtilisation(TCB_CAP);
    uint64_t *buffer = (uint64_t *)&seL4_GetIPCBuffer()->msg[0];

    *total = buffer[BENCHMARK_TOTAL_UTILISATION];
    *number_schedules = buffer[BENCHMARK_TOTAL_NUMBER_SCHEDULES];
    *kernel = buffer[BENCHMARK_TOTAL_KERNEL_UTILISATION];
    *entries = buffer[BENCHMARK_TOTAL_NUMBER_KERNEL_ENTRIES];
}

static void microkit_benchmark_stop_tcb(uint64_t pd_id, uint64_t *total, uint64_t *number_schedules, uint64_t *kernel, uint64_t *entries)
{
    seL4_BenchmarkGetThreadUtilisation(BASE_TCB_CAP + pd_id);
    uint64_t *buffer = (uint64_t *)&seL4_GetIPCBuffer()->msg[0];

    *total = buffer[BENCHMARK_TCB_UTILISATION];
    *number_schedules = buffer[BENCHMARK_TCB_NUMBER_SCHEDULES];
    *kernel = buffer[BENCHMARK_TCB_KERNEL_UTILISATION];
    *entries = buffer[BENCHMARK_TCB_NUMBER_KERNEL_ENTRIES];
}

static void print_benchmark_details(uint64_t pd_id, uint64_t kernel_util, uint64_t kernel_entries, uint64_t number_schedules, uint64_t total_util)
{
    if (pd_id == PD_TOTAL) sddf_printf("Total utilisation details: ");
    else sddf_printf("Utilisation details for PD: ");
    switch (pd_id) {
        case PD_CLIENT_ID: sddf_printf("PD_CLIENT_ID"); break;
        case PD_VIRT_ID: sddf_printf("PD_VIRT_ID"); break;
        case PD_SND_DRV_ID: sddf_printf("PD_SND_DRV_ID"); break;
    }
    if (pd_id != PD_TOTAL) sddf_printf(" ( %llx)", pd_id);
    sddf_printf("\n{\nKernelUtilisation:  %llx\nKernelEntries:  %llx\nNumberSchedules:  %llx\nTotalUtilisation:  %llx\n}\n", 
            kernel_util, kernel_entries, number_schedules, total_util);
}
#endif

#ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES
static inline void seL4_BenchmarkTrackDumpSummary(benchmark_track_kernel_entry_t *logBuffer, uint64_t logSize)
{
    seL4_Word index = 0;
    seL4_Word syscall_entries = 0;
    seL4_Word fastpaths = 0;
    seL4_Word interrupt_entries = 0;
    seL4_Word userlevelfault_entries = 0;
    seL4_Word vmfault_entries = 0;
    seL4_Word debug_fault = 0;
    seL4_Word other = 0;

    while (logBuffer[index].start_time != 0 && index < logSize) {
        if (logBuffer[index].entry.path == Entry_Syscall) {
            if (logBuffer[index].entry.is_fastpath) fastpaths++;
            syscall_entries++;
        } else if (logBuffer[index].entry.path == Entry_Interrupt) interrupt_entries++;
        else if (logBuffer[index].entry.path == Entry_UserLevelFault) userlevelfault_entries++;
        else if (logBuffer[index].entry.path == Entry_VMFault) vmfault_entries++;
        else if (logBuffer[index].entry.path == Entry_DebugFault) debug_fault++;
        else other++;

        index++;
    }

    sddf_printf("Number of system call invocations  %llx and fastpaths  %llx\n", syscall_entries, fastpaths);
    sddf_printf("Number of interrupt invocations  %llx\n", interrupt_entries);
    sddf_printf("Number of user-level faults  %llx\n", userlevelfault_entries);
    sddf_printf("Number of VM faults  %llx\n", vmfault_entries);
    sddf_printf("Number of debug faults  %llx\n", debug_fault);
    sddf_printf("Number of others  %llx\n", other);
}
#endif


void notified(microkit_channel ch)
{
    sddf_printf("BENCH NOTIFIED\n");
    switch(ch) {
        case START:
            sddf_printf("Starting\n");
            sel4bench_reset_counters();
            sddf_printf("Reset counters\n");
            THREAD_MEMORY_RELEASE();
            sel4bench_start_counters(benchmark_bf);
            sddf_printf("Started counters\n");

            #ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
            microkit_benchmark_start();
            sddf_printf("Tracking utilisation\n");
            #endif

            #ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES
            seL4_BenchmarkResetLog();
            sddf_printf("Resetting log\n");
            #endif

            break;
        case STOP:
            sddf_printf("Stopping\n");
            sel4bench_get_counters(benchmark_bf, &counter_values[0]);
            sel4bench_stop_counters(benchmark_bf);

            sddf_printf("{\n");
            for (int i = 0; i < ARRAY_SIZE(benchmarking_events); i++) {
                sddf_printf("%s: %llX\n", counter_names[i], counter_values[i]);
            }
            sddf_printf("}\n");

            #ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
            uint64_t total;
            uint64_t kernel;
            uint64_t entries;
            uint64_t number_schedules;

            microkit_benchmark_stop(&total, &number_schedules, &kernel, &entries);
            print_benchmark_details(PD_TOTAL, kernel, entries, number_schedules, total);

            for (int i = 0; i < PD_ID_COUNT; i++) {
                microkit_benchmark_stop_tcb(pd_ids[i], &total, &number_schedules, &kernel, &entries);
                print_benchmark_details(pd_ids[i], kernel, entries, number_schedules, total);
            }

            #endif

            #ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES
            entries = seL4_BenchmarkFinalizeLog();
            sddf_printf("KernelEntries:  %llx\n", entries);
            seL4_BenchmarkTrackDumpSummary(log_buffer, entries);
            #endif

            break;
        default:
            sddf_printf("Bench thread notified on unexpected channel\n");
    }
    sddf_printf("Finish notify\n");
}

void init(void)
{
    sddf_printf("INIT BENCH\n");
    sel4bench_init();
    seL4_Word n_counters = sel4bench_get_num_counters();

    counter_bitfield_t mask = 0;
    
    for (seL4_Word counter = 0; counter < n_counters; counter++) {
        if (counter >= ARRAY_SIZE(benchmarking_events)) break;
        sel4bench_set_count_event(counter, benchmarking_events[counter]);
        mask |= BIT(counter);
    }

    sel4bench_reset_counters();
    sel4bench_start_counters(mask);
    benchmark_bf = mask;

    /* Notify the idle thread that the sel4bench library is initialised. */
    microkit_notify(INIT);

#ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES
    int res_buf = seL4_BenchmarkSetLogBuffer(LOG_BUFFER_CAP);
    if (res_buf) sddf_printf("Could not set log buffer:  %llx\n", res_buf);
    else sddf_printf("Log buffer set\n");
#endif
}