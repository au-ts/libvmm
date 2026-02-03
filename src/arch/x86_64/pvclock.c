/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/pvclock.h>
#include <libvmm/arch/x86_64/pit.h>

extern uint64_t tsc_hz;

static void update_system_time(struct pvclock_vcpu_time_info *system_time)
{
    assert(tsc_hz);

    system_time->version += 1;
    system_time->tsc_timestamp = rdtsc();
    system_time->system_time = sddf_timer_time_now(
        TIMER_DRV_CH_FOR_PIT); // @billn sus, make an independant channel for pvclock
    system_time->tsc_to_system_mul = tsc_hz / NS_IN_S;
    system_time->tsc_shift = -32; // @billn todo double check;
    system_time->flags = 0;
    system_time->version += 1;


    LOG_VMM("tsc = %llu, sys time = %llu\n", system_time->tsc_timestamp, system_time->system_time);
}

bool pvclock_write_fault_handle(uint64_t msr_addr, uint64_t msr_value)
{
    if (msr_addr == MSR_KVM_SYSTEM_TIME_NEW) {
        struct pvclock_vcpu_time_info *system_time = (struct pvclock_vcpu_time_info *)gpa_to_vaddr(msr_value);
        update_system_time(system_time);
        return true;
    } else {
        return false;
    }
}