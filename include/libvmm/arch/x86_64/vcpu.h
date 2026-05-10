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
#define CR0_PG BIT(31) /* Paging On */
#define CR0_PE BIT(0)  /* Protection Enable */

#define CR0_DEFAULT (CR0_PG | CR0_PE)

/* "CR4 — Contains a group of flags that enable several architectural extensions, and indicate
 *  operating system or executive support for specific processor capabilities."
 * Consult [1b] for more details
 */
#define CR4_PAE     BIT(5)  /* Physical Address Extension */
#define CR4_OSFXSR  BIT(9)  /* Operating system support for FXSAVE and FXRSTOR instructions */
#define CR4_VMXE    BIT(13) /* Virtual Machine Extensions Enable */
#define CR4_OSXSAVE BIT(18) /* XSAVE and Processor Extended States Enable */

#define CR4_DEFAULT (CR4_PAE | CR4_OSFXSR | CR4_OSXSAVE)
#define CR4_EN_MASK (CR4_VMXE)

/* Extended Feature Enable Register: "provides several fields related to IA-32e mode
 *                                    enabling and operation"
 * Note: "IA-32e" is just Intel's name for long mode.
 * Consult [1c] for more details
 */
#define IA32_EFER_LME (BIT(8) | BIT(10)) /* Enable IA-32e mode operation */

/* Long mode default */
#define IA32_EFER_LM_DEFAULT (IA32_EFER_LME)

/* The 32-bit EFLAGS/RFLAGS register contains a group of status flags, a control flag, and a group of system flags.
 * We don't use anything here during boot to mask all IRQs, but the 2nd bit must always be set as 1. See [1d]
 */
#define RFLAGS_DEFAULT BIT(1)

bool vcpu_set_up_long_mode(uint64_t cr3, uint64_t gdt_gpa, uint64_t gdt_limit);

/* Keep track of the VCPU state when a VM Exit happens. */
void vcpu_init_exit_state(bool exit_from_ntfn);
uint64_t vcpu_exit_get_rip(void);
void vcpu_exit_update_ppvc(uint64_t ppvc);
uint64_t vcpu_exit_get_irq(void);
void vcpu_exit_inject_irq(uint64_t irq);
uint64_t vcpu_exit_get_reason(void);
uint64_t vcpu_exit_get_qualification(void);
uint64_t vcpu_exit_get_instruction_len(void);
uint64_t vcpu_exit_get_rflags(void);
uint64_t vcpu_exit_get_interruptability(void);
uint64_t vcpu_exit_get_cr3(void);
seL4_VCPUContext *vcpu_exit_get_context(void);
void vcpu_exit_advance_rip(unsigned rip_additive);
void vcpu_exit_resume(void);

void vcpu_print_regs(size_t vcpu_id);
