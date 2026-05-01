
/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/vmcs.h>

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-088US June 2025
 */

/* Table 29-7. "Exit Qualification for EPT Violations" */
#define EPT_VIOLATION_READ_BIT BIT(0)
#define EPT_VIOLATION_WRITE_BIT BIT(1)

bool ept_fault_is_read(seL4_Word qualification)
{
    return !!(qualification & EPT_VIOLATION_READ_BIT);
}

bool ept_fault_is_write(seL4_Word qualification)
{
    return !!(qualification & EPT_VIOLATION_WRITE_BIT);
}

uint64_t pte_to_gpa(uint64_t pte)
{
    assert(pte & 1);
    return pte & 0xffffffffff000;
}

bool pte_present(uint64_t pte)
{
    return pte & BIT(0);
}

bool pt_page_size(uint64_t pte)
{
    return pte & BIT(7);
}

bool guest_paging_on(void)
{
    return microkit_vcpu_x86_read_vmcs(0, VMX_GUEST_CR0) & BIT(31);
}

bool guest_in_64_bits(void)
{
    return guest_paging_on() && (microkit_vcpu_x86_read_vmcs(0, VMX_GUEST_EFER) & BIT(10));
}
