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
 */
bool virq_controller_init(uintptr_t guest_vapic_vaddr);
#endif

/*
 * Inject an IRQ into the guest according to the routing information.
 * Note that this API requires that the IRQ has been registered (with virq_register).
 */
bool virq_inject(irq_routing_info_t irq_routing_info);

bool virq_register(irq_routing_info_t irq_routing_info, virq_ack_fn_t ack_fn, void *ack_data);

/*
 * These two APIs are convenient for when you want to directly passthrough an IRQ from
 * the hardware to the guest as the same vIRQ. This is useful when the guest has direct
 * passthrough access to a particular device on the hardware.
 * After registering the passthrough IRQ, call `virq_handle_passthrough` when
 * the IRQ has come through from seL4.
 */
bool virq_register_passthrough(irq_routing_info_t irq_routing_info, microkit_channel irq_ch);
bool virq_handle_passthrough(microkit_channel irq_ch);