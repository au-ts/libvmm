/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>

#ifndef MAX_PASSTHROUGH_IRQ
#define MAX_PASSTHROUGH_IRQ MICROKIT_MAX_CHANNELS
#endif

enum irq_type {
    IRQ_TYPE_INVALID = 0,
    IRQ_TYPE_ARM_GIC = 1,
    IRQ_TYPE_X86_IOAPIC = 2,
};

typedef struct irq_routing_info {
    enum irq_type type;
    union {
        struct arm_gic {
            int vcpu_id;
            uint32_t intid;
        } arm_gic;

        struct x86_ioapic {
            uint8_t chip;
            uint8_t pin;
        } x86_ioapic;
    } hw;
} irq_routing_info_t;

#define ARM_GIC_IRQ_ROUTE(_vcpu_id, _intid) \
    (irq_routing_info_t) { \
        .type = IRQ_TYPE_ARM_GIC, \
        .hw.arm_gic = { \
            .vcpu_id = (_vcpu_id), \
            .intid = (_intid) \
        } \
    }

#define X86_IOAPIC_IRQ_ROUTE(_chip, _pin) \
    (irq_routing_info_t) { \
        .type = IRQ_TYPE_X86_IOAPIC, \
        .hw.x86_ioapic = { \
            .chip = (_chip), \
            .pin = (_pin) \
        } \
    }

#define IRQ_ROUTE_INVALID(route) (route.type == IRQ_TYPE_INVALID)

/* Type specific utilities, they all assume that the route is already that specific type! */
#define IRQ_ROUTE_TO_ARM_CPUID(routing) (routing.hw.arm_gic.vcpu_id)
#define IRQ_ROUTE_TO_ARM_INTID(routing) (routing.hw.arm_gic.intid)
#define IRQ_ROUTE_TO_X86_IOAPIC_CHIP(routing) (routing.hw.x86_ioapic.chip)
#define IRQ_ROUTE_TO_X86_IOAPIC_PIN(routing) (routing.hw.x86_ioapic.pin)

typedef void (*virq_ack_fn_t)(irq_routing_info_t irq_routing_info, void *cookie);

#if defined(CONFIG_ARCH_ARM)
/*
 * Initialise the architecture-depedent virtual interrupt controller.
 * On ARM, this is the virtual Generic Interrupt Controller (vGIC).
 */
bool virq_controller_init();
#elif defined(CONFIG_ARCH_X86)
/*
 * Initialise the virtual LAPIC and I/O APIC.
 * When APIC_VIRT_LEVEL == APIC_VIRT_LEVEL_APICV, you need to pass the HVA of the "Virtual APIC" page,
 * consult the Intel SDM for more details.
 */
bool virq_controller_init(uintptr_t apicv_hva);
#endif

/*
 * Inject an edge-triggered IRQ (a discrete message or pulse).
 * Use this for MSI/MSI-X, edge-triggered timers, or IPIs.
 */
bool virq_inject(irq_routing_info_t irq_routing_info);

/*
 * Set the state of a level-sensitive IRQ line.
 * Use this for legacy PCI INTx, level-triggered timers.
 */
bool virq_set_level(irq_routing_info_t irq_routing_info, bool level);

/* Register or deregister an IRQ with the virtual IRQ controller.
 * An IRQ must be registered before they can be injected into the guest. */
bool virq_register(irq_routing_info_t irq_routing_info, virq_ack_fn_t ack_fn, void *ack_data);
#if defined(CONFIG_ARCH_X86)
bool virq_deregister(irq_routing_info_t irq_routing_info);
#endif

/*
 * These two APIs are convenient for when you want to directly passthrough an IRQ from
 * the hardware to the guest as the same vIRQ. This is useful when the guest has direct
 * passthrough access to a particular device on the hardware.
 * After registering the passthrough IRQ, call `virq_handle_passthrough` when
 * the IRQ has come through from seL4.
 */
bool virq_register_passthrough(irq_routing_info_t irq_routing_info, microkit_channel irq_ch);
bool virq_handle_passthrough(microkit_channel irq_ch);