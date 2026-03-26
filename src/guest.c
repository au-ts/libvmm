/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <string.h>
#include <microkit.h>
#include <libvmm/libvmm.h>

/* Global state for managing the guest. */
guest_t guest;

bool guest_init(arch_guest_init_t init_args)
{
    if (init_args.num_vcpus == 0) {
        LOG_VMM_ERR("number of guest vCPUs must be greater than zero");
        return false;
    }

    if (init_args.num_vcpus > GUEST_MAX_NUM_VCPUS) {
        LOG_VMM_ERR("number of guest vCPUs (%lu) is more than maximum (%lu)", init_args.num_vcpus, GUEST_MAX_NUM_VCPUS);
        return false;
    }

    guest.num_vcpus = init_args.num_vcpus;
    for (int i = 0; i < guest.num_vcpus; i++) {
        guest.vcpu_on_state[i] = false;
    }

    /* Initialise the virtual GIC driver */
    bool success = virq_controller_init();
    if (!success) {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
    }

    return success;
}

#if defined(CONFIG_ARCH_X86_64)
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/linux.h>
#include <libvmm/arch/x86_64/fault.h>
#include <sel4/arch/vmenter.h>
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
    seL4_UserContext regs = { 0 };
    regs.x0 = dtb;
    regs.spsr = 5; // PMODE_EL1h
    regs.pc = kernel_pc;
#elif defined(CONFIG_ARCH_X86_64)
#else
#error "Unsupported guest architecture"
#endif
    /* Write out all the TCB registers */

#if defined(CONFIG_ARCH_AARCH64)
    seL4_Word err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + GUEST_BOOT_VCPU_ID,
                                            false, // We'll explcitly start the guest below rather than in this call
                                            0, // No flags
                                            sizeof(seL4_UserContext) / sizeof(seL4_Word), &regs);
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
    LOG_VMM("starting guest at 0x%lx, initial RAM disk at 0x%lx\n", kernel_pc, dtb);
    microkit_vcpu_x86_deferred_resume(kernel_pc, VMCS_PCC_DEFAULT, 0);

#endif

    return true;
}

void guest_stop()
{
    LOG_VMM("Stopping guest\n");
#if !defined(CONFIG_VTX)
    // @billn revisit, not possible on x86
    microkit_vcpu_stop(GUEST_BOOT_VCPU_ID);
#endif
    LOG_VMM("Stopped guest\n");
}

bool guest_restart(uintptr_t guest_ram_vaddr, size_t guest_ram_size)
{
    LOG_VMM("Attempting to restart guest\n");
    // First, stop the guest
#if !defined(CONFIG_VTX)
    microkit_vcpu_stop(GUEST_BOOT_VCPU_ID);
#endif
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
