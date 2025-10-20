/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdbool.h>

#include <libvmm/vcpu.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/vmcs.h>

/* Document referenced:
 * [1] Title: Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4 Order Number: 325462-080US June 2023
 *   [1a] Location: "SYSTEM ARCHITECTURE OVERVIEW", page: "Vol. 3A 2-15"
 */

void vcpu_reset(size_t vcpu_id) {

}

bool vcpu_is_on(size_t vcpu_id) {
    return true;
}

void vcpu_set_on(size_t vcpu_id, bool on) {

}

void vcpu_print_regs(size_t vcpu_id) {

}