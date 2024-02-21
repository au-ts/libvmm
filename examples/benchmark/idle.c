/*
 * Copyright 2022, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <microkit.h>
#include "sel4bench.h"
#include "bench_fence.h"
#include "bench.h"
#include "util/util.h"

#define INIT 3
#define MAGIC_CYCLES 150
#define ULONG_MAX 0xfffffffffffffffful

uintptr_t cyclecounters_vaddr;

struct bench *b = (void *)(uintptr_t)0x5010000;

void count_idle(void)
{
    b->prev = sel4bench_get_cycle_count();
    b->ccount = 0;
    b->overflows = 0;

    printf("Idle thread started\n");

    while (1)
    {

        b->ts = (uint64_t)sel4bench_get_cycle_count();
        uint64_t diff;

        /* Handle overflow: This thread needs to run at least 2 times
           within any ULONG_MAX cycles period to detect overflows */
        if (b->ts < b->prev)
        {
            diff = ULONG_MAX - b->prev + b->ts + 1;
            b->overflows++;
        }
        else
        {
            diff = b->ts - b->prev;
        }

        if (diff < MAGIC_CYCLES)
        {
            COMPILER_MEMORY_FENCE();

            b->ccount += diff;
            COMPILER_MEMORY_FENCE();
        }

        b->prev = b->ts;
    }
}

void notified(microkit_channel ch)
{
    switch (ch)
    {
    case INIT:
        // init is complete so we can start counting.
        count_idle();
        break;
    default:
        microkit_dbg_puts("Idle thread notified on unexpected channel\n");
    }
}

void init(void)
{
    /* Nothing to set up as benchmark.c initialises the sel4bench library for us. */
    return;
}