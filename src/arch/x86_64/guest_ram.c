/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/arch/x86_64/guest_ram.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/vmcs.h>

struct guest_ram_region {
    uint64_t gpa_start;
    uint64_t gpa_end;
    uintptr_t vmm_vaddr;
};

int guest_ram_regions_len = 0;
struct guest_ram_region guest_ram_regions[MAX_GUEST_RAM_REGIONS];

#define X86_PAGING_OBJECT_SIZE 0x1000

bool guest_ram_add_region(uint64_t gpa, uintptr_t vmm_vaddr, uint64_t size)
{
    if (guest_ram_regions_len == MAX_GUEST_RAM_REGIONS) {
        LOG_VMM_ERR("guest_ram_add_region(): bookkeeping array is full\n");
        return false;
    }

    uint64_t gpa_end = gpa + size;

    for (int i = 0; i < guest_ram_regions_len; i++) {
        if (ranges_overlap(gpa, gpa_end, guest_ram_regions[i].gpa_start, guest_ram_regions[i].gpa_end)) {
            LOG_VMM_ERR("guest_ram_add_region(): region [0x%lx..0x%lx) overlaps with existing region [0x%lx..0x%lx)\n",
                        gpa, gpa_end, guest_ram_regions[i].gpa_start, guest_ram_regions[i].gpa_end);
        }
        return false;
    }

    guest_ram_regions[guest_ram_regions_len] = (struct guest_ram_region) {
        .gpa_start = gpa,
        .gpa_end = gpa_end,
        .vmm_vaddr = vmm_vaddr,
    };
    guest_ram_regions_len++;

    return true;
}

bool gpa_to_vaddr(uint64_t gpa, void **ret, int *bytes_remaining)
{
    for (int i = 0; i < guest_ram_regions_len; i++) {
        // LOG_VMM("chekcing 0x%lx, start 0x%lx, end 0x%lx\n", gpa, guest_ram_regions[i].gpa_start, guest_ram_regions[i].gpa_end);
        if (gpa >= guest_ram_regions[i].gpa_start && gpa < guest_ram_regions[i].gpa_end) {
            uint64_t offset = gpa - guest_ram_regions[i].gpa_start;
            *ret = (void *)(guest_ram_regions[i].vmm_vaddr + offset);
            *bytes_remaining = guest_ram_regions[i].gpa_end - gpa;
            return true;
        }
    }
    return false;
}

bool gva_to_gpa(size_t vcpu_id, uint64_t gva, uint64_t *gpa, int *bytes_remaining)
{
    // Make sure that paging is on
    if (!guest_paging_on()) {
        *gpa = gva;
    }

    int pt_bytes_remaining;

    uint64_t pml4_gpa = microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_CR3) & ~0xfff;
    uint64_t *pml4;
    if (!gpa_to_vaddr(pml4_gpa, (void *)&pml4, &pt_bytes_remaining)) {
        LOG_VMM_ERR("PML4 GPA 0x%lx not in any valid guest RAM regions\n", pml4_gpa);
        return false;
    }
    if (pt_bytes_remaining < X86_PAGING_OBJECT_SIZE) {
        LOG_VMM_ERR("PML4 GPA 0x%lx overflows guest RAM regions\n", pml4_gpa);
        return false;
    }

    uint64_t pml4_idx = (gva >> (12 + (9 * 3))) & 0x1ff;
    uint64_t pml4_pte = pml4[pml4_idx];
    if (!pte_present(pml4_pte)) {
        LOG_VMM_ERR("PML4 PTE not present when converting GVA 0x%lx to GPA\n", gva);
        return false;
    }

    uint64_t pdpt_gpa = pte_to_gpa(pml4_pte);
    uint64_t *pdpt;
    if (!gpa_to_vaddr(pdpt_gpa, (void *)&pdpt, &pt_bytes_remaining)) {
        LOG_VMM_ERR("PDPT GPA 0x%lx not in any valid guest RAM regions\n", pdpt_gpa);
        return false;
    }
    if (pt_bytes_remaining < X86_PAGING_OBJECT_SIZE) {
        LOG_VMM_ERR("PDPT GPA 0x%lx overflows guest RAM regions\n", pdpt_gpa);
        return false;
    }

    uint64_t pdpt_idx = (gva >> (12 + (9 * 2))) & 0x1ff;
    uint64_t pdpt_pte = pdpt[pdpt_idx];
    if (!pte_present(pdpt_pte)) {
        LOG_VMM_ERR("PDPT PTE not present when converting GVA 0x%lx to GPA\n", gva);
        return false;
    }

    uint64_t pd_gpa = pte_to_gpa(pdpt_pte);
    uint64_t *pd;
    if (!gpa_to_vaddr(pd_gpa, (void *)&pd, &pt_bytes_remaining)) {
        LOG_VMM_ERR("PD GPA 0x%lx not in any valid guest RAM regions\n", pd_gpa);
        return false;
    }
    if (pt_bytes_remaining < X86_PAGING_OBJECT_SIZE) {
        LOG_VMM_ERR("PD GPA 0x%lx overflows guest RAM regions\n", pd_gpa);
        return false;
    }

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
        uint64_t *pt;
        if (!gpa_to_vaddr(pt_gpa, (void *)&pt, &pt_bytes_remaining)) {
            LOG_VMM_ERR("PT GPA 0x%lx not in any valid guest RAM regions\n", pt_gpa);
            return false;
        }
        if (pt_bytes_remaining < X86_PAGING_OBJECT_SIZE) {
            LOG_VMM_ERR("PT GPA 0x%lx overflows guest RAM regions\n", pt_gpa);
            return false;
        }

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