/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <string.h>
#include <microkit.h>
#include <libvmm/vcpu.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>

#if defined(CONFIG_ARCH_X86_64)
#include <libvmm/arch/x86_64/vcpu.h>
#endif

bool guest_start(uintptr_t kernel_pc, uintptr_t dtb, uintptr_t initrd)
{
    /*
     * Set the TCB registers to what the virtual machine expects to be started with.
     * You will note that this is currently Linux specific as we currently do not support
     * any other kind of guest. However, even though the library is open to supporting other
     * guests, there is no point in prematurely generalising this code.
     */
#if defined(CONFIG_ARCH_AARCH64)
    seL4_UserContext regs = {0};
    regs.x0 = dtb;
    regs.spsr = 5; // PMODE_EL1h
    regs.pc = kernel_pc;
#elif defined(CONFIG_ARCH_X86_64)

#else
#error "Unsupported guest architecture"
#endif
    /* Write out all the TCB registers */

    // @billn the following is probably not correct for x86?
    // should just be seL4_VMEnter() revisit

#if defined(CONFIG_ARCH_AARCH64)
    seL4_Word err = seL4_TCB_WriteRegisters(
        BASE_VM_TCB_CAP + GUEST_BOOT_VCPU_ID,
        false, // We'll explcitly start the guest below rather than in this call
        0, // No flags
        sizeof(seL4_UserContext) / sizeof(seL4_Word),
        &regs);
    assert(err == seL4_NoError);
    if (err != seL4_NoError) {
        LOG_VMM_ERR("Failed to write registers to boot vCPU's TCB (id is 0x%lx), error is: 0x%lx\n", GUEST_BOOT_VCPU_ID,
                    err);
        return false;
    }
    LOG_VMM("starting guest at 0x%lx, DTB at 0x%lx, initial RAM disk at 0x%lx\n", kernel_pc, dtb, initrd);

    // @billn revisit
    // vcpu_set_on(GUEST_BOOT_VCPU_ID, true);
    /* Restart the boot vCPU to the program counter of the TCB associated with it */
    microkit_vcpu_restart(GUEST_BOOT_VCPU_ID, kernel_pc);

#elif defined(CONFIG_ARCH_X86_64)
    LOG_VMM("starting guest at 0x%lx, DTB at 0x%lx, initial RAM disk at 0x%lx\n", kernel_pc, dtb, initrd);

    assert(vcpu_init(GUEST_BOOT_VCPU_ID, kernel_pc, 0x99000));

    // @billn explain
    // SEL4_VMENTER_CALL_EIP_MR
    microkit_mr_set(0, kernel_pc);
    // SEL4_VMENTER_CALL_CONTROL_PPC_MR
    microkit_mr_set(1, (1u<<31) | 1<<7);
    // SEL4_VMENTER_CALL_CONTROL_ENTRY_MR
    microkit_mr_set(2, 0);

    while (true) {
        seL4_Word badge;
        seL4_Word ret = seL4_VMEnter(&badge);

        if (ret == 0) {
            LOG_VMM("notif\n");
        } else if (ret == 1) {
            LOG_VMM("fault\n");
            seL4_Word rip = microkit_mr_get(0);
            LOG_VMM("ip = 0x%x\n", rip);
            seL4_Word cppc = microkit_mr_get(1); // SEL4_VMENTER_CALL_CONTROL_PPC_MR
            LOG_VMM("vm exec control = 0x%x\n", cppc);
            seL4_Word vmec = microkit_mr_get(2); // SEL4_VMENTER_CALL_CONTROL_ENTRY_MR
            LOG_VMM("vm entry control = 0x%x\n", vmec);
            seL4_Word f_reason = microkit_mr_get(3); // SEL4_VMENTER_FAULT_REASON_MR
            LOG_VMM("fault reason = 0x%x\n", f_reason);
            seL4_Word f_qual = microkit_mr_get(4); // SEL4_VMENTER_FAULT_QUALIFICATION_MR
            // reasons: /Users/dreamliner787-9/TS/microkit-capdl-dev/seL4/include/arch/x86/arch/object/vcpu.h
            LOG_VMM("fault qualification = 0x%x\n", f_qual);
            seL4_Word ins_len = microkit_mr_get(5); // SEL4_VMENTER_FAULT_INSTRUCTION_LEN_MR
            LOG_VMM("instruction length = 0x%x\n", ins_len);
            seL4_Word g_p_addr = microkit_mr_get(6); // SEL4_VMENTER_FAULT_GUEST_PHYSICAL_MR
            LOG_VMM("guest physical addr = 0x%x\n", g_p_addr);
        }

        // @billn, doing a vmenter after a fault without handling the fault first causes seL4 to crash. issue?
        break;
    }
    LOG_VMM("done\n");
#endif

    return true;
}

void guest_stop()
{
    LOG_VMM("Stopping guest\n");
    microkit_vcpu_stop(GUEST_BOOT_VCPU_ID);
    LOG_VMM("Stopped guest\n");
}

bool guest_restart(uintptr_t guest_ram_vaddr, size_t guest_ram_size)
{
    LOG_VMM("Attempting to restart guest\n");
    // First, stop the guest
    microkit_vcpu_stop(GUEST_BOOT_VCPU_ID);
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
    // vcpu_reset(GUEST_BOOT_VCPU_ID);
    // Now we need to re-initialise all the VMM state
    // vmm_init();
    // linux_start(GUEST_BOOT_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
    LOG_VMM("Restarted guest\n");
    return true;
}
