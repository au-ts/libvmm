/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <string.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

uint64_t gpa_to_pa(uint64_t gpa);

// Convert guest virtual address to guest physical address, using whichever page table is currently in the guest's CR3.
// Returns:
// - gpa: guest physical address
// - bytes_remaining: number of bytes to the page boundary. Meaning UB if you overrun (gpa + bytes_remaining)
bool gva_to_gpa(size_t vcpu_id, uint64_t gva, uint64_t *gpa, int *bytes_remaining);

// Convert guest physical address to the VMM's virtual memory address.
void *gpa_to_vaddr(uint64_t gpa);