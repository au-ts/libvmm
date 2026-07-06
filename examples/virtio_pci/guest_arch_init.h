/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* virtio_pci example specific helpers to hide architecture specific details from
 * the high level VMM source file. */
bool guest_arch_init(void);

bool virtio_arch_init(void);

bool guest_arch_start(void);