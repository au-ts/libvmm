/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/fault.h>
#include <libvmm/arch/aarch64/vgic/vgic.h>
#include <libvmm/arch/aarch64/vgic/vgic_v3_cpuif.h>

bool icc_sgi1r_el1_access(size_t vcpu_id, seL4_UserContext *regs, bool is_read)
{
#if defined(GIC_V3)
    if (!is_read) {
        return true;
    } else {
        return false;
    }
#else
    /* This register doesn't exist on GICv2. */
    LOG_VMM("icc_sgi1r_el1_access() was unexpectedly called on a GICv2 build of libvmm\n");
    return false;
#endif
}

bool icc_sgi1r_el1_write(size_t vcpu_id, seL4_UserContext *regs, uint64_t data)
{
    uint8_t intid = (data >> 24) & 0xf;
    for (int i = 0; i < GUEST_NUM_VCPUS; i++) {
        // check the target list and raise IRQ on the approproate vCPUs.
        // @billn double check: make sure IRM bit is honoured.
        // @billn double check: affinity routing?
        if (data >> i & 0x1) {
            assert(vgic_inject_irq(i, intid));
        }
    }
    return fault_advance_vcpu(vcpu_id, regs);
}