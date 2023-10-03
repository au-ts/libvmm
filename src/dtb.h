/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>

/*
 * The following definitions are from v0.3, Section 5.2 of the Devicetree
 * Specification.
 */

/* Note that this is the little-endian representation, the specification
 * mentions only the big-endian representation. */
#define DTB_MAGIC 0xEDFE0DD0

struct dtb_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

bool dtb_check_magic(struct dtb_header *h) {
    return h->magic == DTB_MAGIC;
}
