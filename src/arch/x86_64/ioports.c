/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/pit.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/qemu_fw_cfg.h>
#include <libvmm/arch/x86_64/cmos.h>

extern uint64_t primary_ata_cmd_pio_id;
extern uint64_t primary_ata_cmd_pio_addr;

extern uint64_t primary_ata_ctrl_pio_id;
extern uint64_t primary_ata_ctrl_pio_addr;

extern uint64_t second_ata_cmd_pio_id;
extern uint64_t second_ata_cmd_pio_addr;

extern uint64_t second_ata_ctrl_pio_id;
extern uint64_t second_ata_ctrl_pio_addr;

extern struct pci_bus pci_bus_state;

// TODO: hack
#define TIMER_DRV_CH_FOR_LAPIC 11

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

#define ACPI_PMT_FREQUENCY (3579545)

int emulate_ioport_string_read(seL4_VCPUContext *vctx, char *data, size_t data_len, bool is_rep, ioport_access_width_t access_width) {
    int data_index = 0;
    int max_iterations = 1;
    uint64_t eflags = microkit_vcpu_x86_read_vmcs(0, VMX_GUEST_RFLAGS);

    if (is_rep) {
        max_iterations = vctx->ecx;
    }

    int iteration = 0;
    for (; iteration < max_iterations && data_index < data_len; iteration++) {
        uint64_t dest_gpa;
        int bytes_to_page_boundary;
        assert(gva_to_gpa(0, vctx->edi, &dest_gpa, &bytes_to_page_boundary));
        char *dest = gpa_to_vaddr(dest_gpa);

        assert(bytes_to_page_boundary >= ioports_access_width_to_bytes(access_width));

        memcpy(dest, &data[data_index], ioports_access_width_to_bytes(access_width));

        if (eflags & BIT(10)) {
            vctx->edi -= ioports_access_width_to_bytes(access_width);
        } else {
            vctx->edi += ioports_access_width_to_bytes(access_width);
        }
        data_index += ioports_access_width_to_bytes(access_width);
    }

    return ioports_access_width_to_bytes(access_width) * data_index;
}

bool emulate_ioports(seL4_VCPUContext *vctx, uint64_t f_qualification)
{
    uint64_t is_read = f_qualification & BIT(3);
    uint64_t is_string = f_qualification & BIT(4);
    uint64_t is_rep = f_qualification & BIT(5);
    uint16_t port_addr = (f_qualification >> 16) & 0xffff;
    ioport_access_width_t access_width = (ioport_access_width_t)(f_qualification & 0x7);

    bool success = false;

    if (is_read) {
        // prime the result
        vctx->eax = 0;
    }

    if (is_pci_config_space_access_mech_1(port_addr)) {
        assert(!is_string);
        return emulate_pci_config_space_access_mech_1(vctx, port_addr, is_read, access_width);

    // } else if (ata_controller_access_pio_ch(port_addr) != -1) {
    //     return emulate_port_access(vctx, port_addr, ata_controller_access_pio_ch(port_addr), is_read, access_width);

    } else if (port_addr >= 0x400 && port_addr < 0x406) {
        assert(!is_string);
        success = true;

    } else if (port_addr >= 0xC000 && port_addr < 0xCFFF) {
        assert(!is_string);
        if (is_read) {
            // invalid read to simulate no device on pci bus
            vctx->eax = 0xffffffff;
        }
        success = true;
    } else if (port_addr == 0xA0 || port_addr == 0xA1 || port_addr == 0x20 || port_addr == 0x21 || port_addr == 0x4d1
               || port_addr == 0x4d0) {
        assert(!is_string);
        // PIC1/2 access
        if (is_read) {
            // invalid read
            vctx->eax = 0xffffffff;
        }
        success = true;
    } else if (port_addr == 0x70 || port_addr == 0x71) {
        // cmos
        assert(!is_string);
        success = emulate_cmos_access(vctx, port_addr, is_read, access_width);
    } else if (port_addr == 0x80) {
        assert(!is_string);
        // io port access delay, no-op
        if (!is_read) {
            success = true;
        }
    } else if (port_addr == 0x61) {
        assert(!is_string);
        // some sort of PS2 controller?
        success = true;
    // } else if (port_addr >= 0x40 && port_addr <= 0x43) {
    //     return emulate_pit(vctx, port_addr, is_read);

    } else if (port_addr == 0x87) {
        assert(!is_string);
        // dma controller
        success = true;

    } else if (port_addr == 0x2f9) {
        assert(!is_string);
        // parallel port
        success = true;

    } else if (port_addr == 0x3e9 || port_addr == 0x2e9) {
        assert(!is_string);
        // some sort of serial device
        success = true;

    } else if (port_addr == 0x64) {
        assert(!is_string);
        // PS2 controller
        success = true;
    } else if (port_addr == pci_bus_state.isa_bridge_power_mgmt_regs.pmba + 0x8) {
        assert(!is_string);
        // Handle ACPI Power Management Timer
        // 7.2.4 of 82371AB PCI-TO-ISA / IDE XCELERATOR (PIIX4)
        // TODO: maybe handle PCI reset case
        uint64_t timer_ns = sddf_timer_time_now(TIMER_DRV_CH_FOR_LAPIC);
        vctx->eax = (uint64_t)(((double)timer_ns / (double)NS_IN_S) * ACPI_PMT_FREQUENCY);
        success = true;
    } else if (port_addr == 0x510 || port_addr == 0x511 || port_addr == 0x514) {
        success = emulate_qemu_fw_cfg(vctx, port_addr, is_read, is_string, is_rep, access_width);
    } else if (port_addr >= 0xAF00 || port_addr <= 0xaf00 + 12) {
        /* See https://www.qemu.org/docs/master/specs/acpi_cpu_hotplug.html for details.
         * Basically we want to emulate QEMU_CPUHP_R_CMD_DATA2 so that the guest does not try to
         * use modern CPU hot plugging functionality which invovles more emulation.
         */
        if (port_addr == 0xAF00) {
            vctx->eax = 0x1;
        }
        success = true;
    } else {
        LOG_VMM_ERR("unhandled io port 0x%x\n", port_addr);
    }

    return success;
}