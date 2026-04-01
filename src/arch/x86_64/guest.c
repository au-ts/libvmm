/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <libvmm/libvmm.h>
#include <libvmm/guest.h>

bool guest_paging_on(void)
{
    return microkit_vcpu_x86_read_vmcs(0, VMX_GUEST_CR0) & BIT(31);
}

bool guest_in_64_bits(void)
{
    return guest_paging_on() && (microkit_vcpu_x86_read_vmcs(0, VMX_GUEST_EFER) & BIT(10));
}
