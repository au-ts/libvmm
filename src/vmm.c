/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <sel4cp.h>
#include "util/util.h"
#include "vgic/vgic.h"
#include "vmm.h"
#include "linux.h"
#include "tcb.h"
#include "vcpu.h"
#include "fault.h"
#include "guest.h"
#include "virq.h"

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the device tree to be passed to the kernel. */
extern char _guest_dtb_image[];
extern char _guest_dtb_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. */
extern char _guest_initrd_image[];
extern char _guest_initrd_image_end[];
/* seL4CP will set this variable to the start of the guest RAM memory region. */
uintptr_t guest_ram_vaddr;

static void serial_ack(size_t vcpu_id, int irq, void *cookie) {
    /*
     * For now we by default simply ack the serial IRQ, we have not
     * come across a case yet where more than this needs to be done.
     */
    sel4cp_irq_ack(SERIAL_IRQ_CH);
}

void
init(void)
{
    // Initialise the VMM, the VCPU(s), and start the guest
    LOG_VMM("starting \"%s\"\n", sel4cp_name);
    // Place all the binaries in the right locations before starting the guest
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    uintptr_t kernel_pc = linux_setup_images(guest_ram_vaddr,
                                      (uintptr_t) _guest_kernel_image,
                                      kernel_size,
                                      (uintptr_t) _guest_dtb_image,
                                      GUEST_DTB_VADDR,
                                      dtb_size,
                                      (uintptr_t) _guest_initrd_image,
                                      GUEST_INIT_RAM_DISK_VADDR,
                                      initrd_size
                                      );
    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }
    // Initialise the virtual GIC driver
    bool success = virq_controller_init(GUEST_VCPU_ID);
    if (!success) {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
        return;
    }
    // @ivanv: Note that remove this line causes the VMM to fault if we
    // actually get the interrupt. This should be avoided by making the VGIC driver more stable.
    success = virq_register(GUEST_VCPU_ID, SERIAL_IRQ, &serial_ack, NULL);
    // Just in case there is already an interrupt available to handle, we ack it here.
    // @ivanv: not sure if this is necessary? is this the right place to do this
    sel4cp_irq_ack(SERIAL_IRQ_CH);
    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

void
notified(sel4cp_channel ch)
{
    switch (ch) {
        case SERIAL_IRQ_CH: {
            bool success = virq_inject(GUEST_VCPU_ID, SERIAL_IRQ);
            if (!success) {
                LOG_VMM_ERR("IRQ %d dropped on vCPU %d\n", SERIAL_IRQ, GUEST_VCPU_ID);
            }
            break;
        }
        default:
            printf("Unexpected channel, ch: 0x%lx\n", ch);
    }
}

void
fault(sel4cp_id id, sel4cp_msginfo msginfo)
{
    if (id != GUEST_VCPU_ID) {
        LOG_VMM_ERR("Unexpected faulting PD/VM with id %d\n", id);
        return;
    }
    // This is the primary fault handler for the guest, all faults that come
    // from seL4 regarding the guest will need to be handled here.
    uint64_t label = sel4cp_msginfo_get_label(msginfo);
    bool success = false;
    switch (label) {
        case seL4_Fault_VMFault:
            success = handle_vm_fault(id);
            break;
        case seL4_Fault_UnknownSyscall:
            success = handle_unknown_syscall(id);
            break;
        case seL4_Fault_UserException:
            success = handle_user_exception(id);
            break;
        case seL4_Fault_VGICMaintenance:
            success = handle_vgic_maintenance(id);
            break;
        case seL4_Fault_VCPUFault:
            success = handle_vcpu_fault(id);
            break;
        case seL4_Fault_VPPIEvent:
            success = handle_vppi_event(id);
            break;
        default:
            LOG_VMM_ERR("unknown fault, stopping VM with ID %d\n", id);
            sel4cp_vm_stop(id);
            return;
            // @ivanv: print out the actual fault details
    }

    if (success) {
        /* Now that we have handled the fault, we reply to it so that the guest can resume execution. */
        reply_to_fault();
    } else {
        LOG_VMM_ERR("Failed to handle %s fault\n", fault_to_string(label));
    }
}
