/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "../../util/util.h"
#include "uboot.h"
uintptr_t uboot_setup_images(uintptr_t ram_start,
                             uintptr_t uboot,
                             size_t uboot_size,
                             uintptr_t dtb_src,
                             uintptr_t dtb_dest,
                             size_t dtb_size
                             )
{
    //TODO: need to work out where in RAM does uboot go??? Seems like this would be board specific? Could just try loading it into ram start
    uintptr_t uboot_dest = ram_start + 0x200000; // we shift uboot so we can put the devic tree blob at the start of RAM
    LOG_VMM("Copying u-boot to 0x%x (0x%x bytes)\n", uboot_dest, uboot_size);
    memcpy((char *)uboot_dest, (char *)uboot, uboot_size);

    // Just using the linux dtb here so we have one
    LOG_VMM("Copying guest DTB to 0x%x (0x%x bytes)\n", dtb_dest, dtb_size); // need to place a device tree here to be picked up by Uboot
    memcpy((char *)dtb_dest, (char *)dtb_src, dtb_size);

    return uboot_dest;
}

uintptr_t uboot_setup_kernel(uintptr_t ram_start,
                             uintptr_t uboot,
                             size_t uboot_size,
                             uintptr_t dtb_src,
                             uintptr_t dtb_dest,
                             size_t dtb_size,
                             uintptr_t image_src,
                             uintptr_t image_dest,
                             size_t image_size
                             )
{
    //TODO: need to work out where in RAM does uboot go??? Seems like this would be board specific? Could just try loading it into ram start
    uintptr_t uboot_dest = ram_start + 0x200000; // we shift uboot so we can put the devic tree blob at the start of RAM
    LOG_VMM("Copying u-boot to 0x%x (0x%x bytes)\n", uboot_dest, uboot_size);
    memcpy((char *)uboot_dest, (char *)uboot, uboot_size);

    // Just using the linux dtb here so we have one
    LOG_VMM("Copying guest DTB to 0x%x (0x%x bytes)\n", dtb_dest, dtb_size); // need to place a device tree here to be picked up by Uboot
    memcpy((char *)dtb_dest, (char *)dtb_src, dtb_size);

    /* setup and testing for the Linux image */
    struct linux_image_header *image_header = (struct linux_image_header *) image_src;
    assert(image_header->magic == LINUX_IMAGE_MAGIC);
    if (image_header->magic != LINUX_IMAGE_MAGIC) {
        LOG_VMM_ERR("Linux kernel image magic check failed\n");
        return 0;
    }

    LOG_VMM("Copying image to 0x%x (0x%x bytes)\n", image_dest, image_size);
    memcpy((char *)image_dest, (char *)image_src, image_size);

    return uboot_dest;
}

bool uboot_start(size_t boot_vcpu_id, uintptr_t pc, uintptr_t dtb) {
    /*
     * Set the TCB registers to what the virtual machine expects to be started with.
     * You will note that this is currently Linux specific as we currently do not support
     * any other kind of guest. However, even though the library is open to supporting other
     * guests, there is no point in prematurely generalising this code.
     */
    seL4_UserContext regs = {0};
    regs.x0 = dtb;
    regs.spsr = 5; // PMODE_EL1h
    regs.pc = pc;
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
    LOG_VMM("starting uboot at 0x%lx, DTB at 0x%lx\n", regs.pc, regs.x0);
    /* Restart the boot vCPU to the program counter of the TCB associated with it */
    microkit_vm_restart(boot_vcpu_id, regs.pc);

    return true;
}
