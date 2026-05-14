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

extern guest_t guest;

bool guest_ram_add_region(struct guest_ram_region guest_ram_region)
{
    if (guest.guest_ram_regions_len == GUEST_MAX_RAM_REGIONS) {
        LOG_VMM_ERR("bookkeeping array is full\n");
        return false;
    }

    if (guest_ram_region.size == 0) {
        LOG_VMM_ERR("size is 0\n");
        return false;
    }

    if (guest_ram_region.vmm_vaddr == NULL) {
        LOG_VMM_ERR("vmm_vaddr is NULL\n");
        return false;
    }

    uint64_t this_gpa_start = guest_ram_region.gpa_start;
    uint64_t this_gpa_end = this_gpa_start + guest_ram_region.size;

    for (int i = 0; i < guest.guest_ram_regions_len; i++) {
        uint64_t other_start = guest.guest_ram_regions[i].gpa_start;
        uint64_t other_end = other_start + guest.guest_ram_regions[i].size;
        if (ranges_overlap(this_gpa_start, this_gpa_end, other_start, other_end)) {
            LOG_VMM_ERR("guest_ram_add_region(): region [0x%lx..0x%lx) overlaps with existing region [0x%lx..0x%lx)\n",
                        this_gpa_start, this_gpa_end, other_start, other_end);
        }
        return false;
    }

    memcpy(&guest.guest_ram_regions[guest.guest_ram_regions_len], &guest_ram_region, sizeof(struct guest_ram_region));
    guest.guest_ram_regions_len++;

    return true;
}

struct guest_ram_region *guest_ram_get_regions(int *num_regions)
{
    *num_regions = guest.guest_ram_regions_len;
    return guest.guest_ram_regions;
}

void *gpa_to_vaddr(uint64_t gpa, size_t *bytes_remaining)
{
    assert(guest.guest_ram_regions_len);

    for (int i = 0; i < guest.guest_ram_regions_len; i++) {
        uint64_t this_region_gpa_start = guest.guest_ram_regions[i].gpa_start;
        uint64_t this_region_gpa_end = this_region_gpa_start + guest.guest_ram_regions[i].size;
        if (gpa >= this_region_gpa_start && gpa < this_region_gpa_end) {
            uint64_t offset = gpa - this_region_gpa_start;
            *bytes_remaining = this_region_gpa_end - gpa;
            return (void *)((uintptr_t)guest.guest_ram_regions[i].vmm_vaddr + offset);
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