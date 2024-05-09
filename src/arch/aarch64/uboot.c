/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "../../util/util.h"
#include "uboot.h"

uintptr_t uboot_setup_image(uintptr_t ram_start,
                             uintptr_t uboot,
                             size_t uboot_size,
                             size_t uboot_offset,
                             uintptr_t dtb_src,
                             uintptr_t dtb_dest,
                             size_t dtb_size
                             )
{
    // Shift uboot as it seems to expect DTB is at start of RAM
    uintptr_t uboot_dest = ram_start + uboot_offset;
    LOG_VMM("Copying u-boot to 0x%x (0x%x bytes)\n", uboot_dest, uboot_size);
    memcpy((char *)uboot_dest, (char *)uboot, uboot_size);

    LOG_VMM("Copying guest DTB to 0x%x (0x%x bytes)\n", dtb_dest, dtb_size);
    memcpy((char *)dtb_dest, (char *)dtb_src, dtb_size);

    return uboot_dest;
}

bool uboot_start(size_t boot_vcpu_id, uintptr_t pc, uintptr_t dtb) {
    /*
     * Set the TCB registers to what the virtual machine expects to be started with.
     * You will note that this is currently Linux specific as we currently do not support
     * any other kind of guest. However, even though the library is open to supporting other
     * guests, there is no point in prematurely generalising this code.
     * TODO - not sure if we should update this for UBoot or if we even need to do this???
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

bool pl011_emulation_init(uintptr_t base, size_t size, pl011_device_t *dev) {

    // ATM all we're doing is registering this exception handler
    bool success = fault_register_vm_exception_handler(base,
                                                        size,
                                                        &pl011_fault_handle,
                                                        dev);
    assert(success);

    return success;
}

static bool handle_pl011_reg_read(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs) {
    // Need to call the fault_emulate_write function here. This will respond to UBoots read request with 
    // whatever value (in this case need to work out what it's expecting so we can pass that back).

    // Is that how this works? There is no actual device at this address, we're just emulating it so 
    // having access to that memory isn't going to do anything since the client can't actually access it anyway.
    // Here we just need to respond to the client's read request with whatever value they're expecting (which we 
    // can do using fault_emulate_write).

    // The function logic for write looks like it uses offset to determine what register to select
    // and then ANDs that with the data??? Not sure about this

    size_t test = 0x0;    
    fault_emulate_write(regs, offset, fsr, test);

    // // Reading values here doesn't mean anything since this is just some random trash data??
    // uint32_t data = fault_get_data(regs, fsr);
    // uint32_t mask = fault_get_data_mask(offset, fsr);
    // /* Mask the data to write */
    // data &= mask;
    // printf("|PL011READ|INFO: Read to 0x%x. Value: %lu\n", offset, (unsigned long) data);

    // microkit_dbg_putc((char) data);

    return true;
}

static bool handle_pl011_reg_write(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs) {
    uint32_t data = fault_get_data(regs, fsr);
    uint32_t mask = fault_get_data_mask(offset, fsr);
    /* Mask the data to write */
    data &= mask;

    // printf("|PL011WRITE|INFO: Write to 0x%x. Value: %lu\n", offset, (unsigned long) data);

    microkit_dbg_putc((char) data);

    return true;
}


// This is the structure of the callback, can unpack it to find the relevant info
// TODO: can't remove the data here as the callback excepts the argument but not using the device atm
bool pl011_fault_handle(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data) {

    if (fault_is_read(fsr)) {
        return handle_pl011_reg_read(vcpu_id, offset, fsr, regs);
    } else {
        return handle_pl011_reg_write(vcpu_id, offset, fsr, regs);
    }

    return true;
}
