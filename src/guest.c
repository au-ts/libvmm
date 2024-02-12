#include <microkit.h>
#include "util/util.h"
#include "vcpu.h"
#include "guest.h"

#define SCTLR_EL1_UCI (1 << 26)    /* Enable EL0 access to DC CVAU, DC CIVAC, DC CVAC, \
                                    and IC IVAU in AArch64 state   */
#define SCTLR_EL1_C (1 << 2)       /* Enable data and unified caches */
#define SCTLR_EL1_I (1 << 12)      /* Enable instruction cache       */
#define SCTLR_EL1_CP15BEN (1 << 5) /* AArch32 CP15 barrier enable    */
#define SCTLR_EL1_UTC (1 << 15)    /* Enable EL0 access to CTR_EL0   */
#define SCTLR_EL1_NTWI (1 << 16)   /* WFI executed as normal         */
#define SCTLR_EL1_NTWE (1 << 18)   /* WFE executed as normal         */

/* Disable MMU, SP alignment check, and alignment check */
/* A57 default value */
#define SCTLR_EL1_RES 0x30d00800 /* Reserved value */
#define SCTLR_EL1 (SCTLR_EL1_RES | SCTLR_EL1_CP15BEN | SCTLR_EL1_UTC | SCTLR_EL1_NTWI | SCTLR_EL1_NTWE)
#define SCTLR_EL1_NATIVE (SCTLR_EL1 | SCTLR_EL1_C | SCTLR_EL1_I | SCTLR_EL1_UCI)
#define SCTLR_DEFAULT SCTLR_EL1_NATIVE

bool guest_start(size_t boot_vcpu_id, uintptr_t kernel_pc, uintptr_t dtb, uintptr_t initrd) {
    /*
     * Set the TCB registers to what the virtual machine expects to be started with.
     * You will note that this is currently Linux specific as we currently do not support
     * any other kind of guest. However, even though the library is open to supporting other
     * guests, there is no point in prematurely generalising this code.
     */
    seL4_UserContext regs = {0};
    regs.x0 = dtb;
    regs.spsr = 5; // PMODE_EL1h
    regs.pc = kernel_pc;
    /* Write out all the TCB registers */
    seL4_Word err = seL4_TCB_WriteRegisters(
        BASE_VM_TCB_CAP + boot_vcpu_id,
        false, // We'll explcitly start the guest below rather than in this call
        0, // No flags
        SEL4_USER_CONTEXT_SIZE, // Writing to x0, pc, and spsr // @ivanv: for some reason having the number of registers here does not work... (in this case 2)
        &regs
    );
    assert(err == seL4_NoError);
    if (err != seL4_NoError) {
        LOG_VMM_ERR("Failed to write registers to boot vCPU's TCB (id is 0x%lx), error is: 0x%lx\n", boot_vcpu_id, err);
        return false;
    }
    LOG_VMM("starting guest at 0x%lx, DTB at 0x%lx, initial RAM disk at 0x%lx\n",
        regs.pc, regs.x0, initrd);
    printf("Default SCTLR_EL1: 0x%lx\n", SCTLR_EL1);
    printf("SCTLR_EL1: 0x%lx\n", microkit_arm_vcpu_read_reg(boot_vcpu_id, seL4_VCPUReg_SCTLR));
    // microkit_arm_vcpu_write_reg(boot_vcpu_id, seL4_VCPUReg_SCTLR, 0);
    // printf("SCTLR_EL1: 0x%lx\n", microkit_arm_vcpu_read_reg(boot_vcpu_id, seL4_VCPUReg_SCTLR));
    /* Restart the boot vCPU to the program counter of the TCB associated with it */
    microkit_vm_restart(boot_vcpu_id, regs.pc);


    return true;
}

void guest_stop(size_t boot_vcpu_id) {
    LOG_VMM("Stopping guest\n");
    microkit_vm_stop(boot_vcpu_id);
    LOG_VMM("Stopped guest\n");
}

bool guest_restart(size_t boot_vcpu_id, uintptr_t guest_ram_vaddr, size_t guest_ram_size) {
    LOG_VMM("Attempting to restart guest\n");
    // First, stop the guest
    microkit_vm_stop(boot_vcpu_id);
    LOG_VMM("Stopped guest\n");
    // Then, we need to clear all of RAM
    LOG_VMM("Clearing guest RAM\n");
    memset((char *)guest_ram_vaddr, 0, guest_ram_size);
    // Copy back the images into RAM
    // bool success = guest_init_images();
    // if (!success) {
    //     LOG_VMM_ERR("Failed to initialise guest images\n");
    //     return false;
    // }
    vcpu_reset(boot_vcpu_id);
    // Now we need to re-initialise all the VMM state
    // vmm_init();
    // linux_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
    LOG_VMM("Restarted guest\n");
    return true;
}
