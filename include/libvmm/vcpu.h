/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <microkit.h>

typedef enum {
    VCPU_RUNNING = 0,
    VCPU_WFI_ON_ARCH_TIMER,
    VCPU_WFI_ON_OTHER_IRQ,
    VCPU_NUM_STATES
} vcpu_state_t;

typedef struct {
    vcpu_state_t vcpu_state;
} vcpu_data_t;

void vcpu_set_state(size_t vcpu_id, vcpu_state_t value);
vcpu_state_t vcpu_get_state(size_t vcpu_id);

void vcpu_pause(size_t vcpu_id);
void vcpu_resume(size_t vcpu_id);

void vcpu_reset(size_t vcpu_id);
void vcpu_print_regs(size_t vcpu_id);
