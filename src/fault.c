#include "fault.h"
#include "util.h"

#define CPSR_THUMB                 BIT(5)
#define CPSR_IS_THUMB(x)           ((x) & CPSR_THUMB)

// int fault_is_32bit_instruction(seL4_UserContext *regs)
// {
//     // @ivanv: assuming VCPU fault
//     return !CPSR_IS_THUMB(regs->spsr);
// }

bool advance_vcpu_fault(seL4_UserContext *regs, uint32_t hsr) {
    // For now we just ignore it and continue
    // regs->pc += fault_is_32bit_instruction(fault) ? 4 : 2;
    // Assume 64-bit instruction
    regs->pc += 4;
    seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + VM_ID, false, 0, SEL4_USER_CONTEXT_SIZE, regs);
    /* Reply to thread */
    sel4cp_msginfo msg = sel4cp_msginfo_new(0, 0);
    seL4_Send(4, msg);
    // return restart_fault(fault);
    return true;
}
