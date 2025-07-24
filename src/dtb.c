/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <libvmm/dtb.h>
#include <libvmm/util/util.h>

bool dtb_check_magic(char *bytes)
{
    struct dtb_header header = {0};
    memcpy((char *)&header, bytes, sizeof(struct dtb_header));

    return header.magic == DTB_MAGIC;
}
