/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/guest.h>
#include <libvmm/guest_ram.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/vmcs.h>

#define X86_PAGING_OBJECT_SIZE 0x1000

bool gva_to_gpa(size_t vcpu_id, uint64_t gva, uint64_t *gpa, size_t *bytes_remaining)
{
    // Make sure that paging is on
    if (!guest_paging_on()) {
        *gpa = gva;
    }

    size_t pt_bytes_remaining;

    uint64_t pml4_gpa = microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_CR3) & ~0xfff;
    uint64_t *pml4 = gpa_to_vaddr_or_crash(pml4_gpa, &pt_bytes_remaining);
    assert(pt_bytes_remaining >= X86_PAGING_OBJECT_SIZE);
    uint64_t pml4_idx = (gva >> (12 + (9 * 3))) & 0x1ff;
    uint64_t pml4_pte = pml4[pml4_idx];
    if (!pte_present(pml4_pte)) {
        LOG_VMM_ERR("PML4 PTE not present when converting GVA 0x%lx to GPA\n", gva);
        return false;
    }

    uint64_t pdpt_gpa = pte_to_gpa(pml4_pte);
    uint64_t *pdpt = gpa_to_vaddr_or_crash(pdpt_gpa, &pt_bytes_remaining);
    assert(pt_bytes_remaining >= X86_PAGING_OBJECT_SIZE);
    uint64_t pdpt_idx = (gva >> (12 + (9 * 2))) & 0x1ff;
    uint64_t pdpt_pte = pdpt[pdpt_idx];
    if (!pte_present(pdpt_pte)) {
        LOG_VMM_ERR("PDPT PTE not present when converting GVA 0x%lx to GPA\n", gva);
        return false;
    }

    uint64_t pd_gpa = pte_to_gpa(pdpt_pte);
    uint64_t *pd = gpa_to_vaddr_or_crash(pd_gpa, &pt_bytes_remaining);
    assert(pt_bytes_remaining >= X86_PAGING_OBJECT_SIZE);
    uint64_t pd_idx = (gva >> (12 + (9 * 1))) & 0x1ff;
    uint64_t pd_pte = pd[pd_idx];
    if (!pte_present(pd_pte)) {
        LOG_VMM_ERR("PD PTE not present when converting GVA 0x%lx to GPA\n", gva);
        return false;
    }

    if (pt_page_size(pd_pte) && pte_present(pd_pte)) {
        // 2MiB page
        uint64_t page_gpa = pte_to_gpa(pd_pte);
        uint64_t page_offset = (gva & 0x1fffff);
        *gpa = page_gpa + page_offset;
        *bytes_remaining = 0x200000 - page_offset;
    } else {
        // 4k page
        uint64_t pt_gpa = pte_to_gpa(pd_pte);
        uint64_t *pt = gpa_to_vaddr_or_crash(pt_gpa, &pt_bytes_remaining);
        ;
        assert(pt_bytes_remaining >= X86_PAGING_OBJECT_SIZE);
        uint64_t pt_idx = (gva >> (12)) & 0x1ff;
        uint64_t pt_pte = pt[pt_idx];
        if (!pte_present(pt_pte)) {
            LOG_VMM_ERR("PT PTE not present when converting GVA 0x%lx to GPA\n", gva);
            return 0;
        }

        uint64_t page_gpa = pte_to_gpa(pt_pte);
        uint64_t page_offset = (gva & 0xfff);
        *gpa = page_gpa + page_offset;
        *bytes_remaining = 0x1000 - page_offset;
    }

    return true;
}