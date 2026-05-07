/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define GUEST_BOOT_VCPU_ID 0UL

/*
 * This define controls the maximum possible amount of vCPUs libvmm
 * can manage. The number of vCPUs actually managed is configured at
 * run-time via the API.
 */
#ifndef GUEST_MAX_NUM_VCPUS
#define GUEST_MAX_NUM_VCPUS 4UL
#endif

typedef struct guest {
    size_t num_vcpus;
    bool vcpu_on_state[GUEST_MAX_NUM_VCPUS];
} guest_t;

typedef struct arch_guest_init {
    size_t num_vcpus;
} arch_guest_init_t;

/* Initialise the architecture specific subsystems of the VMM library such as the
 * interrupt controller(s), etc. Note that the VCPU architectural state itself won't be
 * initialised, as that is handled by `guest_start`. */
bool guest_init(arch_guest_init_t init_args);

bool guest_start(uintptr_t kernel_pc, uintptr_t dtb, uintptr_t initrd);
void guest_stop();
bool guest_restart(uintptr_t guest_ram_vaddr, size_t guest_ram_size);
