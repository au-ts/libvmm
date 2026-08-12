/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sddf/network/constants.h>

#ifdef NETWORK_HW_HAS_CHECKSUM
#define HAVE_CSUM_OFFLOAD (true)
#else
#define HAVE_CSUM_OFFLOAD (false)
#endif

/* virtio_pci example specific helpers to hide architecture specific details from
 * the high level VMM source file. */
bool guest_arch_init(void);

bool virtio_arch_init(void);

bool guest_arch_start(void);