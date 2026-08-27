/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Implementation of the QEMU ramfb device. Struct definition from
 * https://github.com/tianocore/edk2/blob/edk2-stable202605/OvmfPkg/QemuRamfbDxe/QemuRamfb.c */

#define RAMFB_4CC_XRGB8888 0x34325258

typedef struct __attribute__((packed)) ramfb_config {
    uint64_t address; /* GPA of framebuffer */
    uint32_t four_cc; /* Pixel format, seems like OVMF doesn't care about this and set its own mode */
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    uint32_t stride;  /* width * bytes per pixel */
} ramfb_config_t;