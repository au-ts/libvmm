
/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/vmcs.h>

/* Table 28-7. Exit Qualification for EPT Violations */
#define EPT_VIOLATION_READ (1 << 0)
#define EPT_VIOLATION_WRITE (1 << 1)

bool ept_fault_is_read(seL4_Word qualification)
{
    return qualification & EPT_VIOLATION_READ;
}

bool ept_fault_is_write(seL4_Word qualification)
{
    return qualification & EPT_VIOLATION_WRITE;
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
