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
#include "smc.h"
#include "fault.h"
#include "hsr.h"
#include "vmm.h"

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

// @ivanv: document where these come from
#define SYSCALL_PA_TO_IPA 65
#define SYSCALL_NOP 67

#define HSR_SMC_64_EXCEPTION        (0x17)
#define HSR_MAX_EXCEPTION           (0x3f)
#define HSR_EXCEPTION_CLASS_SHIFT   (26)
#define HSR_EXCEPTION_CLASS_MASK    (HSR_MAX_EXCEPTION << HSR_EXCEPTION_CLASS_SHIFT)
#define HSR_EXCEPTION_CLASS(hsr)    (((hsr) & HSR_EXCEPTION_CLASS_MASK) >> HSR_EXCEPTION_CLASS_SHIFT)

static bool handle_unknown_syscall(sel4cp_msginfo msginfo)
{
    // @ivanv: should print out the name of the VM the fault came from.
    uint64_t syscall = sel4cp_mr_get(seL4_UnknownSyscall_Syscall);
    uint64_t fault_ip = sel4cp_mr_get(seL4_UnknownSyscall_FaultIP);

    seL4_UserContext regs;
    int err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + VM_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    assert(!err);
    regs.pc += 4;

    LOG_VMM("Received syscall 0x%lx\n", syscall);
    switch (syscall) {
        case SYSCALL_PA_TO_IPA:
            // @ivanv: why do we not do anything here?
            // @ivanv, how to get the physical address to translate?
            LOG_VMM("Received PA translation syscall\n");
            break;
        case SYSCALL_NOP:
            LOG_VMM("Received NOP syscall\n");
            break;
        default:
            LOG_VMM_ERR("Unknown syscall: syscall number: 0x%lx, PC: 0x%lx\n", syscall, fault_ip);
            return false;
    }
    err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + VM_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    assert(!err);

    return true;
}

static bool got_ppi_event = false;

static bool handle_vppi_event()
{
    uint64_t ppi_irq = sel4cp_mr_get(seL4_VPPIEvent_IRQ);
    if (!got_ppi_event) {
        printf("FIRST PPI EVENT!\n\n");
    }
    got_ppi_event = true;
    // if (restarted) {
    //     sel4cp_arm_vcpu_ack_vppi(VM_ID, ppi_irq);
    //     reply_to_fault();
    //     return true;
    // }
    // We directly inject the interrupt assuming it has been previously registered.
    // If not the interrupt will dropped by the VM.
    bool success = vgic_inject_irq(VCPU_ID, ppi_irq);
    if (!success) {
        // @ivanv, make a note that when having a lot of printing on it can cause this error
        LOG_VMM_ERR("VPPI IRQ %lu dropped on vCPU %d\n", ppi_irq, VCPU_ID);
        // Acknowledge to unmask it as our guest will not use the interrupt
        // @ivanv: We're going to assume that we only have one VCPU and that the
        // cap is the base one.
        uint64_t ack_err = sel4cp_arm_vcpu_ack_vppi(VM_ID, ppi_irq);
        assert(ack_err == seL4_NoError);
        if (ack_err) {
            LOG_VMM_ERR("Failed to ACK VPPI\n");
            return false;
        }
    }

    reply_to_fault();

    return true;
}

static bool handle_vcpu_fault(sel4cp_msginfo msginfo)
{
    // @ivanv: should this be uint32_t?
    uint64_t hsr = sel4cp_mr_get(seL4_VCPUFault_HSR);
    uint64_t hsr_ec_class = HSR_EXCEPTION_CLASS(hsr);
    switch (hsr_ec_class) {
        case HSR_SMC_64_EXCEPTION:
            return handle_smc(hsr);
        case HSR_WFx_EXCEPTION:
            // If we get a WFI exception, we just do nothing in the VMM.
            reply_to_fault();
            break;
        default:
            LOG_VMM_ERR("unknown SMC exception, EC class: 0x%lx, HSR: 0x%lx\n", hsr_ec_class, hsr);
            return false;
    }

    return true;
}

static int handle_user_exception(sel4cp_msginfo msginfo)
{
    // @ivanv: print out VM name/vCPU id when we have multiple VMs
    uint64_t fault_ip = sel4cp_mr_get(seL4_UserException_FaultIP);
    uint64_t number = sel4cp_mr_get(seL4_UserException_Number);
    LOG_VMM_ERR("Invalid instruction fault at IP: 0x%lx, number: 0x%lx", fault_ip, number);

    // Dump registers
    seL4_UserContext regs = {0};
    seL4_Error ret = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + VM_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    if (ret != seL4_NoError) {
        printf("Failure reading regs, error %d", ret);
        return false;
    } else {
        dump_ctx(&regs);
    }

    return true;
}

static bool handle_vm_fault()
{
    uint64_t addr = sel4cp_mr_get(seL4_VMFault_Addr);
    uint64_t fsr = sel4cp_mr_get(seL4_VMFault_FSR);

    seL4_UserContext regs;
    int err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + VM_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    assert(err == seL4_NoError);

    uint64_t addr_page_aligned = addr & (~(0x1000 - 1));
    switch (addr_page_aligned) {
        case GIC_DIST_PADDR...GIC_DIST_PADDR + GIC_DIST_SIZE:
            // dump_ctx(&regs);
            // printf("Handling GIC dist fault!\n");
            return handle_vgic_dist_fault(VCPU_ID, addr, fsr, &regs);
#if defined(GIC_V3)
        case GIC_REDIST_PADDR...GIC_REDIST_PADDR + GIC_REDIST_SIZE:
            return handle_vgic_redist_fault(VCPU_ID, addr, fsr, &regs);
#endif
        default: {
            uint64_t ip = sel4cp_mr_get(seL4_VMFault_IP);
            uint64_t is_prefetch = seL4_GetMR(seL4_VMFault_PrefetchFault);
            // @ivanv: why does this have a U?
            uint64_t is_write = (fsr & (1U << 6)) != 0;
            LOG_VMM_ERR("unexpected VM fault on address: 0x%lx, FSR: 0x%lx, IP: 0x%lx, is_prefetch: %s, is_write: %s\n", addr, fsr, ip, is_prefetch ? "true" : "false", is_write ? "true" : "false");
            dump_ctx(&regs);
            assert(0);
        }
    }

    return false;
}

#define SGI_RESCHEDULE_IRQ  0
#define SGI_FUNC_CALL       1
#define PPI_VTIMER_IRQ      27

static void vppi_event_ack(uint64_t vcpu_id, int irq, void *cookie)
{
    uint64_t err = sel4cp_arm_vcpu_ack_vppi(VM_ID, irq);
    assert(err == seL4_NoError);
    if (err) {
        LOG_VMM_ERR("failed to ACK VPPI, vCPU: 0x%lx, IRQ: 0x%lx\n", vcpu_id, irq);
    }
}

static void sgi_ack(uint64_t vcpu_id, int irq, void *cookie) {}

static void serial_ack(uint64_t vcpu_id, int irq, void *cookie) {
#if defined(BOARD_qemu_arm_virt_hyp)
    sel4cp_irq_ack(SERIAL_IRQ_CH);
#elif defined(BOARD_odroidc2_hyp)
    sel4cp_irq_ack(SERIAL_IRQ_CH);
#endif
}

void guest_copy_images(void) {
    // Copy the guest kernel image into the right location
    uint64_t kernel_image_size = _guest_kernel_image_end - _guest_kernel_image;
    LOG_VMM("Copying guest kernel image to 0x%x (0x%x bytes)\n", guest_ram_vaddr + 0x80000, kernel_image_size);
    memcpy((char *)guest_ram_vaddr + 0x80000, _guest_kernel_image, kernel_image_size);
    // Copy the guest device tree blob into the right location
    uint64_t dtb_image_size = _guest_dtb_image_end - _guest_dtb_image;
    LOG_VMM("Copying guest DTB to 0x%x (0x%x bytes)\n", GUEST_DTB_VADDR, dtb_image_size);
    memcpy((char *)GUEST_DTB_VADDR, _guest_dtb_image, dtb_image_size);
    // Copy the initial RAM disk into the right location
    uint64_t initrd_image_size = _guest_initrd_image_end - _guest_initrd_image;
    LOG_VMM("Copying guest initial RAM disk to 0x%x (0x%x bytes)\n", GUEST_INIT_RAM_DISK_VADDR, initrd_image_size);
    memcpy((char *)GUEST_INIT_RAM_DISK_VADDR, _guest_initrd_image, initrd_image_size);
}

void guest_start(void) {
    // Initialise the virtual GIC driver
    vgic_init();
#if defined(GIC_V2)
    LOG_VMM("initialised virtual GICv2 driver\n");
#elif defined(GIC_V3)
    LOG_VMM("initialised virtual GICv3 driver\n");
#else
#error "Unsupported GIC version"
#endif
    bool err = vgic_register_irq(VCPU_ID, PPI_VTIMER_IRQ, &vppi_event_ack, NULL);
    if (!err) {
        LOG_VMM_ERR("Failed to register vCPU virtual timer IRQ: 0x%lx\n", PPI_VTIMER_IRQ);
        return;
    }
    err = vgic_register_irq(VCPU_ID, SGI_RESCHEDULE_IRQ, &sgi_ack, NULL);
    if (!err) {
        LOG_VMM_ERR("Failed to register vCPU SGI 0 IRQ");
        return;
    }
    err = vgic_register_irq(VCPU_ID, SGI_FUNC_CALL, &sgi_ack, NULL);
    if (!err) {
        LOG_VMM_ERR("Failed to register vCPU SGI 1 IRQ");
        return;
    }
    // @ivanv: Note that remove this line causes the VMM to fault if we
    // actually get the interrupt. This should be avoided by making the VGIC driver more stable.
    err = vgic_register_irq(VCPU_ID, SERIAL_IRQ, &serial_ack, NULL);
    // @ivanv: comment why we ack it
    sel4cp_irq_ack(SERIAL_IRQ_CH);
    // @ivanv: do we need to set the guest init pc?
    seL4_UserContext regs = {0};
    regs.x0 = GUEST_DTB_VADDR;
    regs.spsr = 5; // PMODE_EL1h
    regs.pc = guest_ram_vaddr + 0x80000;
    err = seL4_TCB_WriteRegisters(
        BASE_VM_TCB_CAP + VM_ID,
        false, // We'll explcitly start the guest below rather than in this call
        0, // No flags
        SEL4_USER_CONTEXT_SIZE, // Writing to x0, pc, and spsr // @ivanv: for some reason having the number of registers here does not work... (in this case 2)
        &regs
    );
    assert(!err);
    // Set the PC to the kernel image's entry point and start the thread.
    // @ivanv: should probably log initrd addr as well.
    LOG_VMM("starting guest at 0x%lx, DTB at 0x%lx\n", regs.pc, regs.x0);
    sel4cp_vm_restart(VM_ID, regs.pc);
}

void guest_restart(void) {
    LOG_VMM("Attempting to restart guest\n");
    // First, stop the guest
    // sel4cp_vm_stop(VM_ID);
    // LOG_VMM("Stopped guest\n");
    // Then, we need to clear all of RAM
    LOG_VMM("Clearing guest RAM\n");
    memset((char *)guest_ram_vaddr, 0, GUEST_RAM_SIZE);
    // Copy back the images into RAM
    guest_copy_images();
    // Now we need to re-initialise all the VMM state
    guest_start();
    // Finally, we can to restart the VM.
    uint64_t guest_init_pc = guest_ram_vaddr + 0x80000; // @ivanv: get rid of
    LOG_VMM("restarting guest at 0x%lx, DTB at 0x%lx\n", guest_init_pc, GUEST_DTB_VADDR);
    sel4cp_vm_restart(VM_ID, guest_init_pc);
}

void
init(void)
{
    // Initialise the VMM, the VCPU(s), and start the guest
    LOG_VMM("starting \"%s\"\n", sel4cp_name);
    // Place all the binaries in the right locations before starting the guest
    guest_copy_images();
    // Initialise and start guest (setup VGIC, setup interrupts, TCB registers)
    guest_start();
}

int restart = 1;

void
notified(sel4cp_channel ch)
{
    switch (ch) {
        case SERIAL_IRQ_CH: {
            bool success = vgic_inject_irq(VCPU_ID, SERIAL_IRQ);
            if (!success) {
                LOG_VMM_ERR("IRQ %d dropped on vCPU %d\n", SERIAL_IRQ, VCPU_ID);
            }
            break;
        }
        default:
            printf("Unexpected channel, ch: 0x%lx\n", ch);
    }
}

void
fault(sel4cp_vm vm, sel4cp_msginfo msginfo)
{
    // Decode the fault and call the appropriate handler
    uint64_t label = sel4cp_msginfo_get_label(msginfo);
    // @ivanv: should be checking the results of the handlers
    switch (label) {
        case seL4_Fault_VMFault:
            handle_vm_fault();
            break;
        case seL4_Fault_UnknownSyscall:
            handle_unknown_syscall(msginfo);
            break;
        case seL4_Fault_UserException:
            handle_user_exception(msginfo);
            break;
        case seL4_Fault_VGICMaintenance:
            handle_vgic_maintenance();
            break;
        case seL4_Fault_VCPUFault:
            handle_vcpu_fault(msginfo);
            break;
        case seL4_Fault_VPPIEvent:
            handle_vppi_event();
            break;
        default:
            LOG_VMM_ERR("unknown fault, stopping VM %d\n", vm);
            sel4cp_vm_stop(vm);
            // @ivanv: print out the actual fault details
    }
}
