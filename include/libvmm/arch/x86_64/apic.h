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

// Table 11-1. Local APIC Register Address Map
#define REG_LAPIC_ID 0x20
#define REG_LAPIC_REV 0x30
#define REG_LAPIC_TPR 0x80
#define REG_LAPIC_APR 0x90
#define REG_LAPIC_PPR 0xa0
#define REG_LAPIC_EOI 0xb0
#define REG_LAPIC_LDR 0xd0
#define REG_LAPIC_DFR 0xe0
#define REG_LAPIC_SVR 0xf0
#define REG_LAPIC_ISR_0 0x100
#define REG_LAPIC_ISR_1 0x110
#define REG_LAPIC_ISR_2 0x120
#define REG_LAPIC_ISR_3 0x130
#define REG_LAPIC_ISR_4 0x140
#define REG_LAPIC_ISR_5 0x150
#define REG_LAPIC_ISR_6 0x160
#define REG_LAPIC_ISR_7 0x170
#define REG_LAPIC_TMR_0 0x180
#define REG_LAPIC_TMR_1 0x190
#define REG_LAPIC_TMR_2 0x1A0
#define REG_LAPIC_TMR_3 0x1B0
#define REG_LAPIC_TMR_4 0x1C0
#define REG_LAPIC_TMR_5 0x1D0
#define REG_LAPIC_TMR_6 0x1E0
#define REG_LAPIC_TMR_7 0x1F0
#define REG_LAPIC_IRR_0 0x200
#define REG_LAPIC_IRR_1 0x210
#define REG_LAPIC_IRR_2 0x220
#define REG_LAPIC_IRR_3 0x230
#define REG_LAPIC_IRR_4 0x240
#define REG_LAPIC_IRR_5 0x250
#define REG_LAPIC_IRR_6 0x260
#define REG_LAPIC_IRR_7 0x270
#define REG_LAPIC_ESR 0x280
#define REG_LAPIC_CMCI 0x2f0
#define REG_LAPIC_ICR_LOW 0x300
#define REG_LAPIC_ICR_HIGH 0x310
#define REG_LAPIC_TIMER 0x320
#define REG_LAPIC_THERMAL 0x330
#define REG_LAPIC_PERF_MON_CNTER 0x340
#define REG_LAPIC_LVT_LINT0 0x350
#define REG_LAPIC_LVT_LINT1 0x360
#define REG_LAPIC_LVT_ERR 0x370
#define REG_LAPIC_INIT_CNT 0x380
#define REG_LAPIC_CURR_CNT 0x390
#define REG_LAPIC_DCR 0x3e0

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

uint32_t vapic_read_reg(int offset);
void vapic_write_reg(int offset, uint32_t value);

bool lapic_read_fault_handle(uint64_t offset);
bool lapic_write_fault_handle(uint64_t offset);
bool ioapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification, decoded_instruction_ret_t decoded_ins);

bool inject_lapic_irq(size_t vcpu_id, uint8_t vector);
bool inject_ioapic_irq(int ioapic, int pin);
bool ioapic_ack_passthrough_irq(uint8_t vector);

bool handle_lapic_timer_nftn(size_t vcpu_id);
