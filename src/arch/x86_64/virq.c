
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

/* Maps Microkit channel numbers with registered I/O APIC IRQs */
struct ioapic_virq_handle virq_passthrough_map[MAX_PASSTHROUGH_IRQ];

struct lapic_regs lapic_regs;
struct ioapic_regs ioapic_regs;
uint64_t tsc_hz;

bool virq_controller_init(uint64_t native_tsc_hz)
{
    tsc_hz = native_tsc_hz;

    LOG_VMM("initialising IRQ book-keeping structures\n");
    memset(&lapic_regs, 0, sizeof(struct lapic_regs));
    memset(&ioapic_regs, 0, sizeof(struct ioapic_regs));
    for (int i = 0; i < MAX_PASSTHROUGH_IRQ; i++) {
        virq_passthrough_map[i].valid = false;
    }

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

void virq_ioapic_passthrough_ack(int ioapic, int pin, void *cookie)
{
    /* We are down-casting to microkit_channel so must first cast to size_t */
    microkit_irq_ack((microkit_channel)(size_t)cookie);
}

bool virq_ioapic_register_passthrough(int ioapic, int pin, microkit_channel irq_ch)
{
    if (irq_ch >= MICROKIT_MAX_CHANNELS) {
        LOG_VMM_ERR("Invalid channel number given '0x%lx' for passthrough virtual I/O APIC #%d IRQ pin 0x%lx\n", irq_ch,
                    ioapic, pin);
        return false;
    }

    if (ioapic != 0) {
        LOG_VMM_ERR("Invalid I/O APIC chip number given '0x%lx' for passthrough virtual I/O APIC #%d IRQ pin 0x%lx\n",
                    ioapic, ioapic, pin);
        return false;
    }

    if (pin >= IOAPIC_NUM_PINS) {
        LOG_VMM_ERR("Invalid I/O APIC pin number given '0x%lx' for passthrough virtual I/O APIC #%d IRQ pin 0x%lx\n",
                    pin, ioapic, pin);
        return false;
    }

    if (virq_passthrough_map[irq_ch].valid) {
        LOG_VMM_ERR("Channel %d is already registered to passthrough virtual I/O APIC #%d IRQ pin 0x%lx\n", irq_ch,
                    virq_passthrough_map[irq_ch].ioapic, virq_passthrough_map[irq_ch].pin);
        return false;
    }

    virq_passthrough_map[irq_ch].valid = true;
    virq_passthrough_map[irq_ch].ioapic = ioapic;
    virq_passthrough_map[irq_ch].pin = pin;
    virq_passthrough_map[irq_ch].ack_fn = virq_ioapic_passthrough_ack;
    virq_passthrough_map[irq_ch].ack_data = (void *)(uint64_t)irq_ch;

    return true;
}

bool virq_ioapic_handle_passthrough(microkit_channel irq_ch)
{
    if (irq_ch >= MICROKIT_MAX_CHANNELS) {
        LOG_VMM_ERR("attempted to handle invalid passthrough IRQ channel 0x%lx\n", irq_ch);
        return false;
    }

    if (virq_passthrough_map[irq_ch].valid == false) {
        LOG_VMM_ERR("attempted to handle unregistered passthrough IRQ channel 0x%lx\n", irq_ch);
        return false;
    }

    return inject_ioapic_irq(virq_passthrough_map[irq_ch].ioapic, virq_passthrough_map[irq_ch].pin);
}
