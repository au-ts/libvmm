/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/guest.h>

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-088US June 2025
 */

/* Table 29-5. "Exit Qualification for I/O Instructions" */
#define PIO_VIOLATION_READ_DIRECTION_BIT BIT(3)
#define PIO_VIOLATION_STRING_OP_BIT BIT(4)
#define PIO_VIOLATION_REP_PREFIX_BIT BIT(5)
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

static void pio_string_read_byte(seL4_VCPUContext *vctx, uint8_t data)
{
    uint64_t gva = vctx->edi;
    /* If paging is off this is still sound because gva_to_gpa will just return the GVA as GPA. */
    uint64_t gpa;
    size_t bytes_to_pg_boundary;
    bool translate_success = gva_to_gpa(0, gva, &gpa, &bytes_to_pg_boundary);
    if (!translate_success) {
        /* @billn inject PF? */
        LOG_VMM_ERR("GVA 0x%lx to GPA fail\n", gva);
        return;
    }
    /* We are copying byte by byte so no need to check the page boundary. */
    (void)bytes_to_pg_boundary;

    uint8_t *dest = gpa_to_hva(gpa, 1);
    if (!dest) {
        /* @billn now what? gpa is invalid */
        LOG_VMM_ERR("GPA 0x%lx is invalid\n", gpa);
        return;
    }

    *dest = data;

    if (vcpu_exit_get_rflags() & RFLAGS_DF) {
        vctx->edi--;
    } else {
        vctx->edi++;
    }
}

int pio_emulate_string_read(uint64_t qualification, seL4_VCPUContext *vctx, uint8_t *data, size_t data_len)
{
    assert(pio_fault_is_read(qualification));
    assert(pio_fault_is_string_op(qualification));

    uint64_t rep = 1;
    if (qualification & PIO_VIOLATION_REP_PREFIX_BIT) {
        rep = vctx->ecx;
    }

    int access_width_bytes = pio_fault_to_access_width_bytes(qualification);
    size_t bytes_requested = rep * access_width_bytes;
    size_t bytes_to_copy = MIN(bytes_requested, data_len);

    for (size_t i = 0; i < bytes_to_copy; i++) {
        pio_string_read_byte(vctx, data[i]);
    }

    for (size_t i = bytes_to_copy; i < bytes_requested; i++) {
        /* Invalid reads return all 1s. */
        pio_string_read_byte(vctx, 0xFF);
    }

    if (qualification & PIO_VIOLATION_REP_PREFIX_BIT) {
        /* We need to clear ECX according to the arch rule of the REP prefix. */
        vctx->ecx = 0;
    }

    return bytes_to_copy;
}

void pio_emulate_read(uint64_t qualification, seL4_VCPUContext *vctx, uint32_t data)
{
    assert(pio_fault_is_read(qualification));
    assert(!pio_fault_is_string_op(qualification));

    int access_width_bytes = pio_fault_to_access_width_bytes(qualification);
    /* We need to preserve the upper bits */
    switch (access_width_bytes) {
    case 1:
        vctx->eax = (vctx->eax & ~0xFFULL) | (data & 0xFFULL);
        break;
    case 2:
        vctx->eax = (vctx->eax & ~0xFFFFULL) | (data & 0xFFFFULL);
        break;
    case 4:
        /* In long mode this will zero extend. */
        vctx->eax = (uint32_t)data;
        break;
    default:
        LOG_VMM_ERR("unreachable!\n");
        assert(false);
    }
}

uint32_t pio_get_write_data(uint64_t qualification, seL4_VCPUContext *vctx)
{
    assert(!pio_fault_is_read(qualification));
    assert(!pio_fault_is_string_op(qualification));
    switch (pio_fault_to_access_width_bytes(qualification)) {
    case 1:
        return vctx->eax & 0xFFULL;
    case 2:
        return vctx->eax & 0xFFFFULL;
    case 4:
        return vctx->eax & 0xFFFFFFFFULL;
    default:
        LOG_VMM_ERR("unreachable!\n");
        assert(false);
        return 0;
    }
}

void emulate_ioport_noop_access(uint64_t qualification, seL4_VCPUContext *vctx)
{
    if (pio_fault_is_read(qualification)) {
        /* An invalid read of port I/O returns all 1s. */
        if (pio_fault_is_string_op(qualification)) {
            pio_emulate_string_read(qualification, vctx, NULL, 0);
        } else {
            uint32_t data = 0xFFFFFFFF;
            pio_emulate_read(qualification, vctx, data);
        }
    }
}