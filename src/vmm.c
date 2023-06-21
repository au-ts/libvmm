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
// @jade: not a good place, the header in temporary here until we figure out a build system
#ifdef VIRTIO_NET
#include "virtio/virtio_mmio.h"
#include "virtio/virtio_net_emul.h"
#include "virtio/virtio_net_vswitch.h"
#endif
#include "smc.h"
#include "fault.h"
#include "hsr.h"
#include "vmm.h"
#include "arch/aarch64/linux.h"

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

static bool handle_unknown_syscall(sel4cp_msginfo msginfo)
{
    // @ivanv: should print out the name of the VM the fault came from.
    uint64_t syscall = sel4cp_mr_get(seL4_UnknownSyscall_Syscall);
    uint64_t fault_ip = sel4cp_mr_get(seL4_UnknownSyscall_FaultIP);

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

    seL4_UserContext regs;
    seL4_Error err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + GUEST_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    assert(!err);

    return fault_advance_vcpu(&regs);
}

static bool handle_vppi_event()
{
    uint64_t ppi_irq = sel4cp_mr_get(seL4_VPPIEvent_IRQ);
    // We directly inject the interrupt assuming it has been previously registered.
    // If not the interrupt will dropped by the VM.
    bool success = vgic_inject_irq(GUEST_VCPU_ID, ppi_irq);
    if (!success) {
        // @ivanv, make a note that when having a lot of printing on it can cause this error
        LOG_VMM_ERR("VPPI IRQ %lu dropped on vCPU %d\n", ppi_irq, GUEST_VCPU_ID);
        // Acknowledge to unmask it as our guest will not use the interrupt
        // @ivanv: We're going to assume that we only have one VCPU and that the
        // cap is the base one.
        sel4cp_arm_vcpu_ack_vppi(GUEST_ID, ppi_irq);
    }

    return true;
}

static bool handle_vcpu_fault(sel4cp_msginfo msginfo, uint64_t vcpu_id)
{
    uint32_t hsr = sel4cp_mr_get(seL4_VCPUFault_HSR);
    uint64_t hsr_ec_class = HSR_EXCEPTION_CLASS(hsr);
    switch (hsr_ec_class) {
        case HSR_SMC_64_EXCEPTION:
            return handle_smc(vcpu_id, hsr);
        case HSR_WFx_EXCEPTION:
            // If we get a WFI exception, we just do nothing in the VMM.
            return true;
        default:
            LOG_VMM_ERR("unknown SMC exception, EC class: 0x%lx, HSR: 0x%lx\n", hsr_ec_class, hsr);
            return false;
    }
}

static int handle_user_exception(sel4cp_msginfo msginfo)
{
    // @ivanv: print out VM name/vCPU id when we have multiple VMs
    uint64_t fault_ip = sel4cp_mr_get(seL4_UserException_FaultIP);
    uint64_t number = sel4cp_mr_get(seL4_UserException_Number);
    LOG_VMM_ERR("Invalid instruction fault at IP: 0x%lx, number: 0x%lx", fault_ip, number);

    // Dump registers
    seL4_UserContext regs = {0};
    seL4_Error ret = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + GUEST_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    if (ret != seL4_NoError) {
        printf("Failure reading regs, error %d", ret);
        return false;
    } else {
        print_tcb_regs(&regs);
    }

    return true;
}

static bool handle_vm_fault()
{
    uint64_t addr = sel4cp_mr_get(seL4_VMFault_Addr);
    uint64_t fsr = sel4cp_mr_get(seL4_VMFault_FSR);

    seL4_UserContext regs;
    int err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + GUEST_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    assert(err == seL4_NoError);

    uint64_t addr_page_aligned = addr & (~(0x1000 - 1));
    switch (addr_page_aligned) {
        case GIC_DIST_PADDR...GIC_DIST_PADDR + GIC_DIST_SIZE:
            return handle_vgic_dist_fault(GUEST_VCPU_ID, addr, fsr, &regs);
#if defined(GIC_V3)
        /* Need to handle redistributor faults for GICv3 platforms. */
        case GIC_REDIST_PADDR...GIC_REDIST_PADDR + GIC_REDIST_SIZE:
            return handle_vgic_redist_fault(GUEST_VCPU_ID, addr, fsr, &regs);
#endif
#ifdef VIRTIO_NET
        case VIRTIO_ADDRESS_START...VIRTIO_ADDRESS_END:
            return handle_virtio_mmio_fault(GUEST_VCPU_ID, addr, fsr, &regs);
#endif
        default: {
            uint64_t ip = sel4cp_mr_get(seL4_VMFault_IP);
            uint64_t is_prefetch = seL4_GetMR(seL4_VMFault_PrefetchFault);
            uint64_t is_write = (fsr & (1 << 6)) != 0;
            LOG_VMM_ERR("unexpected memory fault on address: 0x%lx, FSR: 0x%lx, IP: 0x%lx, is_prefetch: %s, is_write: %s\n", addr, fsr, ip, is_prefetch ? "true" : "false", is_write ? "true" : "false");
            print_tcb_regs(&regs);
            print_vcpu_regs(GUEST_ID);
            return false;
        }
    }
}

#define SGI_RESCHEDULE_IRQ  0
#define SGI_FUNC_CALL       1
#define PPI_VTIMER_IRQ      27

static void vppi_event_ack(uint64_t vcpu_id, int irq, void *cookie)
{
    sel4cp_arm_vcpu_ack_vppi(GUEST_ID, irq);
}

static void sgi_ack(uint64_t vcpu_id, int irq, void *cookie) {}

static void serial_ack(uint64_t vcpu_id, int irq, void *cookie) {
    /*
     * For now we by default simply ack the serial IRQ, we have not
     * come across a case yet where more than this needs to be done.
     */
    sel4cp_irq_ack(SERIAL_IRQ_CH);
}

bool guest_init_images(void) {
    // First we inspect the kernel image header to confirm it is a valid image
    // and to determine where in memory to place the image. Currently this
    // process assumes the guest is the Linux kernel.
    struct linux_image_header *image_header = (struct linux_image_header *) &_guest_kernel_image;
    assert(image_header->magic == LINUX_IMAGE_MAGIC);
    if (image_header->magic != LINUX_IMAGE_MAGIC) {
        LOG_VMM_ERR("Linux kernel image magic check failed\n");
        return false;
    }
    // Copy the guest kernel image into the right location
    uint64_t kernel_image_size = _guest_kernel_image_end - _guest_kernel_image;
    uint64_t kernel_image_vaddr = guest_ram_vaddr + image_header->text_offset;
    // This check is because the Linux kernel image requires to be placed at text_offset of
    // a 2MiB aligned base address anywhere in usable system RAM and called there.
    // In this case, we place the image at the text_offset of the start of the guest's RAM,
    // so we need to make sure that the start of guest RAM is 2MiB aligned.
    //
    // @ivanv: Ideally this check would be done at build time, we have all the information
    // we need at build time to enforce this.
    assert((guest_ram_vaddr & ((1 << 20) - 1)) == 0);
    LOG_VMM("Copying guest kernel image to 0x%x (0x%x bytes)\n", kernel_image_vaddr, kernel_image_size);
    memcpy((char *)kernel_image_vaddr, _guest_kernel_image, kernel_image_size);
    // Copy the guest device tree blob into the right location
    uint64_t dtb_image_size = _guest_dtb_image_end - _guest_dtb_image;
    LOG_VMM("Copying guest DTB to 0x%x (0x%x bytes)\n", GUEST_DTB_VADDR, dtb_image_size);
    memcpy((char *)GUEST_DTB_VADDR, _guest_dtb_image, dtb_image_size);
    // Copy the initial RAM disk into the right location
    uint64_t initrd_image_size = _guest_initrd_image_end - _guest_initrd_image;
    LOG_VMM("Copying guest initial RAM disk to 0x%x (0x%x bytes)\n", GUEST_INIT_RAM_DISK_VADDR, initrd_image_size);
    memcpy((char *)GUEST_INIT_RAM_DISK_VADDR, _guest_initrd_image, initrd_image_size);

    return true;
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
    bool err = vgic_register_irq(GUEST_VCPU_ID, PPI_VTIMER_IRQ, &vppi_event_ack, NULL);
    if (!err) {
        LOG_VMM_ERR("Failed to register vCPU virtual timer IRQ: 0x%lx\n", PPI_VTIMER_IRQ);
        return;
    }
    err = vgic_register_irq(GUEST_VCPU_ID, SGI_RESCHEDULE_IRQ, &sgi_ack, NULL);
    if (!err) {
        LOG_VMM_ERR("Failed to register vCPU SGI 0 IRQ");
        return;
    }
    err = vgic_register_irq(GUEST_VCPU_ID, SGI_FUNC_CALL, &sgi_ack, NULL);
    if (!err) {
        LOG_VMM_ERR("Failed to register vCPU SGI 1 IRQ");
        return;
    }
    // @ivanv: Note that remove this line causes the VMM to fault if we
    // actually get the interrupt. This should be avoided by making the VGIC driver more stable.
    err = vgic_register_irq(GUEST_VCPU_ID, SERIAL_IRQ, &serial_ack, NULL);
    // Just in case there is already an interrupt available to handle, we ack it here.
    // @ivanv: not sure if this is necessary? is this the right place to do this
    sel4cp_irq_ack(SERIAL_IRQ_CH);
    seL4_UserContext regs = {0};

    // @jade: we need to be able to configure devices an irqs in some system description instead of putting them here.
#ifdef VIRTIO_NET
    virtio_net_emul_init();
    err = vgic_register_irq(GUEST_VCPU_ID, VIRTIO_NET_IRQ, &virtio_net_ack, NULL);
    if (!err) {
        printf("VMM|ERROR: Failed to register VirtIO Net IRQ\n");
    }
#endif

    regs.x0 = GUEST_DTB_VADDR;
    regs.spsr = 5; // PMODE_EL1h
    // Read the entry point and set it to the program counter
    struct linux_image_header *image_header = (struct linux_image_header *) &_guest_kernel_image;
    uint64_t kernel_image_vaddr = guest_ram_vaddr + image_header->text_offset;
    regs.pc = kernel_image_vaddr;
    // Set all the TCB registers
    err = seL4_TCB_WriteRegisters(
        BASE_VM_TCB_CAP + GUEST_ID,
        false, // We'll explcitly start the guest below rather than in this call
        0, // No flags
        SEL4_USER_CONTEXT_SIZE, // Writing to x0, pc, and spsr // @ivanv: for some reason having the number of registers here does not work... (in this case 2)
        &regs
    );
    assert(!err);
    // Set the PC to the kernel image's entry point and start the thread.
    LOG_VMM("starting guest at 0x%lx, DTB at 0x%lx, initial RAM disk at 0x%lx\n",
        regs.pc, regs.x0, GUEST_INIT_RAM_DISK_VADDR);
    seL4_UserContext read_regs = {0};
    seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + GUEST_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &read_regs);
    sel4cp_vm_restart(GUEST_ID, regs.pc);
}

#define SCTLR_EL1_UCI       (1 << 26)     /* Enable EL0 access to DC CVAU, DC CIVAC, DC CVAC,
                                           and IC IVAU in AArch64 state   */
#define SCTLR_EL1_C         (1 << 2)      /* Enable data and unified caches */
#define SCTLR_EL1_I         (1 << 12)     /* Enable instruction cache       */
#define SCTLR_EL1_CP15BEN   (1 << 5)      /* AArch32 CP15 barrier enable    */
#define SCTLR_EL1_UTC       (1 << 15)     /* Enable EL0 access to CTR_EL0   */
#define SCTLR_EL1_NTWI      (1 << 16)     /* WFI executed as normal         */
#define SCTLR_EL1_NTWE      (1 << 18)     /* WFE executed as normal         */

/* Disable MMU, SP alignment check, and alignment check */
/* A57 default value */
#define SCTLR_EL1_RES      0x30d00800   /* Reserved value */
#define SCTLR_EL1          ( SCTLR_EL1_RES | SCTLR_EL1_CP15BEN | SCTLR_EL1_UTC \
                           | SCTLR_EL1_NTWI | SCTLR_EL1_NTWE )
#define SCTLR_EL1_NATIVE   (SCTLR_EL1 | SCTLR_EL1_C | SCTLR_EL1_I | SCTLR_EL1_UCI)
#define SCTLR_DEFAULT      SCTLR_EL1_NATIVE

void guest_stop(void) {
    LOG_VMM("Stopping guest\n");
    sel4cp_vm_stop(GUEST_ID);
    LOG_VMM("Stopped guest\n");
}

bool guest_restart(void) {
    LOG_VMM("Attempting to restart guest\n");
    // First, stop the guest
    sel4cp_vm_stop(GUEST_ID);
    LOG_VMM("Stopped guest\n");
    // Then, we need to clear all of RAM
    LOG_VMM("Clearing guest RAM\n");
    memset((char *)guest_ram_vaddr, 0, GUEST_RAM_SIZE);
    // Copy back the images into RAM
    bool success = guest_init_images();
    if (!success) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return false;
    }
    // Reset registers
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_SCTLR, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_TTBR0, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_TTBR1, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_TCR, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_MAIR, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_AMAIR, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_CIDR, 0);
    /* other system registers EL1 */
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_ACTLR, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_CPACR, 0);
    /* exception handling registers EL1 */
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_AFSR0, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_AFSR1, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_ESR, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_FAR, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_ISR, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_VBAR, 0);
    /* thread pointer/ID registers EL0/EL1 */
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_TPIDR_EL1, 0);
#if CONFIG_MAX_NUM_NODES > 1
    /* Virtualisation Multiprocessor ID Register */
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_VMPIDR_EL2, 0);
#endif /* CONFIG_MAX_NUM_NODES > 1 */
    /* general registers x0 to x30 have been saved by traps.S */
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_SP_EL1, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_ELR_EL1, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_SPSR_EL1, 0); // 32-bit
    /* generic timer registers, to be completed */
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_CNTV_CTL, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_CNTV_CVAL, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_CNTVOFF, 0);
    sel4cp_arm_vcpu_write_reg(GUEST_ID, seL4_VCPUReg_CNTKCTL_EL1, 0);
    // Now we need to re-initialise all the VMM state
    guest_start();
    LOG_VMM("Restarted guest\n");
    return true;
}

void
init(void)
{
    // Initialise the VMM, the VCPU(s), and start the guest
    LOG_VMM("starting \"%s\"\n", sel4cp_name);
    // Place all the binaries in the right locations before starting the guest
    bool success = guest_init_images();
    if (!success) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        assert(0);
    }
    // Initialise and start guest (setup VGIC, setup interrupts, TCB registers)
    guest_start();
}

void
notified(sel4cp_channel ch)
{
    switch (ch) {
        case SERIAL_IRQ_CH: {
            bool success = vgic_inject_irq(GUEST_VCPU_ID, SERIAL_IRQ);
            if (!success) {
                LOG_VMM_ERR("IRQ %d dropped on vCPU %d\n", SERIAL_IRQ, GUEST_VCPU_ID);
            }
            break;
        }
#ifdef VIRTIO_NET
        case VSWITCH_CONN_CH_1:
            vswitch_rx(ch);
            break;
#endif
        default:
            printf("Unexpected channel, ch: 0x%lx\n", ch);
    }
}

void
fault(sel4cp_id id, sel4cp_msginfo msginfo)
{
    if (id != GUEST_ID) {
        LOG_VMM_ERR("Unexpected faulting PD/VM with id %d\n", id);
        return;
    }
    // This is the primary fault handler for the guest, all faults that come
    // from seL4 regarding the guest will need to be handled here.
    uint64_t label = sel4cp_msginfo_get_label(msginfo);
    bool success = false;
    switch (label) {
        case seL4_Fault_VMFault:
            success = handle_vm_fault();
            break;
        case seL4_Fault_UnknownSyscall:
            success = handle_unknown_syscall(msginfo);
            break;
        case seL4_Fault_UserException:
            success = handle_user_exception(msginfo);
            break;
        case seL4_Fault_VGICMaintenance:
            success = handle_vgic_maintenance(GUEST_VCPU_ID);
            break;
        case seL4_Fault_VCPUFault:
            success = handle_vcpu_fault(msginfo, GUEST_VCPU_ID);
            break;
        case seL4_Fault_VPPIEvent:
            success = handle_vppi_event();
            break;
        default:
            LOG_VMM_ERR("unknown fault, stopping VM with ID %d\n", id);
            sel4cp_vm_stop(id);
            return;
            // @ivanv: print out the actual fault details
    }

    if (!success) {
        LOG_VMM_ERR("Failed to handle %s fault\n", fault_to_string(label));
    } else {
        /* Now that we have handled the fault, we reply to it so that the guest can resume execution. */
        reply_to_fault();
    }
}
