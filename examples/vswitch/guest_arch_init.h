/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* We always want the guest to give us fully checksummed packets. So that when the VSwitch
 * forwards it to another guest, the checksum at the other end is valid. */
#define HAVE_CSUM_OFFLOAD (false)

/* vswitch example specific helpers to hide architecture specific details from
 * the high level VMM source file. */
bool guest_arch_init(void);

bool virtio_arch_init(void);

bool guest_arch_start(void);