/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/vmm_state.h>

struct local_vmm_state local_vmm_state;
struct global_vmm_state *global_vmm_state;

bool initialise_local_and_global_vmm_state(bool is_bsp, void *global_vmm_state_ptr)
{
    if (!global_vmm_state_ptr) {
        LOG_VMM_ERR("global_vmm_state_ptr is NULL!\n");
        return false;
    }

    global_vmm_state = global_vmm_state_ptr;
    if (is_bsp) {
        memset(global_vmm_state, 0, sizeof(struct global_vmm_state));
    }

    memset(&local_vmm_state, 0, sizeof(struct local_vmm_state));

    return true;
}

struct global_read_only_vmm_state *get_global_vmm_mutable_state(void)
{
    return &global_vmm_state->read_only;
}

struct global_mutable_vmm_state *acquire_global_vmm_mutable_state(void)
{
    // @billn locking!, maybe https://preshing.com/20120226/roll-your-own-lightweight-mutex/

    return &global_vmm_state->mutable;
}

void release_global_vmm_mutable_state(void)
{
    // @billn locking!
}