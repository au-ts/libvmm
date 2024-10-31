/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

/* Notify the VMM by writing into a registered
 * memory region thereby invoking a hyp trap.
 */
void vmm_notify();

/* Writing 1 to the UIO device ACKs the IRQ and
 * also re-enables the interrupt.
 */
void uio_irq_ack_and_enable();

/* Drivers can add their own events to the uio event loop */
void bind_fd_to_epoll(int fd);
