/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Zig will automatically translate microkit.h when it is imported into vmm.zig.
 * This is great, but the C translation functionality has one limitation that involves
 * us doing some extra work. Zig does not auto-translate GCC inline assembly which is
 * used by libsel4. This means that we need to provide certain functions where the implementation
 * is not in the header. Our implementations are just copies of the AArch64 libsel4 ones. */

#define arm_sys_send arm_sys_send_header
#define arm_sys_send_recv arm_sys_send_recv_header

#include <microkit.h>

/* Ignore the implementations of the header so we do not get duplicate function errors. */
#undef arm_sys_send
#undef arm_sys_send_recv

void arm_sys_send(seL4_Word sys, seL4_Word dest, seL4_Word info_arg, seL4_Word mr0, seL4_Word mr1,
                                seL4_Word mr2, seL4_Word mr3)
{
    register seL4_Word destptr asm("x0") = dest;
    register seL4_Word info asm("x1") = info_arg;

    /* Load beginning of the message into registers. */
    register seL4_Word msg0 asm("x2") = mr0;
    register seL4_Word msg1 asm("x3") = mr1;
    register seL4_Word msg2 asm("x4") = mr2;
    register seL4_Word msg3 asm("x5") = mr3;

    /* Perform the system call. */
    register seL4_Word scno asm("x7") = sys;
    asm volatile(
        "svc #0"
        : "+r"(destptr), "+r"(msg0), "+r"(msg1), "+r"(msg2),
        "+r"(msg3), "+r"(info)
        : "r"(scno)
    );
}

void arm_sys_send_recv(seL4_Word sys, seL4_Word dest, seL4_Word *out_badge, seL4_Word info_arg,
                                     seL4_Word *out_info, seL4_Word *in_out_mr0, seL4_Word *in_out_mr1, seL4_Word *in_out_mr2, seL4_Word *in_out_mr3,
                                     LIBSEL4_UNUSED seL4_Word reply)
{
    register seL4_Word destptr asm("x0") = dest;
    register seL4_Word info asm("x1") = info_arg;

    /* Load beginning of the message into registers. */
    register seL4_Word msg0 asm("x2") = *in_out_mr0;
    register seL4_Word msg1 asm("x3") = *in_out_mr1;
    register seL4_Word msg2 asm("x4") = *in_out_mr2;
    register seL4_Word msg3 asm("x5") = *in_out_mr3;
    MCS_PARAM_DECL("x6");

    /* Perform the system call. */
    register seL4_Word scno asm("x7") = sys;
    asm volatile(
        "svc #0"
        : "+r"(msg0), "+r"(msg1), "+r"(msg2), "+r"(msg3),
        "+r"(info), "+r"(destptr)
        : "r"(scno) MCS_PARAM
        : "memory"
    );
    *out_info = info;
    *out_badge = destptr;
    *in_out_mr0 = msg0;
    *in_out_mr1 = msg1;
    *in_out_mr2 = msg2;
    *in_out_mr3 = msg3;
}
