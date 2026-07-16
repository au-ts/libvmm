/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <sddf/util/util.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/vmcs.h>

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-091US March 2026
 */

/* See "Table 30-3. Exit Qualification for Control-Register Accesses" */

static uint8_t get_cr_number(seL4_Word qualification)
{
    return qualification & 0xF;
}

typedef enum {
    MOV_TO_CR = 0,
    MOV_FROM_CR = 1,
    CLTS = 2,
    LMSW = 3,
} access_type_t;

static access_type_t get_access_type(seL4_Word qualification)
{
    return (access_type_t)((qualification >> 4) & 0x3);
}

static seL4_Word get_write_operand(seL4_VCPUContext *vctx, seL4_Word qualification)
{
    uint8_t reg = (qualification >> 8) & 0xF;
    switch (reg) {
    case 0:
        return vctx->eax;
    case 1:
        return vctx->ecx;
    case 2:
        return vctx->edx;
    case 3:
        return vctx->ebx;
    case 4:
        return microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_RSP);
    case 5:
        return vctx->ebp;
    case 6:
        return vctx->esi;
    case 7:
        return vctx->edi;
    case 8:
        return vctx->r8;
    case 9:
        return vctx->r9;
    case 10:
        return vctx->r10;
    case 11:
        return vctx->r11;
    case 12:
        return vctx->r12;
    case 13:
        return vctx->r13;
    case 14:
        return vctx->r14;
    case 15:
        return vctx->r15;
    }

    /* Hardware bug or wrong type of qualification. */
    assert(false);
    return 0;
}

static void write_to_read_operand(seL4_VCPUContext *vctx, seL4_Word qualification, seL4_Word data)
{
    uint8_t reg = (qualification >> 8) & 0xF;
    switch (reg) {
    case 0:
        vctx->eax = data;
        break;
    case 1:
        vctx->ecx = data;
        break;
    case 2:
        vctx->edx = data;
        break;
    case 3:
        vctx->ebx = data;
        break;
    case 4:
        microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_RSP, data);
        break;
    case 5:
        vctx->ebp = data;
        break;
    case 6:
        vctx->esi = data;
        break;
    case 7:
        vctx->edi = data;
        break;
    case 8:
        vctx->r8 = data;
        break;
    case 9:
        vctx->r9 = data;
        break;
    case 10:
        vctx->r10 = data;
        break;
    case 11:
        vctx->r11 = data;
        break;
    case 12:
        vctx->r12 = data;
        break;
    case 13:
        vctx->r13 = data;
        break;
    case 14:
        vctx->r14 = data;
        break;
    case 15:
        vctx->r15 = data;
        break;
    default:
        /* Hardware bug or wrong type of qualification. */
        assert(false);
    }
}

bool handle_cr_access(seL4_VCPUContext *vctx, seL4_Word qualification)
{
    switch (get_cr_number(qualification)) {
#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
    case 8: {
        if (get_access_type(qualification) == MOV_TO_CR) {
            return lapic_write_fault_handle(REG_LAPIC_TPR, get_write_operand(vctx, qualification));
        } else if (get_access_type(qualification) == MOV_FROM_CR) {
            uint32_t data;
            bool success = lapic_read_fault_handle(REG_LAPIC_TPR, &data);
            write_to_read_operand(vctx, qualification, data);
            return success;
        }
    }
#endif

    default:
        LOG_VMM_ERR("unhandled CR%d access, access type %d\n", get_cr_number(qualification),
                    get_access_type(qualification));
        return false;
    }
}