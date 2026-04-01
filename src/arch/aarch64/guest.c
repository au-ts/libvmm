/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/guest.h>

bool gpa_to_vaddr(uint64_t gpa, void **ret, int *bytes_remaining)
{
    /* On ARM we make have 1 to 1 mapping so nothing to do. */
    *bytes_remaining = 0;
    *ret = (void *)gpa;
    return true;
}
