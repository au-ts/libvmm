/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
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

void emulate_ioport_noop_access(seL4_VCPUContext *vctx, uint64_t f_qualification)
{
    uint64_t is_read = pio_fault_is_read(f_qualification);
    uint64_t is_string = pio_fault_is_string_op(f_qualification);

    assert(!is_string);

    if (is_read) {
        /* An invalid read of port I/O returns all 1s. */
        switch (pio_fault_to_access_width_bytes(f_qualification)) {
        case 1:
            vctx->eax = 0xff;
            break;
        case 2:
            vctx->eax = 0xffff;
            break;
        case 4:
            vctx->eax = 0xffffffff;
            break;
        default:
            LOG_VMM_ERR("unreachable!\n");
            assert(false);
        }
    }
}