/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <microkit.h>

bool vcpu_init(size_t vcpu_id, uintptr_t rip, uintptr_t rsp);