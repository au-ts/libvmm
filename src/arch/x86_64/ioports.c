/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

// intel manual
// [1] Table 28-5. Exit Qualification for I/O Instructions

// [1]
typedef enum ioport_access_width_qualification {
    IOPORT_BYTE_ACCESS_QUAL = 0,
    IOPORT_WORD_ACCESS_QUAL = 1, // 2-byte
    IOPORT_DWORD_ACCESS_QUAL = 3, // 4-byte
} ioport_access_width_qualification_t;

bool emulate_ioports(seL4_VCPUContext *vctx, uint64_t f_qualification) {
    uint64_t is_read = f_qualification & BIT(3);
    uint64_t is_string = f_qualification & BIT(4);
    assert(!is_string);
    uint64_t is_immediate = f_qualification & BIT(6);
    uint16_t port_addr = (f_qualification >> 16) & 0xffff;
    ioport_access_width_qualification_t access_width = f_qualification & 0x7;

    bool success = false;

    if (port_addr == 0xCF8) {
        success = true;
    } else if (port_addr >= 0xCFC && port_addr < 0xCFC + 4) {
        if (is_read) {
            // invalid read to simulate no device on pci bus
            vctx->eax = 0xffffffff;
        }
        success = true;
    }

    return success;
}