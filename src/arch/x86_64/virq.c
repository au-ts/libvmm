
/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <sddf/util/custom_libc/string.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/virq.h>
#include <libvmm/arch/x86_64/linux.h>

struct ioapic_virq_handle {
    int ioapic;
    int pin;
    virq_ioapic_ack_fn_t ack_fn;
    void *ack_data;
};

/* Maps Microkit channel numbers with registered vIRQ */
int virq_passthrough_map[MAX_PASSTHROUGH_IRQ] = {-1};

struct lapic_regs lapic_regs;
struct ioapic_regs ioapic_regs;
uint64_t tsc_hz;

bool virq_controller_init(uint64_t native_tsc_hz)
{
    tsc_hz = native_tsc_hz;

    LOG_VMM("initialising IRQ book-keeping structures\n");
    memset(&lapic_regs, 0, sizeof(struct lapic_regs));
    memset(&ioapic_regs, 0, sizeof(struct ioapic_regs));

    LOG_VMM("initialising LAPIC\n");

    // Figure 11-7. Local APIC Version Register
    // "For processors based on the Nehalem microarchitecture (which has 7 LVT entries) and onward, the value returned is 6."
    lapic_regs.revision = 0x10 | 6 << 16;
    // Figure 11-23. Spurious-Interrupt Vector Register (SVR)
    lapic_regs.svr = 0xff; // reset value
    // LVT Timer Register
    lapic_regs.timer = 0x00010000; // reset value, masked.
    // "Specifies interrupt delivery when an interrupt is signaled at the LINT0 pin"
    // Figure 11-8. Local Vector Table (LVT)
    lapic_regs.lint0 = 0x10000; // reset value
    lapic_regs.lint1 = 0x10000; // reset value

    // Figure 11-14. Destination Format Register (DFR)
    lapic_regs.dfr = 0xffffffff; // reset value

    LOG_VMM("initialising I/O APIC\n");
    // https://pdos.csail.mit.edu/6.828/2016/readings/ia32/ioapic.pdf
    // default value for the Intel 82093AA IOAPIC.
    // supports 0x17 indirection entries.
    ioapic_regs.ioapicver = 0x11 | (IOAPIC_LAST_INDIRECT_INDEX << 16);
    // Wire up all the IRQs
    for (int i = 0; i <= IOAPIC_LAST_INDIRECT_INDEX; i++) {
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

bool virq_ioapic_register_passthrough(int ioapic, int pin, microkit_channel irq_ch) {

}

bool virq_ioapic_handle_passthrough(microkit_channel irq_ch) {

}
