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
    vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_GDTR_LIMIT, 24);
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

    LOG_VMM("VMEnter!\n");

    while (true) {
        seL4_Word badge;
        seL4_Word ret = seL4_VMEnter(&badge);

        LOG_VMM("VMExit!\n");

        if (ret == 0) {
            LOG_VMM("notif\n");
        } else if (ret == 1) {
            seL4_Word cppc = microkit_mr_get(SEL4_VMENTER_CALL_CONTROL_PPC_MR);
            seL4_Word vmec = microkit_mr_get(SEL4_VMENTER_CALL_CONTROL_ENTRY_MR);
            seL4_Word f_reason = microkit_mr_get(SEL4_VMENTER_FAULT_REASON_MR);
            seL4_Word f_qual = microkit_mr_get(SEL4_VMENTER_FAULT_QUALIFICATION_MR);
            seL4_Word ins_len = microkit_mr_get(SEL4_VMENTER_FAULT_INSTRUCTION_LEN_MR);
            seL4_Word g_p_addr = microkit_mr_get(SEL4_VMENTER_FAULT_GUEST_PHYSICAL_MR);
            seL4_Word rflags = microkit_mr_get(SEL4_VMENTER_FAULT_RFLAGS_MR);
            seL4_Word interruptability = microkit_mr_get(SEL4_VMENTER_FAULT_GUEST_INT_MR);
            seL4_Word cr3 = microkit_mr_get(SEL4_VMENTER_FAULT_CR3_MR);

            seL4_Word rip = vmcs_read(GUEST_BOOT_VCPU_ID, VMX_GUEST_RIP);
            seL4_Word rsp = vmcs_read(GUEST_BOOT_VCPU_ID, VMX_GUEST_RSP);

            seL4_Word eax = microkit_mr_get(10);
            seL4_Word ebx = microkit_mr_get(11);
            seL4_Word ecx = microkit_mr_get(12);
            seL4_Word edx = microkit_mr_get(13);
            seL4_Word esi = microkit_mr_get(14);
            seL4_Word edi = microkit_mr_get(15);
            seL4_Word ebp = microkit_mr_get(16);

            seL4_Word r8 = microkit_mr_get(17);
            seL4_Word r9 = microkit_mr_get(18);
            seL4_Word r10 = microkit_mr_get(19);
            seL4_Word r11 = microkit_mr_get(20);
            seL4_Word r12 = microkit_mr_get(21);
            seL4_Word r13 = microkit_mr_get(22);
            seL4_Word r14 = microkit_mr_get(23);
            seL4_Word r15 = microkit_mr_get(24);

            LOG_VMM("fault\n");
            LOG_VMM("vm exec control = 0x%lx\n", cppc);
            LOG_VMM("vm entry control = 0x%lx\n", vmec);
            LOG_VMM("fault reason = 0x%lx\n", f_reason);
            // reasons: /Users/dreamliner787-9/TS/microkit-capdl-dev/seL4/include/arch/x86/arch/object/vcpu.h
            LOG_VMM("fault qualification = 0x%lx\n", f_qual);
            LOG_VMM("instruction length = 0x%lx\n", ins_len);
            LOG_VMM("guest physical addr = 0x%lx\n", g_p_addr);
            LOG_VMM("rflags = 0x%lx\n", rflags);
            LOG_VMM("interruptability = 0x%lx\n", interruptability);
            LOG_VMM("cr3 = 0x%lx\n", cr3);
            LOG_VMM("=========================\n");
            LOG_VMM("rip = 0x%lx\n", rip);
            LOG_VMM("rsp = 0x%lx\n", rsp);
            LOG_VMM("eax = 0x%lx\n", eax);
            LOG_VMM("ebx = 0x%lx\n", ebx);
            LOG_VMM("ecx = 0x%lx\n", ecx);
            LOG_VMM("edx = 0x%lx\n", edx);
            LOG_VMM("esi = 0x%lx\n", esi);
            LOG_VMM("edi = 0x%lx\n", edi);
            LOG_VMM("ebp = 0x%lx\n", ebp);
            LOG_VMM("r8 = 0x%lx\n", r8);
            LOG_VMM("r9 = 0x%lx\n", r9);
            LOG_VMM("r10 = 0x%lx\n", r10);
            LOG_VMM("r11 = 0x%lx\n", r11);
            LOG_VMM("r12 = 0x%lx\n", r12);
            LOG_VMM("r13 = 0x%lx\n", r13);
            LOG_VMM("r14 = 0x%lx\n", r14);
            LOG_VMM("r15 = 0x%lx\n", r15);
            LOG_VMM("=========================\n");
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
