// @ivanv: need license for where I grabbed all the handlers from
// #include <assert.h>
#include <sel4cp.h>
#include <stddef.h>
#include "util.h"
#include "vgic/vgic.h"
#include "smc.h"

//
// -> finish handle_smc
// -> finish handle_psci
// -> 

// @ivanv: do we print out an error if we can't read registers or should we just assert

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
    // TODO(ivanv): should print out the name of the VM the fault came from.
    uint64_t syscall = sel4cp_mr_get(seL4_UnknownSyscall_Syscall);
    uint64_t fault_ip = sel4cp_mr_get(seL4_UnknownSyscall_FaultIP);

    seL4_UserContext regs;
    int err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + VM_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    halt(!err);
    regs.pc += 4;

    printf("VMM|INFO: Received syscall 0x%lx\n", syscall);
    switch (syscall) {
        case SYSCALL_PA_TO_IPA:
            // @ivanv: why do we not do anything here?
            // @ivanv, how to get the physical address to translate?
            printf("VMM|INFO: Received PA translation syscall\n");
            break;
        case SYSCALL_NOP:
            printf("VMM|INFO: Received NOP syscall\n");
            break;
        default:
            printf("VMM|ERROR: Unknown syscall: syscall number: 0x%lx, PC: 0x%lx\n", syscall, fault_ip);
            return false;
    }
    err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + VM_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    halt(!err);

    return true;
}

// static bool handle_vppi_event()
// {
//     uint64_t ppi_irq = sel4cp_mr_get(seL4_VPPIEvent_IRQ);
//     // We directly inject the interrupt assuming it has been previously registered
//      * If not the interrupt will dropped by the VM
//     // @ivanv: we're assuming only one vcpu
//     int err = vgic_inject_irq(VCPU_ID, ppi_irq);
//     if (err) {
//         printf("VMM|ERROR: VPPI IRQ %lu dropped on vcpu %d", ppi_irq, VCPU_ID);
//          Acknowledge to unmask it as our guest will not use the interrupt 
//         // @ivanv: We're going to assume that we only have one VCPU and that the
//         // cap is the base one.
//         seL4_Error ack_err = seL4_ARM_VCPU_AckVPPI(BASE_VCPU_CAP + VCPU_ID, ppi_irq);
//         if (ack_err) {
//             printf("VMM|ERROR: Failed to ACK VPPI\n");
//             return false;
//         }
//     }

//     return true;
// }

static bool handle_vcpu_fault(sel4cp_msginfo msginfo)
{
    // @ivanv: should this be uint32_t?
    // @ivanv In order to boot, it seems that all we need is SMC handling.
    uint64_t hsr = sel4cp_mr_get(seL4_VCPUFault_HSR);
    uint64_t hsr_ec_class = HSR_EXCEPTION_CLASS(hsr);
    switch (hsr_ec_class) {
        case HSR_SMC_64_EXCEPTION:
            handle_smc(hsr);
            break;
        default:
            // TODO
            printf("VMM|ERROR: unknown SMC exception, EC class: 0x%lx, HSR: 0x%lx\n", hsr_ec_class, hsr);
            break;
    }
    // Below is from the CAmkES VMM handler:
    // if (vcpu->vcpu_arch.unhandled_vcpu_callback) {
    //     // Pass the vcpu fault to library user in case they can handle it
    //     err = new_vcpu_fault(fault, hsr);
    //     if (err) {
    //         ZF_LOGE("Failed to create new fault");
    //         return VM_EXIT_HANDLE_ERROR;
    //     }
    //     err = vcpu->vcpu_arch.unhandled_vcpu_callback(vcpu, hsr, vcpu->vcpu_arch.unhandled_vcpu_callback_cookie);
    //     if (!err) {
    //         return VM_EXIT_HANDLED;
    //     }
    // }
    // print_unhandled_vcpu_hsr(vcpu, hsr);
    // return VM_EXIT_HANDLE_ERROR;
    return true;
}

static int handle_user_exception(sel4cp_msginfo msginfo)
{
    // TODO(ivanv): print out VM name/VCPU id when we have multiple VMs
    uint64_t fault_ip = sel4cp_mr_get(seL4_UserException_FaultIP);
    uint64_t number = sel4cp_mr_get(seL4_UserException_Number);
    printf("VMM|ERROR: Invalid instruction fault at IP: 0x%lx, number: 0x%lx", fault_ip, number);

    // Dump registers
    seL4_UserContext regs;
    seL4_Error ret = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + VM_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    if (ret != seL4_NoError) {
        printf("Failure reading regs, error %d", ret);
        return false;
    } else {
        // I don't know if it's the best idea, but here we'll just dump the
        // registers in the same order they are defined in seL4_UserContext
        printf("Dumping registers:\n");
        // Frame registers
        printf("pc: 0x%lx\n", regs.pc);
        printf("sp:", regs.sp);
        printf("spsr:", regs.spsr);
        printf("x0: 0x%lx\n", regs.x0);
        printf("x1: 0x%lx\n", regs.x1);
        printf("x2: 0x%lx\n", regs.x2);
        printf("x3: 0x%lx\n", regs.x3);
        printf("x4: 0x%lx\n", regs.x4);
        printf("x5: 0x%lx\n", regs.x5);
        printf("x6: 0x%lx\n", regs.x6);
        printf("x7: 0x%lx\n", regs.x7);
        printf("x8: 0x%lx\n", regs.x8);
        printf("x16: 0x%lx\n", regs.x16);
        printf("x17: 0x%lx\n", regs.x17);
        printf("x18: 0x%lx\n", regs.x18);
        printf("x29: 0x%lx\n", regs.x29);
        printf("x30: 0x%lx\n", regs.x30);
        // Other integer registers
        printf("x9: 0x%lx\n", regs.x9);
        printf("x10: 0x%lx\n", regs.x10);
        printf("x11: 0x%lx\n", regs.x11);
        printf("x12: 0x%lx\n", regs.x12);
        printf("x13: 0x%lx\n", regs.x13);
        printf("x14: 0x%lx\n", regs.x14);
        printf("x15: 0x%lx\n", regs.x15);
        printf("x19: 0x%lx\n", regs.x19);
        printf("x20: 0x%lx\n", regs.x20);
        printf("x21: 0x%lx\n", regs.x21);
        printf("x22: 0x%lx\n", regs.x22);
        printf("x23: 0x%lx\n", regs.x23);
        printf("x24: 0x%lx\n", regs.x24);
        printf("x25: 0x%lx\n", regs.x25);
        printf("x26: 0x%lx\n", regs.x26);
        printf("x27: 0x%lx\n", regs.x27);
        printf("x28: 0x%lx\n", regs.x28);
        // TODO(ivanv): print out thread ID registers?
    }

    return true;
}

static int handle_vm_fault()
{
    // @ivanv: TODO
    uint64_t addr = sel4cp_mr_get(seL4_VMFault_Addr);
    uint64_t fsr = sel4cp_mr_get(seL4_VMFault_FSR);
    printf("VMM|INFO: Fault on address 0x%lx, FSR: 0x%lx\n", addr, fsr);

    uint64_t addr_page_aligned = addr & (~(0x1000 - 1));
    printf("addr_page_aligned: 0x%lx\n", addr_page_aligned);
    switch (addr_page_aligned) {
        case 0x8000000:
            printf("Handle GIC distributor fault\n");
    }

    return false;
}

// Structure of the kernel image header is from the documentation at:
// https://www.kernel.org/doc/Documentation/arm64/booting.txt
#define KERNEL_IMAGE_MAGIC 0x644d5241
struct kernel_image_header {
    uint32_t code0;                // Executable code
    uint32_t code1;                // Executable code
    uint64_t text_offset;          // Image load offset, little endian
    uint64_t image_size;           // Effective Image size, little endian
    uint64_t flags;                // kernel flags, little endian
    uint64_t res2;            // reserved
    uint64_t res3;            // reserved
    uint64_t res4;            // reserved
    uint32_t magic;   // Magic number, little endian, "ARM\x64"
    uint32_t res5;                 // reserved (used for PE COFF offset
};

void
init(void)
{
    // Initialise the VMM, the VCPU(s), and start the guest
    printf("VMM|INFO: initialising\n");
    // Initialise our driver for the virtual GIC
    // @ivanv: need to initialise guest VCPU, inject certain IRQs
    // bool success = vgic_init();
    // if (!success) {
    //     printf("VMM|ERROR: could not initialise VGIC driver, stopping...\n");
    //     return;
    // }
    printf("VMM|INFO: completed initialisation\n");

    // @ivanv: This will need to be cleaned up, we can't have these
    // hardcoded
    uint64_t vm_image_entry = 0x40000000 + 0x80000;
    uint64_t dtb_addr = 0x4f000000;

    seL4_UserContext regs = {0};
    regs.x0 = dtb_addr;
    // regs.pc = vm_image_entry;
    // @ivanv: where the fuck does this value come from?
    regs.spsr = 5; // PMODE_EL1h
    seL4_TCB_WriteRegisters(
        BASE_VM_TCB_CAP + VM_ID,
        false, // We'll explcitly start the VM below rather than in this call
        0, // No flags
        SEL4_USER_CONTEXT_SIZE, // Writing to x0, pc, and spsr // @ivanv: for some reason having the number of registers here does not work... (in this case 2)
        &regs
    );

    printf("VMM|INFO: starting VM at 0x%lx, DTB at 0x%lx\n", vm_image_entry, dtb_addr);
    sel4cp_vm_restart(VM_ID, vm_image_entry);
}

void
notified(sel4cp_channel ch)
{
    // We should never get notified in this system.
    printf("VMM|ERROR: Unexpected notification from channel: %d\n", ch);
}

void
fault(sel4cp_vm vm, sel4cp_msginfo msginfo)
{
    // printf("VMM|INFO: received fault message for VM %d\n", vm);
    // Decode the fault and call the appropriate handler
    uint64_t label = sel4cp_msginfo_get_label(msginfo);
    // @ivanv: should be checking the results of the handlers
    switch (label) {
        case seL4_Fault_VMFault:
            handle_vm_fault();
            printf("TODO: handle VM faults\n");
            break;
        case seL4_Fault_UnknownSyscall:
            // This seems to increase the program counter and,
            // handle no-op syscalls or PA to IPA, I don't know what IPA means.
            // Actually, I am very confused about what happens here in the CAmkES VMM.
            // Come back to this...
            handle_unknown_syscall(msginfo);
            break;
        case seL4_Fault_UserException:
            // Seems like it just prints out the regsiters and then replies?
            handle_user_exception(msginfo);
            break;
        case seL4_Fault_VGICMaintenance:
            // This actually does something.
            // This will, I think, get the index of the VCPU list registers that's stored
            // in the VGIC. Then it will set it to not pending.
            // It then calls the VIRQ ack function, which is bullshit since the only ack
            // function that actually does something is for virtual PPIs where it simply
            // invokes seL4_ARM_VCPU_AckVPPI. SGI IRQs aren't acked or anything.
            // handle_vgic_maintenance();
            break;
        case seL4_Fault_VCPUFault:
            // This basically decodes the VCPU fault itself, there are handlers listed in
            // vcpu_fault_handlers.h which some of them don't do anything. Some of them
            // do actually handle the fault.
            // The two main things I can see that happen is handling an SMC fault and
            // advancing (emulating?) a fault.
            // Advancing a fault looks fairly involved, the code is in fault.c
            handle_vcpu_fault(msginfo);
            // printf("VMM|ERROR: VCPU faults are not currently handled by the VMM\n");
            break;
        case seL4_Fault_VPPIEvent:
            // The IRQ number of the PPI is in the first MR.
            // We use that to inject the IRQ (if possible). This is also fairly involved
            // but all of it just invoking the vGIC driver.
            // handle_vppi_event();
            break;
        default:
            printf("VMM|ERROR: unknown fault, stopping VM %d\n", vm);
            sel4cp_vm_stop(vm);
            // @ivanv: print out the actual fault details
    }
}
