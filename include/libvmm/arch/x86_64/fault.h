/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>

bool fault_is_write(seL4_Word fsr);
bool fault_is_read(seL4_Word fsr);

/* @billn revisit, seL4 vmexit doesnt have msginfo! */
bool fault_handle(size_t vcpu_id);