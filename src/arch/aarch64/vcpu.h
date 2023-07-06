/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <sel4cp.h>

#define SCTLR_EL1_UCI       (1 << 26)     /* Enable EL0 access to DC CVAU, DC CIVAC, DC CVAC,
                                           and IC IVAU in AArch64 state   */
#define SCTLR_EL1_C         (1 << 2)      /* Enable data and unified caches */
#define SCTLR_EL1_I         (1 << 12)     /* Enable instruction cache       */
#define SCTLR_EL1_CP15BEN   (1 << 5)      /* AArch32 CP15 barrier enable    */
#define SCTLR_EL1_UTC       (1 << 15)     /* Enable EL0 access to CTR_EL0   */
#define SCTLR_EL1_NTWI      (1 << 16)     /* WFI executed as normal         */
#define SCTLR_EL1_NTWE      (1 << 18)     /* WFE executed as normal         */

/* Disable MMU, SP alignment check, and alignment check */
/* A57 default value */
#define SCTLR_EL1_RES      0x30d00800   /* Reserved value */
#define SCTLR_EL1          ( SCTLR_EL1_RES | SCTLR_EL1_CP15BEN | SCTLR_EL1_UTC \
                           | SCTLR_EL1_NTWI | SCTLR_EL1_NTWE )
#define SCTLR_EL1_NATIVE   (SCTLR_EL1 | SCTLR_EL1_C | SCTLR_EL1_I | SCTLR_EL1_UCI)
#define SCTLR_DEFAULT      SCTLR_EL1_NATIVE

static void vcpu_reset(size_t vcpu_id) {
    // Reset registers
    // @ivanv: double check, shouldn't we be setting sctlr?
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_SCTLR, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_TTBR0, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_TTBR1, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_TCR, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_MAIR, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_AMAIR, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_CIDR, 0);
    /* other system registers EL1 */
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_ACTLR, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_CPACR, 0);
    /* exception handling registers EL1 */
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_AFSR0, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_AFSR1, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_ESR, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_FAR, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_ISR, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_VBAR, 0);
    /* thread pointer/ID registers EL0/EL1 */
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_TPIDR_EL1, 0);
#if CONFIG_MAX_NUM_NODES > 1
    /* Virtualisation Multiprocessor ID Register */
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_VMPIDR_EL2, 0);
#endif /* CONFIG_MAX_NUM_NODES > 1 */
    /* general registers x0 to x30 have been saved by traps.S */
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_SP_EL1, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_ELR_EL1, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_SPSR_EL1, 0); // 32-bit
    /* generic timer registers, to be completed */
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_CNTV_CTL, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_CNTV_CVAL, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_CNTVOFF, 0);
    sel4cp_arm_vcpu_write_reg(vcpu_id, seL4_VCPUReg_CNTKCTL_EL1, 0);
}

// @ivanv: this should have the same foramtting as the TCB registers
static void vcpu_print_regs(size_t vcpu_id) {
    LOG_VMM("VCPU registers:\n");
    /* VM control registers EL1 */
    printf("    SCTLR: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_SCTLR));
    printf("    TTBR0: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_TTBR0));
    printf("    TTBR1: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_TTBR1));
    printf("    TCR:   0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_TCR));
    printf("    MAIR:  0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_MAIR));
    printf("    AMAIR: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_AMAIR));
    printf("    CIDR:  0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_CIDR));
    /* other system registers EL1 */
    printf("    ACTLR: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_ACTLR));
    printf("    CPACR: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_CPACR));
    /* exception handling registers EL1 */
    printf("    AFSR0: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_AFSR0));
    printf("    AFSR1: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_AFSR1));
    printf("    ESR:   0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_ESR));
    printf("    FAR:   0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_FAR));
    printf("    ISR:   0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_ISR));
    printf("    VBAR:  0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_VBAR));
    /* thread pointer/ID registers EL0/EL1 */
    printf("    TPIDR_EL1: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_TPIDR_EL1));
    // @ivanv: I think thins might not be the correct ifdef
#if CONFIG_MAX_NUM_NODES > 1
    /* Virtualisation Multiprocessor ID Register */
    printf("    VMPIDR_EL2: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_VMPIDR_EL2));
#endif
    /* general registers x0 to x30 have been saved by traps.S */
    printf("    SP_EL1: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_SP_EL1));
    printf("    ELR_EL1: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_ELR_EL1));
    printf("    SPSR_EL1: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_SPSR_EL1)); // 32-bit // @ivanv what
    /* generic timer registers, to be completed */
    printf("    CNTV_CTL: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_CNTV_CTL));
    printf("    CNTV_CVAL: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_CNTV_CVAL));
    printf("    CNTVOFF: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_CNTVOFF));
    printf("    CNTKCTL_EL1: 0x%lx\n", sel4cp_arm_vcpu_read_reg(vcpu_id, seL4_VCPUReg_CNTKCTL_EL1));
}
