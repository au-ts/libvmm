/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>

struct ArmCoprocessorRegInfo {

    char *name;

};

bool handle_sysreg_64_fault(size_t vcpu_id, uint32_t hsr, seL4_UserContext *regs);