/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define GUEST_BOOT_VCPU_ID 0

#ifndef GUEST_NUM_VCPUS
#define GUEST_NUM_VCPUS 1
#endif

bool guest_start(uintptr_t kernel_pc, uintptr_t dtb, uintptr_t initrd);
void guest_stop();
bool guest_restart(uintptr_t guest_ram_vaddr, size_t guest_ram_size);

// Convert guest physical address to the VMM's virtual memory address.
void *gpa_to_vaddr(uint64_t gpa);

#if defined(CONFIG_ARCH_X86)
bool guest_paging_on(void);
bool guest_in_64_bits(void);
bool gva_to_gpa(size_t vcpu_id, uint64_t gva, uint64_t *gpa, uint64_t *bytes_remaining);
uint64_t gpa_to_pa(uint64_t gpa);
#endif