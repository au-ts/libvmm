/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/util/util.h>

#define X86_MAX_INSTRUCTION_LENGTH 15

typedef struct decode_fail_instruction_data {
} decode_fail_instruction_data_t;

typedef struct memory_instruction_data {

} memory_instruction_data_t;

typedef union instruction_data {
    decode_fail_instruction_data_t decode_fail;
    memory_instruction_data_t memory_instruction;
} instruction_data_t;

typedef enum instruction_type {
    DECODE_FAIL,
    MEMORY,
} instruction_type_t;

typedef struct decoded_instruction_ret {
    instruction_type_t type;
    instruction_data_t decoded;
} decoded_instruction_ret_t;

static uint8_t *mov_opcodes[] = { 0x88, 0x89, 0x8a, 0x8b };

extern uint64_t guest_ram_vaddr;

/* Convert guest physical address to the VMM's virtual memory. */
void *gpa_to_vaddr(uint64_t gpa) {
    return (void *)(guest_ram_vaddr + gpa);
}

seL4_Word pte_to_gpa(seL4_Word pte) {
    assert(pte & 1);
    return pte & 0xffffffffff000;
}

decoded_instruction_ret_t fault_instruction(size_t vcpu_id, seL4_Word rip, seL4_Word instruction_len) {
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

    // @billn scan for rex


    LOG_VMM("decoded instruction:\n");
    for (int i = 0; i < instruction_len; i++) {
        LOG_VMM("[%d]: 0x%x\n", i, instruction_buf[i]);
    }

    return 0;
}