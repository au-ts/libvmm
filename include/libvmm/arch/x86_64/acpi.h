/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>

// Create all the ACPI tables from ram_top.
// Returns GPA of ACPI RSDP structure.
// The contiguous range of memory used by all ACPI tables are written to acpi_start_gpa and acpi_end_gpa
// Assume that the Guest-Physical Address of RAM is 0!!!
uint64_t acpi_rsdp_init(uintptr_t guest_ram_vaddr, uint64_t ram_top, uint64_t *acpi_start_gpa, uint64_t *acpi_end_gpa);