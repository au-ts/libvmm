/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/fault.h>
#include <libvmm/tcb.h>
#include <libvmm/vcpu.h>

#ifdef CONFIG_ARCH_RISCV
extern fault_instruction_t decoded_instruction;
#endif

#define MAX_VM_EXCEPTION_HANDLERS 16

struct vm_exception_handler {
    uintptr_t base;
    uintptr_t end;
    vm_exception_handler_t callback;
    void *data;
};

static struct vm_exception_handler registered_vm_exception_handlers[MAX_VM_EXCEPTION_HANDLERS];
static size_t vm_exception_handler_index = 0;

bool fault_register_vm_exception_handler(uintptr_t base, size_t size, vm_exception_handler_t callback, void *data)
{
    if (vm_exception_handler_index == MAX_VM_EXCEPTION_HANDLERS - 1) {
        LOG_VMM_ERR("maximum number of VM exception handlers registered");
        return false;
    }

    if (size == 0) {
        LOG_VMM_ERR("registered VM exception handler with size 0\n");
        return false;
    }

    for (int i = 0; i < vm_exception_handler_index; i++) {
        struct vm_exception_handler *curr = &registered_vm_exception_handlers[i];
        if (!(base >= curr->end || base + size <= curr->base)) {
            LOG_VMM_ERR("VM exception handler [0x%lx..0x%lx), overlaps with another handler [0x%lx..0x%lx)\n",
                        base, base + size, curr->base, curr->end);
            return false;
        }
    }

    registered_vm_exception_handlers[vm_exception_handler_index] = (struct vm_exception_handler) {
        .base = base,
        .end = base + size,
        .callback = callback,
        .data = data,
    };
    vm_exception_handler_index += 1;

    return true;
}

static bool fault_handle_registered_vm_exceptions(size_t vcpu_id, uintptr_t addr, size_t fsr, seL4_UserContext *regs)
{
    for (int i = 0; i < MAX_VM_EXCEPTION_HANDLERS; i++) {
        uintptr_t base = registered_vm_exception_handlers[i].base;
        uintptr_t end = registered_vm_exception_handlers[i].end;
        vm_exception_handler_t callback = registered_vm_exception_handlers[i].callback;
        void *data = registered_vm_exception_handlers[i].data;
        if (addr >= base && addr < end) {
            bool success = callback(vcpu_id, addr - base, fsr, regs, data);
            if (!success) {
                LOG_VMM_ERR("registered virtual memory exception handler for region [0x%lx..0x%lx) at address 0x%lx failed\n", base,
                            end, addr);
            }

            return success;
        }
    }

    /* We could not find a handler for the faulting address. */
    return false;
}

bool fault_handle_vm_exception(size_t vcpu_id)
{
    seL4_Word addr = microkit_mr_get(seL4_VMFault_Addr);
    seL4_Word fsr = microkit_mr_get(seL4_VMFault_FSR);
#ifdef CONFIG_ARCH_RISCV
    seL4_Word htinst = microkit_mr_get(seL4_VMFault_Instruction);
#endif

    seL4_UserContext regs;
    int err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + vcpu_id, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    assert(err == seL4_NoError);

    assert(fault_is_read(fsr) || fault_is_write(fsr));

#ifdef CONFIG_ARCH_RISCV
    decoded_instruction = fault_decode_instruction(vcpu_id, &regs, htinst, addr);
#endif

    bool success = fault_handle_registered_vm_exceptions(vcpu_id, addr, fsr, &regs);
    if (!success) {
        /*
         * We could not find a registered handler for the address, meaning that the fault
         * is genuinely unexpected. Surprise!
         * Now we print out as much information relating to the fault as we can, hopefully
         * the programmer can figure out what went wrong.
         */
        size_t ip = microkit_mr_get(seL4_VMFault_IP);
        size_t is_prefetch = seL4_GetMR(seL4_VMFault_PrefetchFault);
        bool is_write = fault_is_write(fsr);
        LOG_VMM_ERR("unexpected memory fault on address: 0x%lx, FSR: 0x%lx, IP: 0x%lx, is_prefetch: %s, is_write: %s\n",
                    addr, fsr, ip, is_prefetch ? "true" : "false", is_write ? "true" : "false");
        tcb_print_regs(vcpu_id);
        vcpu_print_regs(vcpu_id);
    } else {
#if defined(CONFIG_ARCH_AARCH64)
        return fault_advance_vcpu(vcpu_id, &regs);
#elif defined(CONFIG_ARCH_RISCV)
        return fault_advance_vcpu(vcpu_id, &regs, decoded_instruction.compressed);
#elif defined(CONFIG_ARCH_X86_64)
        assert(false);
        return false;
#else
#error "Unknown architecture for fault handling"
#endif
    }

    return success;
}
