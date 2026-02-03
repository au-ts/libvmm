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

// Implementation of VMM side KVM pvclock. This was used during a short period of
// development where we were doing Hardware Reduced ACPI. But it is no longer used
// as we moved away from it. Leaving it here as it is useful.

extern uint64_t tsc_hz;

// Copied from Linux
/* Same "calling convention" as do_div:
 * - divide (n << 32) by base
 * - put result in n
 * - return remainder
 */
#define do_shl32_div32(n, base)					\
	({							\
	    uint32_t __quot, __rem;					\
	    asm("divl %2" : "=a" (__quot), "=d" (__rem)		\
			: "rm" (base), "0" (0), "1" ((uint32_t) n));	\
	    n = __quot;						\
	    __rem;						\
	 })

// Copied from Linux
static uint32_t div_frac(uint32_t dividend, uint32_t divisor)
{
    do_shl32_div32(dividend, divisor);
    return dividend;
}
// Copied from Linux
static void kvm_get_time_scale(uint64_t scaled_hz, uint64_t base_hz, int8_t *pshift, uint32_t *pmultiplier)
{
    uint64_t scaled64;
    int32_t shift = 0;
    uint64_t tps64;
    uint32_t tps32;

    tps64 = base_hz;
    scaled64 = scaled_hz;
    while (tps64 > scaled64 * 2 || tps64 & 0xffffffff00000000ULL) {
        tps64 >>= 1;
        shift--;
    }

    tps32 = (uint32_t)tps64;
    while (tps32 <= scaled64 || scaled64 & 0xffffffff00000000ULL) {
        if (scaled64 & 0xffffffff00000000ULL || tps32 & 0x80000000)
            scaled64 >>= 1;
        else
            tps32 <<= 1;
        shift++;
    }

    *pshift = shift;
    *pmultiplier = div_frac(scaled64, tps32);
}

static void update_system_time(struct pvclock_vcpu_time_info *system_time)
{
    assert(tsc_hz);

    system_time->version += 1;
    system_time->tsc_timestamp = rdtsc();
    system_time->system_time = sddf_timer_time_now(
        TIMER_DRV_CH_FOR_PIT); // @billn sus, make an independant channel for pvclock
    kvm_get_time_scale(NS_IN_S, tsc_hz, &system_time->tsc_shift, &system_time->tsc_to_system_mul);

    system_time->flags = BIT(0);
    system_time->version += 1;
}

bool pvclock_write_fault_handle(uint64_t msr_addr, uint64_t msr_value)
{
    if (msr_addr == MSR_KVM_SYSTEM_TIME_NEW) {
        struct pvclock_vcpu_time_info *system_time = (struct pvclock_vcpu_time_info *)gpa_to_vaddr((msr_value >> 1)
                                                                                                   << 1);
        update_system_time(system_time);
        return true;
    } else if (msr_addr == MSR_KVM_WALL_CLOCK_NEW) {
        struct pvclock_wall_clock *wall_time = (struct pvclock_wall_clock *)gpa_to_vaddr((msr_value >> 1) << 1);
        wall_time->version += 2;
        uint64_t time = sddf_timer_time_now(TIMER_DRV_CH_FOR_PIT);
        wall_time->nsec = time;
        wall_time->sec = time / NS_IN_S;
    } else {
        return false;
    }
}