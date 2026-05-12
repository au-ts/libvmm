/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/guest_time.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/msr.h>

#define LAPIC_REG_BLOCK_SIZE 0x1000

/* Per vCPU/VMM */
struct local_vmm_state {
    uint64_t tsc_hz;
    bool cpuid_validated;

    /* Intel APICv's "Virtual APIC" page */
    void *apicv_addr;

#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
    /* Emulated local APIC state */
    char lapic_regs[LAPIC_REG_BLOCK_SIZE];
#endif

    /* Bookkeeping for emulating the local APIC timer */
    uint64_t apic_timer_scaled_ticks_when_timer_starts;
    bool apic_timer_timeout_handle_valid;
    guest_timeout_handle_t apic_timer_timeout_handle;

    /* Guest timekeeping */
    virtual_timer_time_out_t guest_timer_timeouts[MAX_CONCURRENT_TIMEOUT];
    /* VMX-preemption timer only ticks after 2^vmx_timer_shift TSC ticks */
    uint8_t vmx_timer_shift;
    /* What is the timer state when it was last primed? */
    bool vmx_timer_primed;
    uint64_t tsc_at_vmx_timer_prime;
    uint64_t tsc_ticks_to_vmx_timeout;

    /* VCPU registers on fault */
    struct vcpu_fault_state vcpu_fault_state;

    uint32_t apic_base_msr_read_mask;

    /* MTRR virtualisation */
    struct mtrr_state mtrr_state;
};

/* State of the entire Virtual Machine. */
struct global_read_only_vmm_state {
    unsigned num_vcpus;
};

struct global_mutable_vmm_state {
    struct ioapic_regs ioapic_regs;

    /* EPT fault handling is global, as all VCPUs have uniform view of memory. We don't
     * have stuff like NUMA right now. */
    struct ept_exception_handler ept_exception_handlers[MAX_EPT_EXCEPTION_HANDLERS];
    size_t num_ept_exception_handlers;

    /* Same thing with PIO */
    struct pio_exception_handler pio_exception_handlers[MAX_PIO_EXCEPTION_HANDLERS];
    size_t num_pio_exception_handlers;
};

struct global_vmm_state {
    struct global_read_only_vmm_state read_only;

    // big libvmm lock goes here @billn
    struct global_mutable_vmm_state mutable;
};

bool initialise_local_and_global_vmm_state(bool is_bsp, void *global_vmm_state_ptr);

struct global_read_only_vmm_state *get_global_vmm_mutable_state(void);
struct global_mutable_vmm_state *acquire_global_vmm_mutable_state(void);
void release_global_vmm_mutable_state(void);