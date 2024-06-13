/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>

void zig_arm_sys_send(seL4_Word sys, seL4_Word dest, seL4_Word info_arg, seL4_Word mr0, seL4_Word mr1,
                                seL4_Word mr2, seL4_Word mr3);
void zig_arm_sys_send_recv(seL4_Word sys, seL4_Word dest, seL4_Word *out_badge, seL4_Word info_arg,
                                     seL4_Word *out_info, seL4_Word *in_out_mr0, seL4_Word *in_out_mr1, seL4_Word *in_out_mr2, seL4_Word *in_out_mr3,
                                     LIBSEL4_UNUSED seL4_Word reply);
