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
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/linux.h>
#include <libvmm/arch/x86_64/fault.h>
#include <sel4/arch/vmenter.h>
#endif

bool guest_start(uintptr_t kernel_pc, uintptr_t dtb, uintptr_t initrd, void *linux_x86_setup)
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
    assert(linux_x86_setup);
    linux_x86_setup_ret_t *linux_setup = (linux_x86_setup_ret_t *) linux_x86_setup;
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

    // @billn explain

    // Set up system registers
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR0, CR0_DEFAULT);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR3, linux_setup->pml4_gpa);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR4, CR4_DEFAULT);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER, IA32_EFER_DEFAULT);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_RFLAGS, RFLAGS_DEFAULT);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_GDTR_BASE, linux_setup->gdt_gpa);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_GDTR_LIMIT, linux_setup->gdt_limit);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS, VMCS_PCC_DEFAULT);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_CONTROL_SECONDARY_PROCESSOR_CONTROLS, VMCS_SPC_DEFAULT);

    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_BASE, 0);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_BASE, 0);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_BASE, 0);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_BASE, 0);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_BASE, 0);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_BASE, 0);

    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_SELECTOR, 8);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_SELECTOR, 16);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_SELECTOR, 16);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_SELECTOR, 16);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_SELECTOR, 0);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_SELECTOR, 0);

    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_LIMIT, 0xfffff);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_LIMIT, 0xfffff);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_LIMIT, 0xfffff);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_LIMIT, 0xfffff);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_LIMIT, 0xfffff);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_LIMIT, 0xfffff);

    // 25-4 Vol. 3C @billn explain
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_ACCESS_RIGHTS, 0xA09B);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_ACCESS_RIGHTS, 0xC093);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_ACCESS_RIGHTS, 0xC093);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_ACCESS_RIGHTS, 0xC093);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_ACCESS_RIGHTS, 0xC093);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_ACCESS_RIGHTS, 0x3 | 1 << 4 | 1 << 7 | 1 << 15);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_ACCESS_RIGHTS, 0x3 | 1 << 4 | 1 << 7 | 1 << 15);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_LDTR_ACCESS_RIGHTS, 0x2 | 1 << 7);
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_TR_ACCESS_RIGHTS, 0xb | 1 << 7);


    microkit_mr_set(SEL4_VMENTER_CALL_EIP_MR, kernel_pc);
    microkit_mr_set(SEL4_VMENTER_CALL_CONTROL_PPC_MR, VMCS_PCC_DEFAULT);
    microkit_mr_set(SEL4_VMENTER_CALL_CONTROL_ENTRY_MR, VMCS_ENTRY_CTRL_DEfAULT);

    
    while (true) {
        LOG_VMM("VMEnter!\n");
        seL4_Word badge;
        seL4_Word ret = seL4_VMEnter(&badge);

        LOG_VMM("VMExit!\n");

        if (ret == SEL4_VMENTER_RESULT_NOTIF) {
            LOG_VMM("notif\n");
        } else if (ret == SEL4_VMENTER_RESULT_FAULT) {
            uint64_t new_rip;
            assert(fault_handle(GUEST_BOOT_VCPU_ID, &new_rip));
            microkit_mr_set(SEL4_VMENTER_CALL_EIP_MR, new_rip);
        }
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
