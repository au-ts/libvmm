/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sddf/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/msr.h>
#include <libvmm/arch/x86_64/vmcs.h>

bool emulate_rdmsr(seL4_VCPUContext *vctx) {
    if (vctx->ecx == 0xc0000080) {
        uint32_t efer_low = (uint32_t) vmcs_read(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER);
        vctx->eax = efer_low;
        vctx->edx = 0;
    } else if (vctx->ecx == 0xcf) {
        vctx->eax = 0;
        vctx->edx = 0;
    } else if (vctx->ecx == 0x33) {
        vctx->eax = 0;
        vctx->edx = 0;
    } else {
        return false;
    }
    return true;
}

bool emulate_wrmsr(seL4_VCPUContext *vctx) {
    if (vctx->ecx == 0xc0000080) {
        vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER, vctx->eax);
    } else if (vctx->ecx == 0x33) {
        LOG_VMM("got ecx 0x33\n");
    } else {
        LOG_VMM("unknown wrmsr 0x%x\n", vctx->eax);
        return false;
    }

    return true;
}

