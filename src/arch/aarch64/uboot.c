/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "../../util/util.h"
#include "uboot.h"

bool pl011_emulation_init(pl011_device_t *dev, uintptr_t base, size_t size, sddf_handler_t* sddf_handlers) {
    /* Set the base address and size for the device */
    dev->base_address = base;
    dev->size = size;

    /* Setup registers */
    uint32_t fr = PL011_FR_RXFE;
    dev->registers.fr = fr; // indicates buffer is empty

    /* Assign handlers */
    dev->sddf_handlers = sddf_handlers;

    /* Register exception handler */
    bool success = fault_register_vm_exception_handler(base,
                                                        size,
                                                        &pl011_fault_handle,
                                                        dev);
    assert(success);

    return success;
}

static bool handle_pl011_reg_read(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, pl011_device_t *dev) {
    if (offset == offsetof(pl011_registers_t, fr)) { /* Attempt to read the flag register */
        /* 
            This might get read to:
                1. check whether the UART is busy transmitting - this won't ever happen since it's not a real device.
                2. check whether there's any input in the device buffer to be read to UBoot (RXFE bit 1 = empty)
            In either case we just return whatever value is in the register - it is updated elsewhere.
        */
        fault_emulate_write(regs, offset, fsr, dev->registers.fr);
    } else if (offset == offsetof(pl011_registers_t, dr)) { /* Attempt to read the data register */
        /*
            We make an assumption here that user keyboard input happens much slower than the rate data is processed.
            Thus whenever the data register is requested to be read by UBoot, we also indicate that there is no longer
            any data to be processed (by updating the flag register accordingly). This is not technically correct - see 
            the logic flow in the appropriate notified switch in client_vmm.c for specific details.
        */

        /* Update the flag register so next time UBoot polls it reads that the FIFO is empty */
        dev->registers.fr = dev->registers.fr | PL011_FR_RXFE;

        /* Respond to UBoot with contents of data register */
        fault_emulate_write(regs, offset, fsr, dev->registers.dr);
    } else {
        printf("Attempted read from unhandled register at: 0x%x\n", offset);
    }

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

bool pl011_console_handle_rx(pl011_device_t *dev) {
    //TODO - CURRENT ISSUE IS UBOOT HAS A BUNCH OF SPURIOUS RECEIVES WHEN IT STARTS UP THAT FILL UP THE BUFFER

    /* Update FR to indicate there's something to read in the buffer */
    dev->registers.fr = dev->registers.fr & ~(PL011_FR_RXFE);

    /* Access the receive buffer and dequeue the input */
    serial_queue_handle_t *sddf_rx_queue = (serial_queue_handle_t *) dev->sddf_handlers[SDDF_SERIAL_RX_HANDLE].queue_h;
    uintptr_t sddf_buffer = 0;
    unsigned int sddf_buffer_len = 0;

    int ret = serial_dequeue_active(sddf_rx_queue, &sddf_buffer, &sddf_buffer_len);
    char *input = ((char *) sddf_buffer);
    LOG_VMM("Buffer contents: %s\n", input);

    /* Put the input in the data register to be read out by UBoot later*/
    dev->registers.dr = input[0];  // Only want to read single characters out of the buffer so this should be okay

    /* Enqueue a free buffer for reuse */
    ret = serial_enqueue_free(sddf_rx_queue, sddf_buffer, BUFFER_SIZE);
    assert(!ret);

    return true;
}

// TODO - pl011_console_handle_tx(pl011_device_t *dev)

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
