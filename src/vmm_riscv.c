/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <sel4cp.h>
#include <stddef.h>
#include "util/util.h"

#if defined(BOARD_qemu_riscv_virt)
#define VM_IMAGE_ENTRY_ADDR (0x80000000 + 0x80000)
#define VM_DTB_ADDR 0x84000000
#else
#error Need to define VM image address and DTB address
#endif

// #define SGI_RESCHEDULE_IRQ  0
// #define SGI_FUNC_CALL       1
// #define PPI_VTIMER_IRQ      27

// #if defined(BOARD_qemu_arm_virt)
// #define SERIAL_IRQ_CH 1
// #define SERIAL_IRQ 33
// #elif defined(BOARD_odroidc2)
// #define SERIAL_IRQ_CH 1
// #define SERIAL_IRQ 225
// #elif defined(BOARD_imx8mm_evk)
// #define SERIAL_IRQ_CH 1
// #define SERIAL_IRQ 79
// #endif

// static void vppi_event_ack(uint64_t vcpu_id, int irq, void *cookie)
// {
//     uint64_t err = sel4cp_vcpu_ack_vppi(VM_ID, irq);
//     assert(err == seL4_NoError);
//     if (err) {
//         LOG_VMM_ERR("failed to ACK VPPI, vCPU: 0x%lx, IRQ: 0x%lx\n", vcpu_id, irq);
//     }
// }

// static void sgi_ack(uint64_t vcpu_id, int irq, void *cookie) {}

// static void serial_ack(uint64_t vcpu_id, int irq, void *cookie) {
// #if defined(BOARD_qemu_arm_virt)
//     sel4cp_irq_ack(SERIAL_IRQ_CH);
// #elif defined(BOARD_odroidc2)
//     // sel4cp_irq_ack(SERIAL_IRQ_CH);
//     // vgic_inject_irq(vcpu_id, irq);
// #endif
// }

bool handle_vm_fault() {
    return true;
}

bool handle_user_exception(sel4cp_msginfo msginfo) {
    return true;
}

bool handle_vcpu_fault(sel4cp_msginfo msginfo) {
    return true;
}

void
init(void)
{
    // Initialise the VMM, the VCPU(s), and start the guest
    LOG_VMM("starting \"%s\"\n", sel4cp_name);
    LOG_VMM("completed all initialisation\n");

    // Set the PC to the kernel image's entry point and start the thread.
    LOG_VMM("starting VM at 0x%lx, DTB at 0x%lx\n", VM_IMAGE_ENTRY_ADDR, VM_DTB_ADDR);
    sel4cp_vm_restart(VM_ID, VM_IMAGE_ENTRY_ADDR);
}

void
notified(sel4cp_channel ch)
{
    // switch (ch) {
    //     case SERIAL_IRQ_CH: {
    //         bool success = vgic_inject_irq(VCPU_ID, SERIAL_IRQ);
    //         if (!success) {
    //             LOG_VMM_ERR("IRQ %d dropped on vCPU: 0x%lx\n", SERIAL_IRQ, VCPU_ID);
    //         }
    //         break;
    //     }
    //     default:
    //         printf("Unexpected channel, ch: 0x%lx\n", ch);
    // }
}

void
fault(sel4cp_vm vm, sel4cp_msginfo msginfo)
{
    // Decode the fault and call the appropriate handler
    uint64_t label = sel4cp_msginfo_get_label(msginfo);
    switch (label) {
        case seL4_Fault_VMFault:
            handle_vm_fault();
            break;
        case seL4_Fault_UserException:
            handle_user_exception(msginfo);
            break;
        case seL4_Fault_VCPUFault:
            handle_vcpu_fault(msginfo);
            break;
        default:
            LOG_VMM_ERR("unknown fault, stopping VM %d\n", vm);
            sel4cp_vm_stop(vm);
    }
}
