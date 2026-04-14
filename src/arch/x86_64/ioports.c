/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/guest.h>

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-088US June 2025
 */

/* Table 29-5. "Exit Qualification for I/O Instructions" */
#define PIO_VIOLATION_READ_DIRECTION_BIT BIT(3)
#define PIO_VIOLATION_STRING_OP_BIT BIT(4)
#define PIO_VIOLATION_ADDR_SHIFT 16
#define PIO_VIOLATION_ADDR_MASK 0xffff
#define PIO_VIOLATION_ACC_WIDTH_MASK 0x7

typedef enum ioport_access_width_qualification {
    IOPORT_BYTE_ACCESS_QUAL = 0,
    IOPORT_WORD_ACCESS_QUAL = 1, // 2-byte
    IOPORT_DWORD_ACCESS_QUAL = 3, // 4-byte
} ioport_access_width_t;

int pio_fault_to_access_width_bytes(seL4_Word qualification)
{
    switch (qualification & PIO_VIOLATION_ACC_WIDTH_MASK) {
    case IOPORT_BYTE_ACCESS_QUAL:
        return 1;
    case IOPORT_WORD_ACCESS_QUAL:
        return 2;
    case IOPORT_DWORD_ACCESS_QUAL:
        return 4;
    default:
        /* Hardware bug or wrong type of qualification */
        assert(false);
        return 0;
    }
}

bool pio_fault_is_read(seL4_Word qualification)
{
    return !!(qualification & PIO_VIOLATION_READ_DIRECTION_BIT);
}

bool pio_fault_is_write(seL4_Word qualification)
{
    return !pio_fault_is_read(qualification);
}

bool pio_fault_is_string_op(seL4_Word qualification)
{
    return !!(qualification & PIO_VIOLATION_STRING_OP_BIT);
}

uint16_t pio_fault_addr(seL4_Word qualification)
{
    return (qualification >> PIO_VIOLATION_ADDR_SHIFT) & PIO_VIOLATION_ADDR_MASK;
}

bool emulate_ioports(seL4_VCPUContext *vctx, uint64_t f_qualification)
{
    uint64_t is_read = pio_fault_is_read(f_qualification);
    uint64_t is_string = pio_fault_is_string_op(f_qualification);
    uint16_t port_addr = pio_fault_addr(f_qualification);

    assert(!is_string);

    bool success = false;

    if (is_read) {
        // prime the result
        vctx->eax = 0;
    }

    if (port_addr >= 0xC000 && port_addr < 0xCFFF) {
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
    } else if (port_addr == 0x70 || port_addr == 0x71) {
        // cmos
        success = true;
    } else if (port_addr == 0x80) {
        // io port access delay, no-op
        if (!is_read) {
            success = true;
        }
    } else if (port_addr == 0x3f2) {
        // floppy disk, seems sus, when booting memtest in nixos, it will write IRQ enable to this register and hangs...
        success = true;

    } else if (port_addr == 0x87 || (port_addr >= 0 && port_addr <= 0x1f)) {
        // dma controller
        success = true;

    } else if (port_addr == 0x2f9) {
        // parallel port
        success = true;

    } else if (port_addr == 0x3e9 || port_addr == 0x2e9) {
        // some sort of serial device
        success = true;

    } else if (port_addr >= 0x60 && port_addr <= 0x64) {
        // PS2 controller
        success = true;
    } else if (port_addr >= 0xAF00 && port_addr <= 0xaf00 + 12) {
        /* See https://www.qemu.org/docs/master/specs/acpi_cpu_hotplug.html for details.
         * Basically we want to emulate QEMU_CPUHP_R_CMD_DATA2 so that the guest does not try to
         * use modern CPU hot plugging functionality which invovles more emulation.
         */
        if (port_addr == 0xAF00) {
            vctx->eax = 0x1;
        }
        success = true;
    } else if (port_addr == 0x92) {
        // TODO: handle properly, I don't understand why UEFI is touching A20 gate register
        success = true;
    } else if (port_addr == 0xb004) {
        vctx->eax = 0;
        success = true;
    } else if (port_addr == 0x5658 || port_addr == 0x5659) {
        // vmware backdoor
        // https://wiki.osdev.org/VMware_tools
        success = true;
    } else if (port_addr == 0x4e || port_addr == 0x4f || port_addr == 0x2e || port_addr == 0x2f
               || (port_addr >= 0xc80 && port_addr <= 0xc84) || (port_addr >= 0x1c80 && port_addr <= 0x1c84)
               || (port_addr >= 0x2c80 && port_addr <= 0x2c84) || (port_addr >= 0x3c80 && port_addr <= 0x3c84)
               || (port_addr >= 0x4c80 && port_addr <= 0x4c84) || (port_addr >= 0x5c80 && port_addr <= 0x5c84)
               || (port_addr >= 0x6c80 && port_addr <= 0x6c84) || (port_addr >= 0x7c80 && port_addr <= 0x7c84)
               || (port_addr >= 0x8c80 && port_addr <= 0x8c84) || (port_addr >= 0x9c80 && port_addr <= 0x9c84)
               || (port_addr >= 0xac80 && port_addr <= 0xac84) || (port_addr >= 0xbc80 && port_addr <= 0xbc84)
               || (port_addr >= 0xcc80 && port_addr <= 0xcc84) || (port_addr >= 0xdc80 && port_addr <= 0xdc84)
               || (port_addr >= 0xec80 && port_addr <= 0xec84) || (port_addr >= 0xfc80 && port_addr <= 0xfc84)) {
        if (is_read) {
            vctx->eax = 0;
        }
        success = true;
    } else if (port_addr == 0x3BE || port_addr == 0x7BE || port_addr == 0x3BD || port_addr == 0x3BC
               || port_addr == 0x37A || port_addr == 0x77A || port_addr == 0x379 || port_addr == 0x378
               || port_addr == 0x27A || port_addr == 0x67a || port_addr == 0x279 || port_addr == 0x278) {
        // parallel/printer port
        if (is_read) {
            vctx->eax = 0;
        }
        success = true;
    } else {
        if (is_read) {
            LOG_VMM_ERR("unhandled io port read 0x%x\n", port_addr);
        } else {
            LOG_VMM_ERR("unhandled io port write 0x%x (value: 0x%lx)\n", port_addr, vctx->eax);
        }
    }

    return success;
}