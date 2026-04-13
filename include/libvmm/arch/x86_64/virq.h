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

bool virq_ioapic_register(int ioapic, int pin, virq_ioapic_ack_fn_t ack_fn, void *ack_data);

bool virq_ioapic_register_passthrough(int ioapic, int pin, microkit_channel irq_ch);
bool virq_ioapic_handle_passthrough(microkit_channel irq_ch);

void virq_ioapic_passthrough_ack(int ioapic, int pin, void *cookie);