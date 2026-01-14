/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <microkit.h>

typedef void (*virq_ioapic_ack_fn_t)(int ioapic, int pin, void *cookie);

/*
 * Initialise the virtual LAPIC and I/O APIC.
 */
bool virq_controller_init(uint64_t native_tsc_hz);

bool virq_ioapic_register(int ioapic, int pin, virq_ioapic_ack_fn_t ack_fn, void *ack_data);

bool virq_ioapic_register_passthrough(int ioapic, int pin, microkit_channel irq_ch);
bool virq_ioapic_handle_passthrough(microkit_channel irq_ch);


// @billn sus
#define IOAPIC_IRQ_HANDLE_NO_CH (-1)
struct ioapic_virq_handle {
    bool valid;
    int ch;
    virq_ioapic_ack_fn_t ack_fn;
    void *ack_data;
};
void virq_ioapic_passthrough_ack(int ioapic, int pin, void *cookie);