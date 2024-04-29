/*
 * Copyright 2022, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <microkit.h>
#include "sel4bench.h"
#include "bench.h"
#include "bench_fence.h"
#include "util/util.h"
// #include "bench_util.h"
#include <sel4/benchmark_track_types.h>
#include <sel4/benchmark_utilisation_types.h>

#define MAGIC_CYCLES 150
#define ULONG_MAX 0xfffffffffffffffful
#define UINT_MAX 0xfffffffful

#define LOG_BUFFER_CAP 7

#define START 1
#define STOP 2

#define INIT 3

#define PD_VMM_ID 1

uintptr_t uart_base;
uintptr_t cyclecounters_vaddr;

ccnt_t before, middle, after;

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

event_id_t benchmarking_events[] = {
    SEL4BENCH_EVENT_CACHE_L1I_MISS,
    SEL4BENCH_EVENT_CACHE_L1D_MISS,
    SEL4BENCH_EVENT_TLB_L1I_MISS,
    SEL4BENCH_EVENT_TLB_L1D_MISS,
    SEL4BENCH_EVENT_EXECUTE_INSTRUCTION,
    SEL4BENCH_EVENT_BRANCH_MISPREDICT,
};

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
static void
microkit_benchmark_start(void)
{
    // printf("Reseting thread utilisation\n");
    // seL4_BenchmarkDumpAllThreadsUtilisation();

    seL4_BenchmarkResetThreadUtilisation(TCB_CAP);
    // printf("------------------------ BASE THREAD RESET ------------------------\n");
    // seL4_BenchmarkDumpAllThreadsUtilisation();
    seL4_BenchmarkResetThreadUtilisation(BASE_TCB_CAP + PD_VMM_ID);
    // printf("------------------------ VMM THREAD RESET ------------------------\n");
    // seL4_BenchmarkDumpAllThreadsUtilisation();
    seL4_BenchmarkResetLog();
    // printf("err: %d\n", err);
    // printf("------------------------ LOG RESET ------------------------\n");
    // seL4_BenchmarkDumpAllThreadsUtilisation();
    // seL4_BenchmarkResetAllThreadsUtilisation();
    // printf("------------------------ ALL THREADS RESET ------------------------\n");
    // seL4_BenchmarkDumpAllThreadsUtilisation();

}

static void
microkit_benchmark_stop(uint64_t *total, uint64_t *idle, uint64_t *kernel, uint64_t *entries)
{
    seL4_BenchmarkFinalizeLog();
    // seL4_BenchmarkDumpAllThreadsUtilisation();

    seL4_BenchmarkGetThreadUtilisation(TCB_CAP);
    uint64_t *buffer = (uint64_t *)&seL4_GetIPCBuffer()->msg[0];

    *total = buffer[BENCHMARK_TOTAL_UTILISATION];
    *idle = buffer[BENCHMARK_IDLE_LOCALCPU_UTILISATION];
    *kernel = buffer[BENCHMARK_TOTAL_KERNEL_UTILISATION];
    *entries = buffer[BENCHMARK_TOTAL_NUMBER_KERNEL_ENTRIES];
}

static void
microkit_benchmark_stop_tcb(uint64_t pd_id, uint64_t *total, uint64_t *number_schedules, uint64_t *kernel, uint64_t *entries)
{
    seL4_BenchmarkGetThreadUtilisation(BASE_TCB_CAP + pd_id);
    uint64_t *buffer = (uint64_t *)&seL4_GetIPCBuffer()->msg[0];

    *total = buffer[BENCHMARK_TCB_UTILISATION];
    *number_schedules = buffer[BENCHMARK_TCB_NUMBER_SCHEDULES];
    *kernel = buffer[BENCHMARK_TCB_KERNEL_UTILISATION];
    *entries = buffer[BENCHMARK_TCB_NUMBER_KERNEL_ENTRIES];
}

static void
print_benchmark_details(uint64_t pd_id, uint64_t kernel_util, uint64_t kernel_entries, uint64_t number_schedules, uint64_t total_util)
{
    printf("Utilisation details for PD: ");
    switch (pd_id)
    {
    case PD_VMM_ID:
        printf("VMM");
        break;
    }
    printf(" (");
    puthex64(pd_id);
    printf(")\n");
    printf("KernelUtilisation");
    printf(": ");
    puthex64(kernel_util);
    printf("\n");
    printf("KernelEntries");
    printf(": ");
    puthex64(kernel_entries);
    printf("\n");
    printf("NumberSchedules: ");
    puthex64(number_schedules);
    printf("\n");
    printf("TotalUtilisation: ");
    puthex64(total_util);
    printf("\n");
    printf("}\n");
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

    while (logBuffer[index].start_time != 0 && index < logSize)
    {
        if (logBuffer[index].entry.path == Entry_Syscall)
        {
            if (logBuffer[index].entry.is_fastpath)
            {
                fastpaths++;
            }
            syscall_entries++;
        }
        else if (logBuffer[index].entry.path == Entry_Interrupt)
        {
            interrupt_entries++;
        }
        else if (logBuffer[index].entry.path == Entry_UserLevelFault)
        {
            userlevelfault_entries++;
        }
        else if (logBuffer[index].entry.path == Entry_VMFault)
        {
            vmfault_entries++;
        }
        else if (logBuffer[index].entry.path == Entry_DebugFault)
        {
            debug_fault++;
        }
        else
        {
            other++;
        }
        index++;
    }

    printf("Number of system call invocations ");
    puthex64(syscall_entries);
    printf(" and fastpaths ")
        puthex64(fastpaths);
    printf("\n");
    printf("Number of interrupt invocations ");
    puthex64(interrupt_entries);
    printf("\n");
    printf("Number of user-level faults ");
    puthex64(userlevelfault_entries);
    printf("\n");
    printf("Number of VM faults ");
    puthex64(vmfault_entries);
    printf("\n");
    printf("Number of debug faults ");
    puthex64(debug_fault);
    printf("\n");
    printf("Number of others ");
    puthex64(other);
    printf("\n");
}
#endif

void notified(microkit_channel ch)
{
    switch (ch)
    {
    case START:
    // printf("Starting benchmark\n");
        sel4bench_reset_counters();
        THREAD_MEMORY_RELEASE();

        sel4bench_start_counters(benchmark_bf);
        middle = sel4bench_get_cycle_count();
#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
        microkit_benchmark_start();
#endif

#ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES
        seL4_BenchmarkResetLog();
#endif

        break;
    case STOP:
        printf("Stopping benchmark\n");
        after = sel4bench_get_cycle_count();
        sel4bench_get_counters(benchmark_bf, &counter_values[0]);
        sel4bench_stop_counters(benchmark_bf);
        // Dump the counters
        printf("{\n");
        for (int i = 0; i < ARRAY_SIZE(benchmarking_events); i++)
        {
            printf(counter_names[i]);
            printf(": ");
            puthex64(counter_values[i]);
            printf("\n");
        }

        printf("}\n");
        printf("Before %lx, Middle %lx, After %lx\n", before, middle, after);
        printf("Init to start time: %lx\nInit to stop time: %lx\n", middle - before, after - before);
        seL4_BenchmarkResetThreadUtilisation(TCB_CAP);

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
            uint64_t total;
        uint64_t kernel;
        uint64_t entries;
        uint64_t idle;
        uint64_t number_schedules;
        microkit_benchmark_stop(&total, &idle, &kernel, &entries);
        print_benchmark_details(TCB_CAP, kernel, entries, idle, total);

        microkit_benchmark_stop_tcb(PD_VMM_ID, &total, &number_schedules, &kernel, &entries);
        print_benchmark_details(PD_VMM_ID, kernel, entries, number_schedules, total);
#endif

#ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES
        entries = seL4_BenchmarkFinalizeLog();
        printf("KernelEntries");
        printf(": ");
        puthex64(entries);
        seL4_BenchmarkTrackDumpSummary(log_buffer, entries);
#endif

        break;
    default:
        printf("Bench thread notified on unexpected channel\n");
    }
}

void init(void)
{
    printf("Initialising benchmarking\n");
    sel4bench_init();
    seL4_Word n_counters = sel4bench_get_num_counters();

    counter_bitfield_t mask = 0;

    for (seL4_Word i = 0; i < n_counters; i++)
    {
        seL4_Word counter = i;
        if (counter >= ARRAY_SIZE(benchmarking_events))
        {
            break;
        }
        sel4bench_set_count_event(i, benchmarking_events[counter]);
        mask |= BIT(i);
    }

    sel4bench_reset_counters();
    sel4bench_start_counters(mask);

    benchmark_bf = mask;

    printf("Benchmark bit mask %x\n", benchmark_bf);
    before = sel4bench_get_cycle_count();

    /* Notify the idle thread that the sel4bench library is initialised. */
    microkit_notify(INIT);

#ifdef CONFIG_BENCHMARK_TRACK_KERNEL_ENTRIES
    int res_buf = seL4_BenchmarkSetLogBuffer(LOG_BUFFER_CAP);
    if (res_buf)
    {
        printf("Could not set log buffer");
        puthex64(res_buf);
    }
    else
    {
        printf("We set the log buffer\n");
    }
#endif
}