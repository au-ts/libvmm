/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <string.h>
#include <microkit.h>
#include <libvmm/libvmm.h>

/* Document referenced:
 * https://www.kernel.org/doc/Documentation/arm64/booting.txt
 */

#define SPSR_PMODE_EL1H 5
#define SPSR_ASYNC_ABORT_MASK_BIT BIT_LOW(8)
#define SPSR_IRQ_MASK_BIT BIT_LOW(7)
#define SPSR_FIQ_MASK_BIT BIT_LOW(6)

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

    if (init_args.num_guest_ram_regions == 0) {
        LOG_VMM_ERR("number of guest RAM regions must be greater than zero");
        return false;
    }

    if (init_args.num_guest_ram_regions > GUEST_MAX_RAM_REGIONS) {
        LOG_VMM_ERR("number of guest RAM regions (%lu) is more than maximum (%d)", init_args.num_guest_ram_regions,
                    GUEST_MAX_RAM_REGIONS);
        return false;
    }

    guest.num_vcpus = init_args.num_vcpus;
    for (int i = 0; i < guest.num_vcpus; i++) {
        guest.vcpu_on_state[i] = false;
    }

    for (int i = 0; i < init_args.num_guest_ram_regions; i++) {
        if (!guest_ram_add_region(init_args.guest_ram_regions[i])) {
            LOG_VMM_ERR("failed to bookkeep RAM region\n");
            return false;
        }
    }

    /* Initialise the virtual GIC driver */
    bool success = virq_controller_init();
    if (!success) {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
    }

    return success;
}

bool guest_start(uintptr_t kernel_pc, uintptr_t dtb, uintptr_t initrd)
{
    /*
     * Set the TCB registers to what the virtual machine expects to be started with.
     * You will note that this is currently Linux specific as we currently do not support
     * any other kind of guest. However, even though the library is open to supporting other
     * guests, there is no point in prematurely generalising this code.
     */
    seL4_UserContext regs = { 0 };
    regs.x0 = dtb;
    /* From referenced document:
     * Before jumping into the kernel, the following conditions must be met:
     * - CPU mode
     * All forms of interrupts must be masked in PSTATE.DAIF (Debug, SError,
     * IRQ and FIQ).
     * The CPU must be in either EL2 (RECOMMENDED in order to have access to
     * the virtualisation extensions) or non-secure EL1.
     */
    regs.spsr = SPSR_PMODE_EL1H | SPSR_ASYNC_ABORT_MASK_BIT | SPSR_IRQ_MASK_BIT | SPSR_FIQ_MASK_BIT;
    regs.pc = kernel_pc;
    /* Write out all the TCB registers */
    seL4_Word err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + GUEST_BOOT_VCPU_ID,
                                            false, // We'll explcitly start the guest below rather than in this call
                                            0, // No flags
                                            SEL4_USER_CONTEXT_SIZE, &regs);
    assert(err == seL4_NoError);
    if (err != seL4_NoError) {
        LOG_VMM_ERR("Failed to write registers to boot vCPU's TCB (id is 0x%lx), error is: 0x%lx\n", GUEST_BOOT_VCPU_ID,
                    err);
        return false;
    }
    LOG_VMM("starting guest at 0x%lx, DTB at 0x%lx, initial RAM disk at 0x%lx\n", regs.pc, regs.x0, initrd);

    vcpu_set_on(GUEST_BOOT_VCPU_ID, true);
    /* Restart the boot vCPU to the program counter of the TCB associated with it */
    microkit_vcpu_restart(GUEST_BOOT_VCPU_ID, regs.pc);

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
    vcpu_reset(GUEST_BOOT_VCPU_ID);
    // Now we need to re-initialise all the VMM state
    // vmm_init();
    // linux_start(GUEST_BOOT_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
    LOG_VMM("Restarted guest\n");
    return true;
}