/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/cpuif.h>

bool icc_sgi1r_el1_write(size_t vcpu_id, seL4_UserContext *regs, uint64_t data);

bool icc_sre_el1_read(size_t vcpu_id, seL4_UserContext *regs, uint64_t *data);
