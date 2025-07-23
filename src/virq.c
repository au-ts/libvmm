/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <libvmm/virq.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>

/* Maps Microkit channel numbers with registered vIRQ */
int virq_passthrough_map[MAX_PASSTHROUGH_IRQ] = {-1};

static void virq_passthrough_ack(size_t vcpu_id, int irq, void *cookie)
{
    /* We are down-casting to microkit_channel so must first cast to size_t */
    microkit_irq_ack((microkit_channel)(size_t)cookie);
}

bool virq_register_passthrough(size_t vcpu_id, size_t irq, microkit_channel irq_ch)
{
    assert(irq_ch < MICROKIT_MAX_CHANNELS);
    if (irq_ch >= MICROKIT_MAX_CHANNELS) {
        LOG_VMM_ERR("Invalid channel number given '0x%lx' for passthrough vIRQ 0x%lx\n", irq_ch, irq);
        return false;
    }

    LOG_VMM("Register passthrough vIRQ 0x%lx on vCPU 0x%lx (IRQ channel: 0x%lx)\n", irq, vcpu_id, irq_ch);
    virq_passthrough_map[irq_ch] = irq;

    bool success = virq_register(GUEST_VCPU_ID, irq, &virq_passthrough_ack, (void *)(size_t)irq_ch);
    assert(success);
    if (!success) {
        LOG_VMM_ERR("Failed to register passthrough vIRQ %d\n", irq);
        return false;
    }

    return true;
}

bool virq_handle_passthrough(microkit_channel irq_ch)
{
    assert(virq_passthrough_map[irq_ch] >= 0);
    if (virq_passthrough_map[irq_ch] < 0) {
        LOG_VMM_ERR("attempted to handle invalid passthrough IRQ channel 0x%lx\n", irq_ch);
        return false;
    }

    bool success = virq_inject(GUEST_VCPU_ID, virq_passthrough_map[irq_ch]);
    if (!success) {
        LOG_VMM_ERR("could not inject passthrough vIRQ 0x%lx, dropped on vCPU 0x%lx\n", virq_passthrough_map[irq_ch],
                    GUEST_VCPU_ID);
        return false;
    }

    return true;
}
