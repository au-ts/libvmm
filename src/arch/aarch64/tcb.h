/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <sel4cp.h>

// @ivanv: should we have a header-only file or have tcb.c and vcpu.c as well?
static void tcb_print_regs(seL4_UserContext *ctx) {
    // I don't know if it's the best idea, but here we'll just dump the
    // registers in the same order they are defined in seL4_UserContext
    LOG_VMM("TCB registers:\n");
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
    // @ivanv print out thread ID registers?
}
