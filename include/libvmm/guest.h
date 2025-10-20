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

bool guest_start(uintptr_t kernel_pc, uintptr_t dtb, uintptr_t initrd, void *linux_x86_setup);
void guest_stop();
bool guest_restart(uintptr_t guest_ram_vaddr, size_t guest_ram_size);
