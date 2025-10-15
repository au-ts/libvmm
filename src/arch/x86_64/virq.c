
/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <libvmm/arch/x86_64/linux.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>

bool virq_controller_init()
{
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