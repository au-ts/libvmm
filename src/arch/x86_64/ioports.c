/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/pit.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/instruction.h>

extern uint64_t primary_ata_cmd_pio_id;
extern uint64_t primary_ata_cmd_pio_addr;

extern uint64_t primary_ata_ctrl_pio_id;
extern uint64_t primary_ata_ctrl_pio_addr;

extern uint64_t second_ata_cmd_pio_id;
extern uint64_t second_ata_cmd_pio_addr;

extern uint64_t second_ata_ctrl_pio_id;
extern uint64_t second_ata_ctrl_pio_addr;

int ioports_access_width_to_bytes(ioport_access_width_t access_width) {
    switch (access_width) {
        case IOPORT_BYTE_ACCESS_QUAL:
            return 1;
        case IOPORT_WORD_ACCESS_QUAL:
            return 2;
        case IOPORT_DWORD_ACCESS_QUAL:
            return 4;
        default:
            return 0;
    }
}

// int ata_controller_access_pio_ch(uint16_t port_addr) {
//     if (port_addr >= primary_ata_cmd_pio_addr && port_addr < primary_ata_cmd_pio_addr + 8) {
//         return primary_ata_cmd_pio_id;
//     }
//     if (port_addr >= primary_ata_ctrl_pio_addr && port_addr < primary_ata_ctrl_pio_addr + 1) {
//         return primary_ata_ctrl_pio_id;
//     }
//     if (port_addr >= second_ata_cmd_pio_addr && port_addr < second_ata_cmd_pio_addr + 8) {
//         return second_ata_cmd_pio_id;
//     }
//     if (port_addr >= second_ata_ctrl_pio_addr && port_addr < second_ata_ctrl_pio_addr + 1) {
//         return second_ata_ctrl_pio_id;
//     }
//     return -1;
// }

// bool emulate_port_access(seL4_VCPUContext *vctx, uint16_t port_addr, int port_id, uint64_t is_read, ioport_access_width_t access_width) {
//     uint64_t *vctx_raw = (uint64_t *) vctx;

//     if (is_read) {
//         switch (access_width) {
//             case IOPORT_BYTE_ACCESS_QUAL:
//                 vctx_raw[RAX_IDX] = microkit_x86_ioport_read_8(port_id, port_addr);
//                 break;
//             case IOPORT_WORD_ACCESS_QUAL:
//                 vctx_raw[RAX_IDX] = microkit_x86_ioport_read_16(port_id, port_addr);
//                 break;
//             case IOPORT_DWORD_ACCESS_QUAL:
//                 vctx_raw[RAX_IDX] = microkit_x86_ioport_read_32(port_id, port_addr);
//                 break;
//         }
//     } else {
//         switch (access_width) {
//             case IOPORT_BYTE_ACCESS_QUAL:
//                 microkit_x86_ioport_write_8(port_id, port_addr, vctx_raw[RAX_IDX]);
//                 break;
//             case IOPORT_WORD_ACCESS_QUAL:
//                 microkit_x86_ioport_write_16(port_id, port_addr, vctx_raw[RAX_IDX]);
//                 break;
//             case IOPORT_DWORD_ACCESS_QUAL:
//                 microkit_x86_ioport_write_32(port_id, port_addr, vctx_raw[RAX_IDX]);
//                 break;
//         }
//     }

//     return true;
// }

bool emulate_ioports(seL4_VCPUContext *vctx, uint64_t f_qualification)
{
    uint64_t is_read = f_qualification & BIT(3);
    uint64_t is_string = f_qualification & BIT(4);
    assert(!is_string); // unsupported right now
    uint64_t is_immediate = f_qualification & BIT(6);
    uint16_t port_addr = (f_qualification >> 16) & 0xffff;
    ioport_access_width_t access_width = (ioport_access_width_t)(f_qualification & 0x7);

    bool success = false;

    if (is_read) {
        // prime the result
        vctx->eax = 0;
    }

    if (is_pci_config_space_access_mech_1(port_addr)) {
        return emulate_pci_config_space_access_mech_1(vctx, port_addr, is_read, access_width);

    // } else if (ata_controller_access_pio_ch(port_addr) != -1) {
    //     return emulate_port_access(vctx, port_addr, ata_controller_access_pio_ch(port_addr), is_read, access_width);

    } else if (port_addr >= 0xC000 && port_addr < 0xCFFF) {
        if (is_read) {
            // invalid read to simulate no device on pci bus
            vctx->eax = 0xffffffff;
        }
        success = true;
    } else if (port_addr == 0xA0 || port_addr == 0xA1 || port_addr == 0x20 || port_addr == 0x21 || port_addr == 0x4d1
               || port_addr == 0x4d0) {
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