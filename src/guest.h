/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>

#define GUEST_VCPU_ID 0
#define GUEST_NUM_VCPUS 1

bool guest_start(size_t boot_vcpu_id, uintptr_t kernel_pc, uintptr_t dtb, uintptr_t initrd);
void guest_stop(size_t boot_vcpu_id);
bool guest_restart(size_t boot_vcpu_id, uintptr_t guest_ram_vaddr, size_t guest_ram_size);
