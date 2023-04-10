/*
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <sel4cp.h>
#include "printf.h"

// @ivanv: these are here for convience, should not be here though
#define GUEST_ID 0
#define GUEST_VCPU_ID 0
#define GUEST_NUM_VCPUS 1
// Note that this is AArch64 specific
#if defined(CONFIG_ARCH_AARCH64)
    #define SEL4_USER_CONTEXT_SIZE 0x24
#endif

#define PAGE_SIZE_4K 4096

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#define CTZ(x) __builtin_ctz(x)

#if __STDC_VERSION__ >= 201112L && !defined(__cplusplus)
#define static_assert _Static_assert
#endif

//  __attribute__ ((__noreturn__))
// void __assert_func(const char *file, int line, const char *function, const char *str);

void _putchar(char character);

#define LOG_VMM(...) do{ printf("%s|INFO: ", sel4cp_name); printf(__VA_ARGS__); }while(0)
#define LOG_VMM_ERR(...) do{ printf("%s|ERROR: ", sel4cp_name); printf(__VA_ARGS__); }while(0)

static char
decchar(unsigned int v) {
    return '0' + v;
}

static void
put8(uint8_t x)
{
    char tmp[4];
    unsigned i = 3;
    tmp[3] = 0;
    do {
        uint8_t c = x % 10;
        tmp[--i] = decchar(c);
        x /= 10;
    } while (x);
    sel4cp_dbg_puts(&tmp[i]);
}

// @ivanv: sort this out...
static void
reply_to_fault()
{
    sel4cp_msginfo msg = sel4cp_msginfo_new(0, 0);
    seL4_Send(4, msg);
}

static uint64_t get_vmm_id(char *sel4cp_name)
{
    // @ivanv: Absolute hack
    return sel4cp_name[4] - '0';
}

static void
print_tcb_regs(seL4_UserContext *ctx) {
#if defined(ARCH_aarch64)
    // I don't know if it's the best idea, but here we'll just dump the
    // registers in the same order they are defined in seL4_UserContext
    printf("VMM|INFO: TCB registers:\n");
    // Frame registers
    printf("    pc:   0x%016lx\n", ctx->pc);
    printf("    sp:   0x%016lx\n", ctx->sp);
    printf("    spsr: 0x%016lx\n", ctx->spsr);
    printf("    x0:   0x%016lx\n", ctx->x0);
    printf("    x1:   0x%016lx\n", ctx->x1);
    printf("    x2:   0x%016lx\n", ctx->x2);
    printf("    x3:   0x%016lx\n", ctx->x3);
    printf("    x4:   0x%016lx\n", ctx->x4);
    printf("    x5:   0x%016lx\n", ctx->x5);
    printf("    x6:   0x%016lx\n", ctx->x6);
    printf("    x7:   0x%016lx\n", ctx->x7);
    printf("    x8:   0x%016lx\n", ctx->x8);
    printf("    x16:  0x%016lx\n", ctx->x16);
    printf("    x17:  0x%016lx\n", ctx->x17);
    printf("    x18:  0x%016lx\n", ctx->x18);
    printf("    x29:  0x%016lx\n", ctx->x29);
    printf("    x30:  0x%016lx\n", ctx->x30);
    // Other integer registers
    printf("    x9:   0x%016lx\n", ctx->x9);
    printf("    x10:  0x%016lx\n", ctx->x10);
    printf("    x11:  0x%016lx\n", ctx->x11);
    printf("    x12:  0x%016lx\n", ctx->x12);
    printf("    x13:  0x%016lx\n", ctx->x13);
    printf("    x14:  0x%016lx\n", ctx->x14);
    printf("    x15:  0x%016lx\n", ctx->x15);
    printf("    x19:  0x%016lx\n", ctx->x19);
    printf("    x20:  0x%016lx\n", ctx->x20);
    printf("    x21:  0x%016lx\n", ctx->x21);
    printf("    x22:  0x%016lx\n", ctx->x22);
    printf("    x23:  0x%016lx\n", ctx->x23);
    printf("    x24:  0x%016lx\n", ctx->x24);
    printf("    x25:  0x%016lx\n", ctx->x25);
    printf("    x26:  0x%016lx\n", ctx->x26);
    printf("    x27:  0x%016lx\n", ctx->x27);
    printf("    x28:  0x%016lx\n", ctx->x28);
    // TODO(ivanv): print out thread ID registers?
#endif
}

// @ivanv: this should have the same foramtting as the TCB registers
static void
print_vcpu_regs(uint64_t vcpu_id) {
    printf("VMM|INFO: VCPU registers: \n");
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

static void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (; n; n--) *d++ = *s++;
    return dest;
}

static void *memset(void *dest, int c, size_t n)
{
    unsigned char *s = dest;
    for (; n; n--, s++) *s = c;
    return dest;
}

static void assert_fail(
    const char  *assertion,
    const char  *file,
    unsigned int line,
    const char  *function)
{
    printf("Failed assertion '");
    printf(assertion);
    printf("' at ");
    printf(file);
    printf(":");
    put8(line);
    printf(" in function ");
    printf(function);
    printf("\n");
    while (1) {}
}

#define assert(expr) \
    do { \
        if (!(expr)) { \
            assert_fail(#expr, __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

