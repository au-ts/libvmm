/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <libvmm/fault.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/riscv/plic.h>

bool virq_controller_init(size_t boot_vcpu_id)
{
    bool success = fault_register_vm_exception_handler(PLIC_ADDR, PLIC_SIZE, plic_handle_fault, NULL);
    if (!success) {
        LOG_VMM_ERR("Failed to register fault handler for PLIC region\n");
        return false;
    }
    assert(success);

    return success;
}

bool virq_inject(size_t vcpu_id, int irq)
{
    return plic_inject_irq(vcpu_id, irq);
}

bool virq_register(size_t vcpu_id, size_t irq, virq_ack_fn_t ack_fn, void *ack_data)
{
    return plic_register_irq(vcpu_id, irq, ack_fn, ack_data);
}
