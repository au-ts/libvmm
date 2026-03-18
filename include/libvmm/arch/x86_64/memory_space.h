/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* Document layout of guest RAM */

#define LOW_RAM_START_GPA 0x0

#define ECAM_GPA 0xD0000000
// 1 bus, 32 devices per bus, 8 functions per device, 4k config space per function
#define ECAM_SIZE (1 * 32 * 8 * 4096)

#define IOAPIC_GPA 0xFEC00000
#define IOAPIC_SIZE 0x1000

// This just matches the x86 CPU default physical address from
// See bit 9 of 'Table 1-20. More on Feature Information Returned in the EDX Register'.
#define LAPIC_GPA 0xFEE00000
#define LAPIC_SIZE 0x1000

#define HPET_GPA 0xFED00000
#define HPET_SIZE 0x1000