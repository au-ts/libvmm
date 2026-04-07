/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/util/util.h>

/* Enough for low RAM + high RAM and a firmware region */
#define MAX_GUEST_RAM_REGIONS 3

/* Tell libvmm valid guest physical RAM region */
bool guest_ram_add_region(uint64_t gpa, void *vmm_vaddr, uint64_t size);

/* Convert guest physical address to the VMM's virtual memory address.
 * `bytes_remaining` will contain the number of bytes to the end of the region. */
void *gpa_to_vaddr(uint64_t gpa, size_t *bytes_remaining);
void *gpa_to_vaddr_or_crash(uint64_t gpa, size_t *bytes_remaining);

#if defined(CONFIG_ARCH_X86_64)
/* Convert guest virtual address to guest physical address. `bytes_remaining` will
 * contain number of bytes to page boundary. */
bool gva_to_gpa(size_t vcpu_id, uint64_t gva, uint64_t *gpa, size_t *bytes_remaining);
#endif