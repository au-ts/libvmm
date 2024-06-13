/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sddf/sound/sound.h>

#define UIO_SND_FAULT_ADDRESS 0x301000

typedef struct vm_shared_state {
    sound_shared_state_t sound;
    uintptr_t data_paddr;
} vm_shared_state_t;
