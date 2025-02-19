#include <libvmm/fault.h>
#include <libvmm/util/util.h>

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

bool fault_handle_registered_vm_exceptions(size_t vcpu_id, uintptr_t addr, size_t fsr, seL4_UserContext *regs)
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
