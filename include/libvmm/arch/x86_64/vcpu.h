/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <microkit.h>

#include <libvmm/util/util.h>
#include <sddf/util/util.h>

/* Document referenced:
 * [1] Title: Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4 Order Number: 325462-080US June 2023
 *   [1a] Location: "SYSTEM ARCHITECTURE OVERVIEW", page: "Vol. 3A 2-15"
 *   [1b] Location: "SYSTEM ARCHITECTURE OVERVIEW", page: "Vol. 3A 2-17"
 *   [1c] Location: "SYSTEM ARCHITECTURE OVERVIEW", page: "Vol. 3A 2-9"
 *   [1d] Location: "BASIC EXECUTION ENVIRONMENT", page: "3-16 Vol. 1" 
 */

/* "CR0 — Contains system control flags that control operating mode and states of the processor."
 * Consult [1a] for more details
 */
#define CR0_PG BIT_LOW(31) /* Paging On */
#define CR0_PE BIT_LOW(0)  /* Protection Enable */

#define CR0_DEFAULT (CR0_PG | CR0_PE)

/* "CR4 — Contains a group of flags that enable several architectural extensions, and indicate
 *  operating system or executive support for specific processor capabilities."
 * Consult [1b] for more details
 */
#define CR4_PAE BIT_LOW(5) /* Physical Address Extension */
// @billn do we need CR4.PGE and CR4.MCE as well?

#define CR4_DEFAULT (CR4_PAE)

/* Extended Feature Enable Register: "provides several fields related to IA-32e mode
 *                                    enabling and operation"
 * Note: "IA-32e" is just Intel's name for long mode. 
 * Consult [1c] for more details
 */
#define IA32_EFER_LME (BIT_LOW(8) | BIT_LOW(10)) /* Enable IA-32e mode operation */

#define IA32_EFER_DEFAULT (IA32_EFER_LME)

/* The 32-bit EFLAGS/RFLAGS register contains a group of status flags, a control flag, and a group of system flags.
 * We don't use anything here during boot to mask all IRQs, but the 2nd bit must always be set as 1. See [1d]
 */
#define RFLAGS_DEFAULT BIT(1)

void vcpu_print_regs(size_t vcpu_id);
