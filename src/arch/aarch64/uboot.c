/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "../../util/util.h"
#include "uboot.h"

static struct pl011_queue_handler pl011_console_queues[PL011_CONSOLE_NUM_VIRTQ];

bool pl011_emulation_init(pl011_device_t *dev, uintptr_t base, size_t size, sddf_handler_t* sddf_handlers) {

    /* Set the base address and size for the device */
    dev->base_address = base;
    dev->size = size;

    /* Setup registers */
    uint32_t fr = PL011_FR_RXFE;
    dev->registers.fr = fr; // indicates buffer is empty

    /* Assign handlers */
    dev->sddf_handlers = sddf_handlers;

    /* Setup queues */
    dev->num_vqs = PL011_CONSOLE_NUM_VIRTQ;
    dev->vqs = pl011_console_queues;

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

bool pl011_console_handle_rx(pl011_device_t *dev) {
    /* Update FR to indicate there's something to read in the buffer */
    dev->registers.fr = dev->registers.fr & ~(PL011_FR_RXFE);

    dev->registers.dr = 120; // x in asci

    /*********NEW STUFF**********/
    serial_queue_handle_t *sddf_rx_queue = (serial_queue_handle_t *) dev->sddf_handlers[SDDF_SERIAL_RX_HANDLE].queue_h;
    uintptr_t sddf_buffer = 0;
    unsigned int sddf_buffer_len = 0;
    int ret = serial_dequeue_active(sddf_rx_queue, &sddf_buffer, &sddf_buffer_len);
    assert(!ret);

    assert(dev->num_vqs > RX_QUEUE);

    // TODO - STOPPED HERE - basically want to copy the remainder of the below structure over and try to read a character
    // out of the queue. All I've done atm is started adding some of the necessary structures to the pl011 device and
    // uboot.c/h files to set this all up + the small amount of code above. I'm not sure where the contents of the buffer
    // (i.e. the input we're trying to read) is actually stored atm.


    // /***********/
    // // serial_queue_handle_t *sddf_rx_queue = (serial_queue_handle_t *)dev->sddf_handlers[SDDF_SERIAL_RX_HANDLE].queue_h;
    // // uintptr_t sddf_buffer = 0;
    // // unsigned int sddf_buffer_len = 0;
    // // int ret = serial_dequeue_active(sddf_rx_queue, &sddf_buffer, &sddf_buffer_len);
    // // assert(!ret);
    // // if (ret != 0) {
    // //     LOG_CONSOLE_ERR("could not dequeue from RX used queue\n");
    // //     // @ivanv: handle properly
    // // }
 
    // // assert(dev->num_vqs > RX_QUEUE);
    // struct virtio_queue_handler *rx_queue = &dev->vqs[RX_QUEUE];
    // struct virtq *virtq = &rx_queue->virtq;
    // uint16_t guest_idx = virtq->avail->idx;
    // size_t idx = rx_queue->last_idx;

    // if (idx != guest_idx) {
    //     size_t bytes_written = 0;

    //     uint16_t desc_head = virtq->avail->ring[idx % virtq->num];
    //     struct virtq_desc desc;
    //     uint16_t desc_idx = desc_head;

    //     do {
    //         desc = virtq->desc[desc_idx];

    //         size_t bytes_to_copy = (desc.len < sddf_buffer_len) ? desc.len : sddf_buffer_len;
    //         memcpy((char *) desc.addr, (char *) sddf_buffer, bytes_to_copy - bytes_written);

    //         bytes_written += bytes_to_copy;
    //     } while (bytes_written != sddf_buffer_len);

    //     struct virtq_used_elem used_elem;
    //     used_elem.id = desc_head;
    //     used_elem.len = bytes_written;
    //     virtq->used->ring[guest_idx % virtq->num] = used_elem;
    //     virtq->used->idx++;

    //     rx_queue->last_idx++;

    //     // 3. Inject IRQ to guest
    //     // @ivanv: is setting interrupt status necesary?
    //     dev->data.InterruptStatus = BIT_LOW(0);
    //     bool success = virq_inject(GUEST_VCPU_ID, dev->virq);
    //     assert(success);
    // }

    // // 4. Enqueue sDDF buffer into RX free queue
    // ret = serial_enqueue_free(sddf_rx_queue, sddf_buffer, BUFFER_SIZE);
    // assert(!ret);
    // // @ivanv: error handle for release mode

    // /***********/







    return true;
}

// TODO - pl011_console_handle_tx(pl011_device_t *dev) {}

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
