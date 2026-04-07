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

struct guest_ram_region {
    uint64_t gpa_start;
    uint64_t gpa_end;
    void *vmm_vaddr;
};

int guest_ram_regions_len = 0;
struct guest_ram_region guest_ram_regions[MAX_GUEST_RAM_REGIONS];

bool guest_ram_add_region(uint64_t gpa, void *vmm_vaddr, uint64_t size)
{
    if (guest_ram_regions_len == MAX_GUEST_RAM_REGIONS) {
        LOG_VMM_ERR("guest_ram_add_region(): bookkeeping array is full\n");
        return false;
    }

    if (size == 0) {
        LOG_VMM_ERR("guest_ram_add_region(): size is 0\n");
        return false;
    }

    if (vmm_vaddr == NULL) {
        LOG_VMM_ERR("guest_ram_add_region(): vmm_vaddr is NULL\n");
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

void *gpa_to_vaddr(uint64_t gpa, size_t *bytes_remaining)
{
    if (guest_ram_regions_len == 0) {
        LOG_VMM_ERR("gpa_to_vaddr(): no recorded guest RAM region, have you called guest_ram_add_region()?\n");
        return NULL;
    }

    for (int i = 0; i < guest_ram_regions_len; i++) {
        if (gpa >= guest_ram_regions[i].gpa_start && gpa < guest_ram_regions[i].gpa_end) {
            uint64_t offset = gpa - guest_ram_regions[i].gpa_start;
            *bytes_remaining = guest_ram_regions[i].gpa_end - gpa;
            return (void *)((uintptr_t)guest_ram_regions[i].vmm_vaddr + offset);
        }
    }
    return NULL;
}

void *gpa_to_vaddr_or_crash(uint64_t gpa, size_t *bytes_remaining)
{
    void *ret = gpa_to_vaddr(gpa, bytes_remaining);
    assert(ret);
    return ret;
}