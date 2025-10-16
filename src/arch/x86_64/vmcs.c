/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <microkit.h>
#include <libvmm/vcpu.h>
#include <libvmm/util/util.h>

// @billn these should be turned into Microkit API calls
int vmcs_write(size_t vcpu_id, seL4_Word field, seL4_Word value) {
    seL4_X86_VCPU_WriteVMCS_t ret = seL4_X86_VCPU_WriteVMCS(BASE_VCPU_CAP + vcpu_id, field, value);
    assert(ret.error == seL4_NoError);

    return ret.error;
}

seL4_Word vmcs_read(size_t vcpu_id, seL4_Word field) {
    seL4_X86_VCPU_ReadVMCS_t ret = seL4_X86_VCPU_ReadVMCS(BASE_VCPU_CAP + vcpu_id, field);
    assert(ret.error == seL4_NoError);

    return ret.value;
}