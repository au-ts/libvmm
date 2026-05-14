/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/util/util.h>

/* Enough for now */
#define GUEST_MAX_RAM_REGIONS 1

struct guest_ram_region {
    uint64_t gpa_start;
    uint64_t size;
    void *vmm_vaddr;
};

/* Tell libvmm valid guest physical RAM region */
bool guest_ram_add_region(struct guest_ram_region guest_ram_region);

/* Convert guest physical address to the VMM's virtual memory address.
 * `bytes_remaining` will contain the number of bytes to the end of the region. */
void *gpa_to_vaddr(uint64_t gpa, size_t *bytes_remaining);
void *gpa_to_vaddr_or_crash(uint64_t gpa, size_t *bytes_remaining);

/* Returns the list of guest RAM regions registered. The number of regions
 * in the list will be written to `num_regions_ret`. */
struct guest_ram_region *guest_ram_get_regions(int *num_regions_ret);

#if defined(CONFIG_ARCH_X86_64)
/* Convert guest virtual address to guest physical address. `bytes_remaining` will
 * contain number of bytes to page boundary. */
bool gva_to_gpa(size_t vcpu_id, uint64_t gva, uint64_t *gpa, size_t *bytes_remaining);
#endif
