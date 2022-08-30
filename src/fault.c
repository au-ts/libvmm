#include "fault.h"

bool advance_fault() {
    return true;
    // /* If data was to be read, load it into the user context */
    // if (fault_is_data(fault) && fault_is_read(fault)) {
    //     /* Get register opearand */
    //     int rt  = get_rt(fault);

    //     /* Decode whether operand is banked */
    //     int reg = decode_vcpu_reg(rt, fault);
    //     if (reg == seL4_VCPUReg_Num) {
    //         /* register is not banked, use seL4_UserContext */
    //         seL4_Word *reg_ctx = decode_rt(rt, fault_get_ctx(fault));
    //         *reg_ctx = fault_emulate(fault, *reg_ctx);
    //     } else {
    //         /* register is banked, use vcpu invocations */
    //         seL4_ARM_VCPU_ReadRegs_t res = seL4_ARM_VCPU_ReadRegs(fault->vcpu->vcpu.cptr, reg);
    //         if (res.error) {
    //             ZF_LOGF("Read registers failed");
    //             return -1;
    //         }
    //         int error = seL4_ARM_VCPU_WriteRegs(fault->vcpu->vcpu.cptr, reg, fault_emulate(fault, res.value));
    //         if (error) {
    //             ZF_LOGF("Write registers failed");
    //             return -1;
    //         }
    //     }
    // }
    // DFAULT("%s: Emulate fault @ 0x%x from PC 0x%x\n",
    //        fault->vcpu->vm->vm_name, fault->addr, fault->ip);
    // /* If this is the final stage of the fault, return to user */
    // assert(fault->stage > 0);
    // fault->stage--;
    // if (fault->stage) {
    //     /* Data becomes invalid */
    //     fault->content &= ~CONTENT_DATA;
    //     return 0;
    // } else {
    //     return ignore_fault(fault);
    // }
}
