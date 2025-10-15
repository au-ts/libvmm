/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <microkit.h>
#include <libvmm/vcpu.h>
#include <libvmm/util/util.h>

void vcpu_reset(size_t vcpu_id)
{
    // TODO
    assert(false);
}

void vcpu_print_regs(size_t vcpu_id)
{
    // TODO
    assert(false);
}

int vcpu_write_vmcs(size_t vcpu_id, seL4_Word field, seL4_Word value) {
    seL4_X86_VCPU_WriteVMCS_t ret = seL4_X86_VCPU_WriteVMCS(BASE_VCPU_CAP + vcpu_id, field, value);
    assert(ret.error == seL4_NoError);

    return ret.error;
}