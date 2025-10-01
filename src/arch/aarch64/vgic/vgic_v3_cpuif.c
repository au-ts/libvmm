/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <libvmm/guest.h>
#include <libvmm/vcpu.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/fault.h>
#include <libvmm/arch/aarch64/vgic/vgic.h>
#include <libvmm/arch/aarch64/vgic/vgic_v3_cpuif.h>

#define ICC_SGI1R_EL1_IRM_BIT 40
#define ICC_SGI1R_EL1_INTID_SHIFT 24
#define ICC_SGI1R_EL1_INTID_MASK  0xf

bool icc_sgi1r_el1_write(size_t vcpu_id, seL4_UserContext *regs, uint64_t data)
{
    uint8_t intid = (data >> ICC_SGI1R_EL1_INTID_SHIFT) & ICC_SGI1R_EL1_INTID_MASK;

    if ((data >> ICC_SGI1R_EL1_IRM_BIT) & 0x1) {
        /* Interrupt routed to all PEs in the system, excluding “self”. */
        for (int i = 0; i < GUEST_NUM_VCPUS; i++) {
            if (vcpu_is_on(i) && i != vcpu_id) {
                assert(vgic_inject_irq(i, intid));
            }
        }
    } else {
        /* Interrupt routed to the PEs specified by Aff3.Aff2.Aff1.<target list>.
           Where the bit number in target specify vCPU ID. */
        uint16_t target_list = data & 0xffff;
        for (int i = 0; i < GUEST_NUM_VCPUS && i < 16; i++) {
            // @billn revisit: affinity routing for > 16 vCPUs
            if ((target_list >> i & 0x1) && vcpu_is_on(i)) {
                assert(vgic_inject_irq(i, intid));
            }
        }
    }

    return fault_advance_vcpu(vcpu_id, regs);
}