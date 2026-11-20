/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifdef NETWORK_HW_HAS_CHECKSUM
#define HAVE_CSUM_OFFLOAD (true)
#else
#define HAVE_CSUM_OFFLOAD (false)
#endif

#define SERIAL_IRQ_CH 51

/* Windows example specific helpers to hide architecture specific details from
 * the high level VMM source file. */
bool guest_arch_init(void);

bool virtio_arch_init(void);

bool guest_arch_start(void);