/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdbool.h>
#include <sel4/sel4.h>

bool fault_handle_sbi(size_t vcpu_id, seL4_UserContext *regs);
