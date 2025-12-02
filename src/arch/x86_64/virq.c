
/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/linux.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

extern struct lapic_regs lapic_regs;
extern struct ioapic_regs ioapic_regs;
extern uint64_t lapic_timer_hz;

bool virq_controller_init(uint64_t tsc_hz)
{
    LOG_VMM("initialising LAPIC\n");
    lapic_timer_hz = tsc_hz;

    lapic_regs.id = 0;
    // Figure 11-7. Local APIC Version Register
    // "For processors based on the Nehalem microarchitecture (which has 7 LVT entries) and onward, the value returned is 6."
    lapic_regs.revision = 0x10 | 6 << 16;
    // Figure 11-18. Task-Priority Register (TPR)
    lapic_regs.tpr = 0; // reset value
    // Figure 11-23. Spurious-Interrupt Vector Register (SVR)
    lapic_regs.svr = 0xff; // reset value
    // Interrupt Command Register
    lapic_regs.icr = 0;
    // LVT Timer Register
    lapic_regs.timer = 0x00010000; // reset value, masked.
    // "Specifies interrupt delivery when an interrupt is signaled at the LINT0 pin"
    // Figure 11-8. Local Vector Table (LVT)
    lapic_regs.lint0 = 0x10000; // reset value
    lapic_regs.lint1 = 0x10000; // reset value

    for (int i = 0; i < LAPIC_NUM_ISR_IRR_32B; i++) {
        lapic_regs.isr[i] = 0;
        lapic_regs.irr[i] = 0;
    }

    LOG_VMM("initialising I/O APIC\n");
    // https://pdos.csail.mit.edu/6.828/2016/readings/ia32/ioapic.pdf
    ioapic_regs.ioapicid = 0;
    // default value for the Intel 82093AA IOAPIC.
    // supports 0x17 indirection entries.
    ioapic_regs.ioapicver = 0x11 | (IOAPIC_LAST_INDIRECT_INDEX << 16);
    ioapic_regs.ioapicarb = 0;
    // Wire up all the IRQs
    for (int i = 0; i <= IOAPIC_LAST_INDIRECT_INDEX; i++) {
        // Set vector
        ioapic_regs.ioredtbl[i] = IOAPIC0_BASE_VECTOR + i;
        // fixed intr deivery, all zero
        // destination mode, physical mode to apic 0, all zero
        // delivery status, all zero
        // Interrupt Input Pin Polarity, high active, all zero
        // edge triggered, all zero
        // interrupt mask, mask all irq:
        ioapic_regs.ioredtbl[i] |= BIT(16);
    }

    return true;
}

bool virq_inject(int irq)
{
    return true;
}

bool virq_register(size_t vcpu_id, size_t irq, virq_ack_fn_t ack_fn, void *ack_data)
{
    return true;
}