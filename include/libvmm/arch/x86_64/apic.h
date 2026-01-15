/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sel4/sel4.h>
#include <libvmm/arch/x86_64/virq.h>
#include <libvmm/arch/x86_64/instruction.h>

#define TIMER_DRV_CH_FOR_LAPIC 11

#define LAPIC_NUM_ISR_IRR_TMR_32B 8
#define MIN_VECTOR 32
#define MAX_VECTOR (LAPIC_NUM_ISR_IRR_TMR_32B * 32)

struct lapic_regs {
    // @billn make a container struct?
    // Not a LAPIC register, just bookkeeping
    uint32_t native_scaled_tsc_when_timer_starts;
    uint8_t last_injected_vector;

    uint32_t id;
    uint32_t revision;
    uint32_t svr;
    uint32_t tpr;
    uint32_t apr;
    uint32_t ppr;
    uint32_t dfr;
    uint32_t ldr;
    // These two are actually 256-bit register
    uint32_t isr[LAPIC_NUM_ISR_IRR_TMR_32B];
    uint32_t irr[LAPIC_NUM_ISR_IRR_TMR_32B];
    uint32_t tmr[LAPIC_NUM_ISR_IRR_TMR_32B];
    uint32_t dcr;
    uint32_t init_count;
    uint32_t timer;
    uint32_t icr_low;
    uint32_t icr_high;
    uint32_t lint0;
    uint32_t lint1;
    // uint32_t esr;
};

#define IOAPIC_LAST_INDIRECT_INDEX 0x17
#define IOAPIC_NUM_PINS (IOAPIC_LAST_INDIRECT_INDEX + 1)

struct ioapic_regs {
    uint32_t selected_reg;

    uint32_t ioapicid;
    uint32_t ioapicver;
    uint32_t ioapicarb;
    uint64_t ioredtbl[IOAPIC_NUM_PINS];

    struct ioapic_virq_handle virq_passthrough_map[IOAPIC_NUM_PINS];
};

bool lapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification, memory_instruction_data_t decoded_mem_ins);
bool ioapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification, memory_instruction_data_t decoded_mem_ins);

bool inject_lapic_irq(size_t vcpu_id, uint8_t vector);
bool inject_ioapic_irq(int ioapic, int pin);

bool handle_lapic_timer_nftn(size_t vcpu_id);
void lapic_maintenance(void);
