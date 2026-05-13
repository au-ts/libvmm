/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <sddf/util/util.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/cpuid.h>
#include <libvmm/arch/x86_64/msr.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/hpet.h>
#include <libvmm/arch/x86_64/fpu.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/arch/x86_64/memory_space.h>
#include <libvmm/arch/x86_64/guest_time.h>
#include <libvmm/guest.h>
#include <sel4/arch/vmenter.h>

/* Documents referenced:
 * [1] seL4: include/arch/x86/arch/object/vcpu.h
 * [2] Title: Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4 Order Number: 325462-080US June 2023
 * [3] https://wiki.osdev.org/Interrupt_Vector_Table
 */

/* Exit reasons.
 * From [1]
 */
enum exit_reasons {
    EXCEPTION_OR_NMI = 0x00,
    EXTERNAL_INTERRUPT = 0x01,
    TRIPLE_FAULT = 0x02,
    INIT_SIGNAL = 0x03,
    SIPI = 0x04,
    /*IO_SMI = 0x05,
     *   OTHER_SMI = 0x06,*/
    INTERRUPT_WINDOW = 0x07,
    NMI_WINDOW = 0x08,
    TASK_SWITCH = 0x09,
    CPUID = 0x0A,
    GETSEC = 0x0B,
    HLT = 0x0C,
    INVD = 0x0D,
    INVLPG = 0x0E,
    RDPMC = 0x0F,
    RDTSC = 0x10,
    RSM = 0x11,
    VMCALL = 0x12,
    VMCLEAR = 0x13,
    VMLAUNCH = 0x14,
    VMPTRLD = 0x15,
    VMPTRST = 0x16,
    VMREAD = 0x17,
    VMRESUME = 0x18,
    VMWRITE = 0x19,
    VMXOFF = 0x1A,
    VMXON = 0x1B,
    CONTROL_REGISTER = 0x1C,
    MOV_DR = 0x1D,
    IO = 0x1E,
    RDMSR = 0x1F,
    WRMSR = 0x20,
    INVALID_GUEST_STATE = 0x21,
    MSR_LOAD_FAIL = 0x22,
    /* 0x23 */
    MWAIT = 0x24,
    MONITOR_TRAP_FLAG = 0x25,
    /* 0x26 */
    MONITOR = 0x27,
    PAUSE = 0x28,
    MACHINE_CHECK = 0x29,
    /* 0x2A */
    TPR_BELOW_THRESHOLD = 0x2B,
    APIC_ACCESS = 0x2C,
    VIRTUALIZED_EOI = 0x2D,
    GDTR_OR_IDTR = 0x2E,
    LDTR_OR_TR = 0x2F,
    EPT_VIOLATION = 0x30,
    EPT_MISCONFIGURATION = 0x31,
    INVEPT = 0x32,
    RDTSCP = 0x33,
    VMX_PREEMPTION_TIMER = 0x34,
    INVVPID = 0x35,
    WBINVD = 0x36,
    XSETBV = 0x37,
    APIC_WRITE = 0x38,
    NUM_EXIT_REASONS = 0x39,
};

/* [2] "Table C-1. Basic Exit Reasons" */
static char *exit_reason_strs[] = { "Exception or non-maskable interrupt (NMI)",
                                    "External interrupt",
                                    "Triple Fault",
                                    "INIT signal",
                                    "Start-up IPI (SIPI)",
                                    "I/O system-management interrupt (SMI)",
                                    "Other SMI",
                                    "Interrupt window",
                                    "NMI window",
                                    "Task switch",
                                    "CPUID",
                                    "GETSEC",
                                    "HLT",
                                    "INVD",
                                    "INVLPG",
                                    "RDPMC",
                                    "RDTSC",
                                    "RSM",
                                    "VMCALL",
                                    "VMCLEAR",
                                    "VMLAUNCH",
                                    "VMPTRLD",
                                    "VMPTRST",
                                    "VMREAD",
                                    "VMRESUME",
                                    "VMWRITE",
                                    "VMXOFF",
                                    "VMXON",
                                    "Control-register accesses",
                                    "MOV DR",
                                    "I/O instruction",
                                    "RDMSR",
                                    "WRMSR",
                                    "VM-entry failure due to invalid guest state",
                                    "VM-entry failure due to MSR loading",
                                    "",
                                    "MWAIT",
                                    "Monitor trap flag",
                                    "",
                                    "MONITOR",
                                    "PAUSE",
                                    "VM-entry failure due to machine-check event",
                                    "",
                                    "TPR below threshold",
                                    "APIC access",
                                    "Virtualized EOI",
                                    "Access to GDTR or IDTR",
                                    "Access to LDTR or TR",
                                    "EPT violation",
                                    "EPT misconfiguration",
                                    "INVEPT",
                                    "RDTSCP",
                                    "VMX-preemption timer expired",
                                    "INVVPID",
                                    "WBINVD or WBNOINVD",
                                    "XSETBV",
                                    "APIC Write" };

_Static_assert(sizeof(exit_reason_strs) / sizeof(char *) == NUM_EXIT_REASONS,
               "Exit reason strings table is not correct length");

char *fault_to_string(int exit_reason)
{
    assert(exit_reason < NUM_EXIT_REASONS);
    assert(exit_reason_strs[exit_reason] != NULL);
    return exit_reason_strs[exit_reason];
}

bool fault_is_trap_like(int exit_reason)
{
    switch (exit_reason) {
    case APIC_WRITE:
    case VIRTUALIZED_EOI:
    case VMX_PREEMPTION_TIMER:
    case INTERRUPT_WINDOW:
        return true;
    default:
        return false;
    }
}

struct ept_exception_handler {
    uintptr_t base;
    uintptr_t end;
    ept_exception_callback_t callback;
    void *cookie;
};
#define MAX_EPT_EXCEPTION_HANDLERS 16
static struct ept_exception_handler registered_ept_exception_handlers[MAX_EPT_EXCEPTION_HANDLERS];
static size_t ept_exception_handler_index = 0;

bool fault_update_ept_exception_handler(uintptr_t base, uintptr_t new_base)
{
    for (int i = 0; i < ept_exception_handler_index; i++) {
        struct ept_exception_handler *curr = &registered_ept_exception_handlers[i];
        if (curr->base == base) {
            curr->base = new_base;
            return true;
        }
    }

    LOG_VMM("failed to update EPT exception handler, none exists for base 0x%lx\n", base);

    return false;
}

bool fault_register_ept_exception_handler(uintptr_t base, size_t size, ept_exception_callback_t callback, void *cookie)
{
    if (ept_exception_handler_index == MAX_EPT_EXCEPTION_HANDLERS - 1) {
        LOG_VMM_ERR("maximum number of EPT exception handlers registered");
        return false;
    }

    if (size == 0) {
        LOG_VMM_ERR("registered EPT exception handler with size 0\n");
        return false;
    }

    for (int i = 0; i < ept_exception_handler_index; i++) {
        struct ept_exception_handler *curr = &registered_ept_exception_handlers[i];
        if (!(base >= curr->end || base + size <= curr->base)) {
            LOG_VMM_ERR("EPT exception handler [0x%lx..0x%lx), overlaps with another handler [0x%lx..0x%lx)\n", base,
                        base + size, curr->base, curr->end);
            return false;
        }
    }

    registered_ept_exception_handlers[ept_exception_handler_index] = (struct ept_exception_handler) {
        .base = base,
        .end = base + size,
        .callback = callback,
        .cookie = cookie,
    };
    ept_exception_handler_index += 1;

    return true;
}

static bool handle_ept_fault(seL4_VCPUContext *vctx, seL4_Word qualification, decoded_instruction_ret_t decoded_ins)
{
    uint64_t addr = microkit_mr_get(SEL4_VMENTER_FAULT_GUEST_PHYSICAL_MR);

    if (addr >= IOAPIC_GPA && addr < IOAPIC_GPA + IOAPIC_SIZE) {
        LOG_FAULT("handling IO APIC 0x%lx\n", addr);
        return ioapic_fault_handle(vctx, addr - IOAPIC_GPA, qualification, decoded_ins);
    } else if (addr >= HPET_GPA && addr < HPET_GPA + HPET_SIZE) {
        LOG_FAULT("handling HPET 0x%lx\n", addr);
        return hpet_fault_handle(vctx, addr - HPET_GPA, qualification, decoded_ins);
#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
    } else if (addr >= LAPIC_GPA && addr < LAPIC_GPA + LAPIC_SIZE) {
        LOG_FAULT("handling LAPIC 0x%lx\n", addr);
        return lapic_fault_handle(vctx, addr - LAPIC_GPA, qualification, decoded_ins);
#endif
    } else {
        LOG_FAULT("handling other EPT 0x%lx\n", addr);
        for (int i = 0; i < MAX_EPT_EXCEPTION_HANDLERS; i++) {
            uintptr_t base = registered_ept_exception_handlers[i].base;
            uintptr_t end = registered_ept_exception_handlers[i].end;
            ept_exception_callback_t callback = registered_ept_exception_handlers[i].callback;
            void *cookie = registered_ept_exception_handlers[i].cookie;
            if (addr >= base && addr < end) {
                bool success = callback(0, addr - base, qualification, decoded_ins, vctx, cookie);
                if (!success) {
                    LOG_VMM_ERR("registered EPT exception handler for region [0x%lx..0x%lx) at address "
                                "0x%lx failed\n",
                                base, end, addr);
                }

                return success;
            }
        }

        LOG_VMM_ERR("failed to find EPT handler for address 0x%lx\n", addr);
    }

    return false;
}

struct pio_exception_handler {
    uint16_t base;
    uint16_t end;
    pio_exception_callback_t callback;
    void *cookie;
};
#define MAX_PIO_EXCEPTION_HANDLERS 16
static struct pio_exception_handler registered_pio_exception_handlers[MAX_PIO_EXCEPTION_HANDLERS];
static size_t pio_exception_handler_index = 0;

bool fault_register_pio_exception_handler(uint16_t base, uint16_t size, pio_exception_callback_t callback, void *cookie)
{
    if (pio_exception_handler_index == MAX_PIO_EXCEPTION_HANDLERS - 1) {
        LOG_VMM_ERR("maximum number of PIO exception handlers registered");
        return false;
    }

    if (size == 0) {
        LOG_VMM_ERR("registered PIO exception handler with size 0\n");
        return false;
    }

    uint64_t size_64 = (uint64_t)base + (uint64_t)size;
    if (size_64 > 0xffff) {
        LOG_VMM_ERR("base + size = 0x%lx exceed uint16_t max\n", size_64);
        return false;
    }

    for (int i = 0; i < pio_exception_handler_index; i++) {
        struct pio_exception_handler *curr = &registered_pio_exception_handlers[i];
        if (!(base >= curr->end || base + size <= curr->base)) {
            LOG_VMM_ERR("PIO exception handler [0x%hx..0x%hx), overlaps with another handler [0x%hx..0x%hx)\n", base,
                        base + size, curr->base, curr->end);
            return false;
        }
    }

    registered_pio_exception_handlers[pio_exception_handler_index] = (struct pio_exception_handler) {
        .base = base,
        .end = base + size,
        .callback = callback,
        .cookie = cookie,
    };
    pio_exception_handler_index += 1;

    return true;
}

static bool handle_pio_fault(seL4_VCPUContext *vctx, seL4_Word qualification)
{
    uint16_t port_addr = pio_fault_addr(qualification);

    for (int i = 0; i < MAX_PIO_EXCEPTION_HANDLERS; i++) {
        uint16_t base = registered_pio_exception_handlers[i].base;
        uint16_t end = registered_pio_exception_handlers[i].end;
        pio_exception_callback_t callback = registered_pio_exception_handlers[i].callback;
        void *cookie = registered_pio_exception_handlers[i].cookie;
        if (port_addr >= base && port_addr < end) {
            bool success = callback(0, port_addr - base, qualification, vctx, cookie);
            if (!success) {
                LOG_VMM_ERR("registered PIO exception handler for region [0x%hx..0x%hx) at address "
                            "0x%hx failed\n",
                            base, end, port_addr);
            }

            return success;
        }
    }

    emulate_ioport_noop_access(vctx, qualification);
    return true;
}

bool fault_handle(size_t vcpu_id)
{
    bool success = false;
    decoded_instruction_ret_t decoded_ins;

    vcpu_init_exit_state(false);

    seL4_Word f_reason = vcpu_exit_get_reason();
    seL4_Word qualification = vcpu_exit_get_qualification();
    seL4_Word rip = vcpu_exit_get_rip();
    seL4_VCPUContext *vctx = vcpu_exit_get_context();

    LOG_FAULT("handling vmexit reason %s\n", fault_to_string(f_reason));

    switch (f_reason) {
    case CPUID:
        success = emulate_cpuid(vctx);
        break;
    case RDMSR:
        success = emulate_rdmsr(vctx);
        break;
    case WRMSR:
        success = emulate_wrmsr(vctx);
        break;
    case EPT_VIOLATION:
        decoded_ins = decode_instruction(vcpu_id, rip);
        if (decoded_ins.type == INSTRUCTION_DECODE_FAIL) {
            success = false;
        } else {
            success = handle_ept_fault(vctx, qualification, decoded_ins);
        }
        break;
    case IO:
        success = handle_pio_fault(vctx, qualification);
        break;
    case XSETBV:
        success = emulate_xsetbv(vctx);
        break;
    case VMX_PREEMPTION_TIMER:
        guest_time_handle_timer_ntfn();
        success = true;
        break;
#if APIC_VIRT_LEVEL == APIC_VIRT_LEVEL_APICV
    case VIRTUALIZED_EOI: {
        uint8_t eoi_vector = qualification;
        /* If we get here then the guest has ack'ed a passed through I/O APIC irq,
         * find and run the ack function, the clear the vector from the bitmap
         */
        // @billn the whole clearing vector from bitmap thing could be done more efficiently, e.g.
        // only do it when the guest reprogram the vector

        success = ioapic_ack_passthrough_irq(eoi_vector);
        break;
    }
    case APIC_WRITE: {
        /* [2] "31.4.3.3 APIC-Write VM Exits"
         * "The exit qualification is the page offset of the write access that led to the VM exit."
         */
        uint64_t lapic_reg_offset = qualification;
        success = lapic_write_fault_handle(lapic_reg_offset, lapic_read_reg(lapic_reg_offset));
        break;
    }
    case APIC_ACCESS: {
        /* [2] "Table 29-6. Exit Qualification for APIC-Access VM Exits from Linear Accesses and Guest-Physical Accesses" */
        uint16_t offset = qualification & 0x7ff;
        uint8_t access_type = (qualification >> 12) & 0xf;

        // only handle reads and writes due to instruction execution
        if (access_type == 0) {
            uint32_t data;
            success = lapic_read_fault_handle(offset, &data);
            decoded_ins = decode_instruction(vcpu_id, rip, ins_len);
            assert(decoded_ins.type == INSTRUCTION_MEMORY);
            uint64_t *vctx_raw = (uint64_t *)&vctx;
            vctx_raw[decoded_ins.decoded.memory_instruction.target_reg] = data;
        } else if (access_type == 1) {
            decoded_ins = decode_instruction(vcpu_id, rip, ins_len);
            assert(decoded_ins.type == INSTRUCTION_MEMORY);
            // TODO: probably do not have this assert
            assert(mem_access_width_to_bytes(decoded_ins) == 4);
            uint64_t *vctx_raw = (uint64_t *)&vctx;
            // TODO: probably use wrapper for getting write value.
            success = lapic_write_fault_handle(offset, vctx_raw[decoded_ins.decoded.memory_instruction.target_reg]);
        } else {
            LOG_VMM_ERR("unsupported access type %d for apic access vm exit\n", access_type);
            success = false;
        }
        break;
    }
#else
    case INTERRUPT_WINDOW:
        lapic_maintenance();
        success = true;
        break;
#endif
    default:
        LOG_VMM_ERR("unhandled fault: 0x%lx\n", f_reason);
    };

    if (success) {
        unsigned rip_additive = 0;
        if (!fault_is_trap_like(f_reason)) {
            if (f_reason == EPT_VIOLATION) {
                /* Note that for EPT faults the instruction length won't be valid.
                * See "30.2.5 Information for VM Exits Due to Instruction Execution"
                */
                rip_additive = decoded_ins.len;
            } else {
                rip_additive = vcpu_exit_get_instruction_len();
            }
        }
        vcpu_exit_advance_rip(rip_additive);
        vcpu_exit_resume();
    } else if (!success && (f_reason == RDMSR || f_reason == WRMSR || f_reason == XSETBV)) {
        /* [2] "RDMSR—Read From Model Specific Register":
         * "Specifying a reserved or unimplemented MSR address in ECX will also cause a general protection exception." */
        microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_ENTRY_EXCEPTION_ERROR_CODE, 0);

        /* [2] "Table 26-17. Format of the VM-Entry Interruption-Information Field" and [3] */
        uint64_t interruption = GP_VECTOR; // General protection fault vector
        interruption |= 3ull << 8; // Hardware exception
        interruption |= BIT(11); // deliver error code
        interruption |= BIT(31); // valid

        vcpu_exit_inject_irq(interruption);
        /* Don't advance rip */
        vcpu_exit_resume();
    } else if (!success) {
        LOG_VMM_ERR("failed handling fault: '%s' (0x%lx)\n", fault_to_string(f_reason), f_reason);
        vcpu_print_regs(vcpu_id);
        LOG_VMM_ERR("VCPU will not be resumed.\n");
    }

    return success;
}