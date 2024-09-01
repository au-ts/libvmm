/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stddef.h>

void vcpu_reset(size_t vcpu_id);
void vcpu_print_regs(size_t vcpu_id);
