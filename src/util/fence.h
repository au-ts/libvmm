/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* Prevent the compiler from re-ordering any read or write across the fence. */
#define COMPILER_MEMORY_FENCE() __atomic_signal_fence(__ATOMIC_ACQ_REL)

/* Prevent the compiler from re-ordering any write which follows the fence
 * in program order with any read or write which preceeds the fence in
 * program order. */
#define COMPILER_MEMORY_RELEASE() __atomic_signal_fence(__ATOMIC_RELEASE)

/* Prevent the compiler from re-ordering any read which preceeds the fence
 * in program order with any read or write which follows the fence in
 * program order. */
#define COMPILER_MEMORY_ACQUIRE() __atomic_signal_fence(__ATOMIC_ACQUIRE)

/* THREAD_MEMORY_FENCE: Implements a full processor memory barrier.
 * All stores before this point are completed, and all loads after this
 * point are delayed until after it.
 */
#define THREAD_MEMORY_FENCE() __atomic_thread_fence(__ATOMIC_ACQ_REL)

/* THREAD_MEMORY_RELEASE: Implements a fence which has the effect of
 * forcing all stores before this point to complete.
 */
#define THREAD_MEMORY_RELEASE() __atomic_thread_fence(__ATOMIC_RELEASE)

/* THREAD_MEMORY_ACQUIRE: Implements a fence which has the effect of
 * forcing all loads beyond this point to occur after this point.
 */
#define THREAD_MEMORY_ACQUIRE() __atomic_thread_fence(__ATOMIC_ACQUIRE)
