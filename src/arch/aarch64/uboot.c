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

bool pl011_emulation_init(pl011_device_t *dev, uintptr_t base, size_t size) {

    /* Set the base address and size for the device */
    dev->base_address = base;
    dev->size = size;

    /* Setup registers */
    uint32_t fr = PL011_FR_RXFE;
    dev->registers.fr = fr; // indicates buffer is empty

    /* Register exception handler */
    bool success = fault_register_vm_exception_handler(base,
                                                        size,
                                                        &pl011_fault_handle,
                                                        dev);
    assert(success);

    return success;
}

static bool handle_pl011_reg_read(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, pl011_device_t *dev) {
    // Why might we poll the flag register?? How can I differentiate these?
    // - checking if there's user input
    // - checking if the uart is still busy transmitting data

    // I expect UBoot to start polling 0x18 at some point but it keeps hanging - why is it hanging???
    //

    if (offset == offsetof(pl011_registers_t, fr)) { /* Attempt to read the flag register */
        /* 
            This might get read to:
                1. check whether the UART is busy transmitting - this won't ever happen since it's not a real device.
                2. check whether there's any input in the device buffer to be read to UBoot (RXFE bit 1 = empty)
            In either case we just return whatever value is in the register - it is updated elsewhere.
        */
        fault_emulate_write(regs, offset, fsr, dev->registers.fr);
    } else if (offset == offsetof(pl011_registers_t, dr)) { /* Attempt to read the data register */
        // The flow for this will be something like (We've told UBoot there's something in the buffer):
        // 1. Do all the protocol stuff for the SDDF queues (dequeu/enqueue, etc.) to read the next element
        // 2. Store that element in the data register
        // 3. Check if there's anything left in the SDDF queues, if there isn't we update the bit in the flag register
        // 4. Fault emulate write (i.e. send that element back to Uboot)
        // That should do it - UBoot will receive it and do whatever with it. It will then poll again and if the FR is still
        // set it should go through this flow again.

        // Really ally we have to do here is call fault emulate write. The SDDF queue handling and updating the DR and FR
        // is all done in client_vmm.c

        fault_emulate_write(regs, offset, fsr, dev->registers.dr);
    } else {
        printf("Attempted read from unhandled register at: 0x%x\n", offset);
    }

    // /* Polling flag register to see if there's anything available to read */
    // if (offset == FR_OFFSET) {
    //     printf("PLO11: access to FR (0x18)\n");
    //     uint32_t reg = 0x00000000; //TODO - stopped here
    //     // What format should the data be in to be sent back to UBoot
    //     fault_emulate_write(regs, offset, fsr, reg);
    // } else if (offset == 0x0) {
    //     printf("PLO11: access to DR (0x0)\n");
    //     // expect to loop on this since we're reading the data register but not checking its updated state
    // } else {
    //     printf("PL011: access to something random at 0x%x\n", offset);    
    // }



    // printf("|PL011READ|INFO: Read to 0x%x\n", offset);

    // 0x18 = flag register

    // microkit_dbg_putc((char) data);

    return true;
}

static bool handle_pl011_reg_write(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, pl011_device_t *dev) {
    uint32_t data = fault_get_data(regs, fsr);
    uint32_t mask = fault_get_data_mask(offset, fsr);
    /* Mask the data to write */
    data &= mask;
    
    if (offset == offsetof(pl011_registers_t, tcr)) { /* Attempt to write to control register */
        dev->registers.tcr = data;
    } else if (offset == offsetof(pl011_registers_t, lcr_h)) { /* Attempt to write to line control register */
        dev->registers.lcr_h = data;
    } else if (offset == offsetof(pl011_registers_t, dr)) { /* Attempt to write to data register */
        microkit_dbg_putc((char) data); // TODO - this needs to work with SDDF, currently just debug printing this
    } else {
        printf("Attempted write to unhandled register at: 0x%x\n", offset);
    }

    return true;
}

bool pl011_fault_handle(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data) {

    pl011_device_t *dev = (pl011_device_t *) data;
    assert(dev);

    if (fault_is_read(fsr)) {
        return handle_pl011_reg_read(vcpu_id, offset, fsr, regs, dev);
    } else {
        return handle_pl011_reg_write(vcpu_id, offset, fsr, regs, dev);
    }

    return true;
}
