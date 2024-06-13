/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// Qemu faults if you try load or store with a strict memorder on aarch64.
#ifdef BOARD_qemu_virt_aarch64
#define VMM_NO_ATOMICS
#endif

#ifdef VMM_NO_ATOMICS
#define ATOMIC_LOAD(ptr, memorder) __atomic_load_n(ptr, __ATOMIC_RELAXED)
#define ATOMIC_STORE(ptr, val, memorder) __atomic_store_n(ptr, val, __ATOMIC_RELAXED)
#else
#define ATOMIC_LOAD(ptr, memorder) __atomic_load_n(ptr, memorder)
#define ATOMIC_STORE(ptr, val, memorder) __atomic_store_n(ptr, val, memorder)
#endif
