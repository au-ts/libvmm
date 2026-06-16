
/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <string.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/virq.h>
#include <libvmm/arch/x86_64/linux.h>

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-080US June 2023
 * 2. Intel 82093AA I/O ADVANCED PROGRAMMABLE INTERRUPT CONTROLLER (IOAPIC)
 *    Order Number: 290566-001 May 1996
 */

uintptr_t vapic_vaddr;
struct ioapic_regs ioapic_regs;

bool virq_controller_init(uintptr_t guest_vapic_vaddr)
{
    LOG_VMM("initialising IRQ book-keeping structures\n");
    memset(&ioapic_regs, 0, sizeof(struct ioapic_regs));

    LOG_VMM("initialising LAPIC\n");

    vapic_vaddr = guest_vapic_vaddr;

    // [1] "12.4.8 Local APIC Version Register"
    // "For processors based on the Nehalem microarchitecture (which has 7 LVT entries) and onward, the value returned is 6."
    lapic_write_reg(REG_LAPIC_REV, (uint32_t)0x10 | (uint32_t)6 << 16);
    // [1] "Figure 12-23. Spurious-Interrupt Vector Register (SVR)"
    lapic_write_reg(REG_LAPIC_SVR, (uint32_t)0xff); // reset value
    // [1] "Figure 12-8. Local Vector Table (LVT)"
    // LVT Timer Register
    lapic_write_reg(REG_LAPIC_TIMER, 0x10000); // reset value, masked.
    // [1] "Figure 12-8. Local Vector Table (LVT)"
    // "Specifies interrupt delivery when an interrupt is signaled at the LINT0 pin"
    lapic_write_reg(REG_LAPIC_LVT_LINT0, 0x10000); // reset value
    lapic_write_reg(REG_LAPIC_LVT_LINT1, 0x10000); // reset value
    // [1] "Figure 12-14. Destination Format Register (DFR)"
    lapic_write_reg(REG_LAPIC_DFR, 0xffffffff); // reset value

    LOG_VMM("initialising I/O APIC\n");
    // [2] default value for the Intel 82093AA IOAPIC.
    // supports 0x17 indirection entries.
    ioapic_regs.ioapicver = 0x11 | (IOAPIC_LAST_INDIRECT_INDEX << 16);
    // Wire up all the IRQs
    for (int i = 0; i <= IOAPIC_LAST_INDIRECT_INDEX; i++) {
        // fixed intr deivery, all zero
        // destination mode, physical mode to apic 0, all zero
        // delivery status, all zero
        // Interrupt Input Pin Polarity, high active, all zero
        // edge triggered, all zero
        // interrupt mask bit 16, mask all irq:
        ioapic_regs.ioredtbl[i] |= BIT(16);

        ioapic_regs.virq_passthrough_map[i].valid = false;
        ioapic_regs.virq_passthrough_map[i].ch = IOAPIC_IRQ_HANDLE_NO_CH;
    }

    return true;
}

void virq_ioapic_passthrough_ack(int ioapic, int pin, void *cookie)
{
    /* We are down-casting to microkit_channel so must first cast to size_t */
    microkit_irq_ack((microkit_channel)(size_t)cookie);
}

bool virq_ioapic_register(int ioapic, int pin, virq_ioapic_ack_fn_t ack_fn, void *ack_data)
{
    if (ioapic != 0) {
        LOG_VMM_ERR("Invalid I/O APIC chip number given '0x%x' for passthrough virtual I/O APIC #%d IRQ pin 0x%x\n",
                    ioapic, ioapic, pin);
        return false;
    }

    if (pin >= IOAPIC_NUM_PINS) {
        LOG_VMM_ERR("Invalid I/O APIC pin number given '0x%x' for passthrough virtual I/O APIC #%d IRQ pin 0x%x\n", pin,
                    ioapic, pin);
        return false;
    }

    if (ioapic_regs.virq_passthrough_map[pin].valid) {
        LOG_VMM_ERR("Pin %d is already registered on virtual I/O APIC\n", pin);
        return false;
    }

    ioapic_regs.virq_passthrough_map[pin].valid = true;
    ioapic_regs.virq_passthrough_map[pin].ack_fn = ack_fn;
    ioapic_regs.virq_passthrough_map[pin].ack_data = ack_data;

    return true;
}

bool virq_ioapic_register_passthrough(int ioapic, int pin, microkit_channel irq_ch)
{
    if (irq_ch >= MICROKIT_MAX_CHANNELS) {
        LOG_VMM_ERR("Invalid channel number given '0x%x' for passthrough virtual I/O APIC #%d IRQ pin 0x%x\n", irq_ch,
                    ioapic, pin);
        return false;
    }

    bool success = virq_ioapic_register(ioapic, pin, virq_ioapic_passthrough_ack, (void *)(uint64_t)irq_ch);
    if (success) {
        ioapic_regs.virq_passthrough_map[pin].ch = irq_ch;
    }

    return success;
}

bool virq_ioapic_handle_passthrough(microkit_channel irq_ch)
{
    if (irq_ch >= MICROKIT_MAX_CHANNELS) {
        LOG_VMM_ERR("attempted to handle invalid passthrough IRQ channel 0x%x\n", irq_ch);
        return false;
    }

    for (int i = 0; i < IOAPIC_NUM_PINS; i++) {
        if (ioapic_regs.virq_passthrough_map[i].valid) {
            if (ioapic_regs.virq_passthrough_map[i].ch == irq_ch) {
                return inject_ioapic_irq(0, i);
            }
        }
    }

    LOG_VMM_ERR("attempted to handle unregistered passthrough IRQ channel 0x%x\n", irq_ch);
    return false;
}
