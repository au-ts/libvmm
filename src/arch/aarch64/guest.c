/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/guest.h>

void *gpa_to_vaddr(uint64_t gpa)
{
    return (void *)gpa;
}
