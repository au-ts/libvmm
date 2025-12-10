/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stddef.h>
#include <stdbool.h>
#include <microkit.h>

#ifndef MAX_PASSTHROUGH_IRQ
#define MAX_PASSTHROUGH_IRQ MICROKIT_MAX_CHANNELS
#endif

typedef void (*virq_ioapic_ack_fn_t)(int ioapic, int pin, void *cookie);

/*
 * Initialise the virtual LAPIC and I/O APIC.
 */
bool virq_controller_init(uint64_t native_tsc_hz);

bool virq_ioapic_register_passthrough(int ioapic, int pin, microkit_channel irq_ch);
bool virq_ioapic_handle_passthrough(microkit_channel irq_ch);