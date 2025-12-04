/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <libvmm/arch/x86_64/pit.h>

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

    if (is_read) {
        // prime the result
        vctx->eax = 0;
    }

    if (port_addr >= 0xCF8 && port_addr < 0xCF8 + 4) {
        success = true;
    } else if (port_addr >= 0xCFC && port_addr < 0xCFC + 4) {
        if (is_read) {
            // invalid read to simulate no device on pci bus
            vctx->eax = 0xffffffff;
        }
        success = true;
    } else if (port_addr >= 0xC000 && port_addr < 0xCFFF) {
        if (is_read) {
            // invalid read to simulate no device on pci bus
            vctx->eax = 0xffffffff;
        }
        success = true;
    } else if (port_addr == 0xA0 || port_addr == 0xA1 || port_addr == 0x20 || port_addr == 0x21 || port_addr == 0x4d1 || port_addr == 0x4d0) {
        // PIC1/2 access
        if (is_read) {
            // invalid read
            vctx->eax = 0xffffffff;
        }
        success = true;
    } else if (port_addr == 0x70) {
        // cmos select
        assert(!is_read);
        success = true;
    } else if (port_addr == 0x71) {
        // cmos data
        // @billn i feel like we should do this properly
        // assert(is_read);
        vctx->eax = 0;
        success = true;
    } else if (port_addr == 0x80) {
        // io port access delay, no-op
        if (!is_read) {
            success = true;
        }
    } else if (port_addr == 0x61) {
        // some sort of PS2 controller?
        success = true;
    // } else if (port_addr >= 0x40 && port_addr <= 0x43) {
    //     return emulate_pit(vctx, port_addr, is_read);

    } else if (port_addr == 0x87) {
        // dma controller
        success = true;

    } else if (port_addr == 0x2f9) {
        // parallel port
        success = true;

    } else if (port_addr == 0x3e9 || port_addr == 0x2e9) {
        // some sort of serial device
        success = true;

    } else if (port_addr == 0x64) {
        // PS2 controller
        success = true;

    } else {
        LOG_VMM_ERR("unhandled io port 0x%x\n", port_addr);
    }

    return success;
}