
/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/vmcs.h>

extern uintptr_t guest_ram_vaddr;
extern uintptr_t guest_high_ram_vaddr;
extern uint64_t guest_high_ram_size;
extern uintptr_t guest_flash_vaddr;

static uint64_t pte_to_gpa(uint64_t pte)
{
    assert(pte & 1);
    return pte & 0xffffffffff000;
}

static bool pte_present(uint64_t pte)
{
    return pte & BIT(0);
}

static bool pt_page_size(uint64_t pte)
{
    return pte & BIT(7);
}

void *gpa_to_vaddr(uint64_t gpa)
{
    // @billn ugly hack
    uint64_t firmware_region_base_gpa = 0xffa00000;
    uint64_t high_ram_base_gpa = 0x100000000;
    if (gpa < firmware_region_base_gpa) {
        return (void *)(guest_ram_vaddr + gpa);
    } else if (gpa >= high_ram_base_gpa && gpa < high_ram_base_gpa + guest_high_ram_size) {
        // @billn ugly one-to-one hack
        return (void *) gpa;
    } else if (gpa >= firmware_region_base_gpa && gpa < high_ram_base_gpa){
        return (void *)(guest_flash_vaddr + (gpa - firmware_region_base_gpa));
    } else {
        LOG_VMM_ERR("gpa_to_vaddr(): GPA 0x%lx not in any valid guest memory regions\n", gpa);
        return NULL;
    }
}

uint64_t gpa_to_pa(uint64_t gpa)
{
    // @billn ugly hack
    uint64_t ram_base_gpa = 0x20000000;
    uint64_t high_ram_base_gpa = 0x100000000;
    if (gpa >= high_ram_base_gpa && gpa < high_ram_base_gpa + guest_high_ram_size) {
        // @billn ugly one-to-one hack
        return gpa;
    } else if (gpa < high_ram_base_gpa){
        return 0x20000000 + gpa;
    } else {
        LOG_VMM_ERR("gpa_to_pa(): GPA 0x%lx not in any valid guest memory regions\n", gpa);
        return 0;
    }
}

bool gva_to_gpa(size_t vcpu_id, uint64_t gva, uint64_t *gpa, int *bytes_remaining) {
    // Make sure that paging is on
    assert(microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_CR0) & BIT(31));

    uint64_t pml4_gpa = microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_CR3) & ~0xfff;
    uint64_t *pml4 = gpa_to_vaddr(pml4_gpa);
    uint64_t pml4_idx = (gva >> (12 + (9 * 3))) & 0x1ff;
    uint64_t pml4_pte = pml4[pml4_idx];
    if (!pte_present(pml4_pte)) {
        LOG_VMM_ERR("PML4 PTE not present when converting GVA 0x%lx to GPA\n", gva);
        return false;
    }

    uint64_t pdpt_gpa = pte_to_gpa(pml4_pte);
    uint64_t *pdpt = gpa_to_vaddr(pdpt_gpa);
    uint64_t pdpt_idx = (gva >> (12 + (9 * 2))) & 0x1ff;
    uint64_t pdpt_pte = pdpt[pdpt_idx];
    if (!pte_present(pdpt_pte)) {
        LOG_VMM_ERR("PDPT PTE not present when converting GVA 0x%lx to GPA\n", gva);
        return false;
    }

    uint64_t pd_gpa = pte_to_gpa(pdpt_pte);
    uint64_t *pd = gpa_to_vaddr(pd_gpa);
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
        uint64_t *pt = gpa_to_vaddr(pt_gpa);
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