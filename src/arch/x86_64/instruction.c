/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <string.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

// https://wiki.osdev.org/X86-64_Instruction_Encoding

#define X86_MAX_INSTRUCTION_LENGTH 15

// https://c9x.me/x86/html/file_module_x86_id_176.html
#define NUM_MOV_OPCODES 4
#define OPCODE_MOV_BYTE_TO_MEM 0x88
#define OPCODE_MOV_WORD_TO_MEM 0x89
#define OPCODE_MOV_BYTE_FROM_MEM 0x8a
#define OPCODE_MOV_WORD_FROM_MEM 0x8b
static uint8_t mov_opcodes[NUM_MOV_OPCODES] = { OPCODE_MOV_BYTE_TO_MEM, OPCODE_MOV_WORD_TO_MEM,
                                                OPCODE_MOV_BYTE_FROM_MEM, OPCODE_MOV_WORD_FROM_MEM };

extern uint64_t guest_ram_vaddr;

/* Convert guest physical address to the VMM's virtual memory. */
void *gpa_to_vaddr(uint64_t gpa)
{
    return (void *)(guest_ram_vaddr + gpa);
}

seL4_Word pte_to_gpa(seL4_Word pte)
{
    assert(pte & 1);
    return pte & 0xffffffffff000;
}

bool pt_page_present(seL4_Word pte)
{
    return pte & BIT(7);
}

static register_idx_t modrm_reg_to_vctx_idx(uint8_t reg)
{
    switch (reg) {
    case 0:
        return RAX_IDX;
    case 1:
        return RCX_IDX;
    case 2:
        return RDX_IDX;
    case 3:
        return RBX_IDX;
    case 5:
        return RBP_IDX;
    case 6:
        return RSI_IDX;
    case 7:
        return RDI_IDX;
    default:
        LOG_VMM_ERR("unknown mod rm reg: 0x%x\n", reg);
        assert(false);
    }
}

decoded_instruction_ret_t decode_instruction(size_t vcpu_id, seL4_Word rip, seL4_Word instruction_len)
{
    seL4_Word cr4 = microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_CR4);

    seL4_Word pml4_gpa = microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_CR3) & ~0xfff;
    seL4_Word *pml4 = gpa_to_vaddr(pml4_gpa);
    seL4_Word pml4_idx = (rip >> (12 + (9 * 3))) & 0x1ff;

    seL4_Word pdpt_gpa = pte_to_gpa(pml4[pml4_idx]);
    uint64_t *pdpt = gpa_to_vaddr(pdpt_gpa);
    seL4_Word pdpt_idx = (rip >> (12 + (9 * 2))) & 0x1ff;

    seL4_Word pd_gpa = pte_to_gpa(pdpt[pdpt_idx]);
    uint64_t *pd = gpa_to_vaddr(pd_gpa);
    seL4_Word pd_idx = (rip >> (12 + (9 * 1))) & 0x1ff;

    uint64_t *page;
    if (pt_page_present(pd[pd_idx] & BIT(7))) {
        // 2MiB page
        seL4_Word page_gpa = pte_to_gpa((pd[pd_idx])) | (rip & 0x1fffff);
        page = gpa_to_vaddr(page_gpa);
    } else {
        // 4k page
        seL4_Word pt_gpa = pte_to_gpa(pd[pd_idx]);
        uint64_t *pt = gpa_to_vaddr(pt_gpa);
        seL4_Word pt_idx = (rip >> (12)) & 0x1ff;

        seL4_Word page_gpa = pte_to_gpa((pt[pt_idx])) | (rip & 0xfff);
        page = gpa_to_vaddr(page_gpa);
    }

    // @billn I really think something more "industrial grade" should be used for a job like this.
    // Such as https://github.com/zyantific/zydis which is no-malloc and no-libc, but it uses cmake...yuck
    // But then we introduce a dependency...

    assert(instruction_len <= X86_MAX_INSTRUCTION_LENGTH);
    uint8_t instruction_buf[X86_MAX_INSTRUCTION_LENGTH];
    memset(instruction_buf, 0, X86_MAX_INSTRUCTION_LENGTH);
    memcpy(instruction_buf, (uint8_t *)page, instruction_len);

    decoded_instruction_ret_t ret = { .type = INSTRUCTION_DECODE_FAIL, .decoded = {} };

    // @billn scan for rex byte to detect 64-bit r/w, at this point all APIC accesses are 32-bit so this isnt an issue

    bool opcode_valid = false;

    // First step is to match the opcode against a list of known opcodes that we provide decoding logic for.
    for (int i = 0; i < NUM_MOV_OPCODES; i++) {
        // An opcode can be multi-bytes, but `mov`s are only 1-byte
        if (instruction_buf[0] == mov_opcodes[i]) {
            opcode_valid = true;

            // An opcode byte is followed by a ModR/M byte to encode src/dest register.
            uint8_t modrm = instruction_buf[1];
            // uint8_t mod = modrm >> 6;
            uint8_t reg = (modrm >> 3) & 0x7;
            // uint8_t rm = modrm & 0x7;

            ret.type = INSTRUCTION_MEMORY;
            switch (instruction_buf[0]) {
            case OPCODE_MOV_BYTE_TO_MEM:
            case OPCODE_MOV_BYTE_FROM_MEM:
                ret.decoded.memory_instruction.access_width = BYTE_ACCESS_WIDTH;
                break;
            case OPCODE_MOV_WORD_TO_MEM:
            case OPCODE_MOV_WORD_FROM_MEM:
                // @billn revisit 16-byte access decoding
                ret.decoded.memory_instruction.access_width = DWORD_ACCESS_WIDTH;
                break;
            }

            ret.decoded.memory_instruction.target_reg = modrm_reg_to_vctx_idx(reg);

            break;
        }
    }

    assert(opcode_valid);

    // LOG_VMM("raw instruction:\n");
    // for (int i = 0; i < instruction_len; i++) {
    //     LOG_VMM("[%d]: 0x%x\n", i, instruction_buf[i]);
    // }
    return ret;
}