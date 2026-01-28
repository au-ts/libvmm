/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <sddf/util/util.h>
#include <libvmm/guest.h>
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
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <sel4/arch/vmenter.h>

/* Documents referenced:
 * [1] seL4: include/arch/x86/arch/object/vcpu.h
 * [2] Title: Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4 Order Number: 325462-080US June 2023
 *   [2a] Location: Table C-1 "VMX BASIC EXIT REASONS", page: "Vol. 3D C-1"
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
    NUM_EXIT_REASONS = 0x38,
};

/* [2a] */
static char *exit_reason_strs[NUM_EXIT_REASONS] = {
    "Exception or non-maskable interrupt (NMI)",
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
};

char *fault_to_string(int exit_reason)
{
    assert(exit_reason < NUM_EXIT_REASONS);
    return exit_reason_strs[exit_reason];
}

// @billn dedup
#define LAPIC_BASE 0xFEE00000
#define LAPIC_SIZE 0x1000

#define IOAPIC_BASE 0xFEC00000
#define IOAPIC_SIZE 0x1000

#define HPET_BASE 0xfed00000
#define HPET_SIZE 0x1000

/* Table 28-7. Exit Qualification for EPT Violations */
#define EPT_VIOLATION_READ (1 << 0)
#define EPT_VIOLATION_WRITE (1 << 1)

bool ept_fault_is_read(seL4_Word qualification)
{
    return qualification & EPT_VIOLATION_READ;
}

bool ept_fault_is_write(seL4_Word qualification)
{
    return qualification & EPT_VIOLATION_WRITE;
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

bool fault_update_ept_exception_handler(uintptr_t base, uintptr_t new_base) {
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

static bool handle_ept_fault(seL4_VCPUContext *vctx, seL4_Word qualification, memory_instruction_data_t decoded_mem_ins)
{
    uint64_t addr = microkit_mr_get(SEL4_VMENTER_FAULT_GUEST_PHYSICAL_MR);
    // LOG_VMM("handling EPT fault on GPA 0x%lx, qualification: 0x%lx\n", addr, qualification);

    if (addr >= LAPIC_BASE && addr < LAPIC_BASE + LAPIC_SIZE) {
        // No log fault here, as the local APIC timer will just fill the terminal...
        return lapic_fault_handle(vctx, addr - LAPIC_BASE, qualification, decoded_mem_ins);
    } else if (addr >= IOAPIC_BASE && addr < IOAPIC_BASE + IOAPIC_SIZE) {
        LOG_FAULT("handling IO APIC 0x%lx\n", addr);
        return ioapic_fault_handle(vctx, addr - IOAPIC_BASE, qualification, decoded_mem_ins);
    } else if (addr >= HPET_BASE && addr < HPET_BASE + HPET_SIZE) {
        LOG_FAULT("handling HPET 0x%lx\n", addr);
        return hpet_fault_handle(vctx, addr - HPET_BASE, qualification, decoded_mem_ins);
    // } else if (addr >= ECAM_GPA && addr < ECAM_GPA + ECAM_SIZE) {
    //     return pci_x86_emulate_ecam_access(vctx, addr - ECAM_GPA, qualification, decoded_mem_ins);
    } else {
        LOG_FAULT("handling other EPT 0x%lx\n", addr);
        for (int i = 0; i < MAX_EPT_EXCEPTION_HANDLERS; i++) {
            uintptr_t base = registered_ept_exception_handlers[i].base;
            uintptr_t end = registered_ept_exception_handlers[i].end;
            ept_exception_callback_t callback = registered_ept_exception_handlers[i].callback;
            void *cookie = registered_ept_exception_handlers[i].cookie;
            if (addr >= base && addr < end) {
                bool success = callback(0, addr - base, qualification, decoded_mem_ins, vctx, cookie);
                if (!success) {
                    LOG_VMM_ERR("registered EPT exception handler for region [0x%lx..0x%lx) at address "
                                "0x%lx failed\n",
                                base, end, addr);
                }

                return success;
            }
        }
    }

    // LOG_VMM("done\n");

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
            LOG_VMM_ERR("PIO exception handler [0x%lx..0x%lx), overlaps with another handler [0x%lx..0x%lx)\n", base,
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
    uint16_t port_addr = (qualification >> 16) & 0xffff;
    // TODO: pass access width to the callbacks?
    // ioport_access_width_t access_width = (ioport_access_width_t)(qualification & 0x7);

    if (port_addr >= 0x3f8 && port_addr <= 0x3f8 + 8) {
    } else if (port_addr >= 0x2f8 && port_addr <= 0x2f8 + 8) {
    } else if (port_addr >= 0x3e8 && port_addr <= 0x3e8 + 8) {
    } else if (port_addr >= 0x2ef && port_addr <= 0x2ef + 8) {
    } else {
        LOG_FAULT("handling PIO 0x%lx\n", port_addr);
    }

    for (int i = 0; i < MAX_PIO_EXCEPTION_HANDLERS; i++) {
        uint16_t base = registered_pio_exception_handlers[i].base;
        uint16_t end = registered_pio_exception_handlers[i].end;
        pio_exception_callback_t callback = registered_pio_exception_handlers[i].callback;
        void *cookie = registered_pio_exception_handlers[i].cookie;
        if (port_addr >= base && port_addr < end) {
            bool success = callback(0, port_addr - base, qualification, vctx, cookie);
            if (!success) {
                LOG_VMM_ERR("registered PIO exception handler for region [0x%lx..0x%lx) at address "
                            "0x%lx failed\n",
                            base, end, port_addr);
            }

            return success;
        }
    }

    return emulate_ioports(vctx, qualification);
}

bool fault_handle(size_t vcpu_id, uint64_t *new_rip)
{
    bool success = false;
    decoded_instruction_ret_t decoded_ins;

    seL4_Word f_reason = microkit_mr_get(SEL4_VMENTER_FAULT_REASON_MR);
    seL4_Word ins_len = microkit_mr_get(SEL4_VMENTER_FAULT_INSTRUCTION_LEN_MR);
    seL4_Word qualification = microkit_mr_get(SEL4_VMENTER_FAULT_QUALIFICATION_MR);
    seL4_Word rip = microkit_mr_get(SEL4_VMENTER_CALL_EIP_MR);

    seL4_VCPUContext vctx;
    vctx.eax = microkit_mr_get(SEL4_VMENTER_FAULT_EAX);
    vctx.ebx = microkit_mr_get(SEL4_VMENTER_FAULT_EBX);
    vctx.ecx = microkit_mr_get(SEL4_VMENTER_FAULT_ECX);
    vctx.edx = microkit_mr_get(SEL4_VMENTER_FAULT_EDX);
    vctx.esi = microkit_mr_get(SEL4_VMENTER_FAULT_ESI);
    vctx.edi = microkit_mr_get(SEL4_VMENTER_FAULT_EDI);
    vctx.ebp = microkit_mr_get(SEL4_VMENTER_FAULT_EBP);
    vctx.r8 = microkit_mr_get(SEL4_VMENTER_FAULT_R8);
    vctx.r9 = microkit_mr_get(SEL4_VMENTER_FAULT_R9);
    vctx.r10 = microkit_mr_get(SEL4_VMENTER_FAULT_R10);
    vctx.r11 = microkit_mr_get(SEL4_VMENTER_FAULT_R11);
    vctx.r12 = microkit_mr_get(SEL4_VMENTER_FAULT_R12);
    vctx.r13 = microkit_mr_get(SEL4_VMENTER_FAULT_R13);
    vctx.r14 = microkit_mr_get(SEL4_VMENTER_FAULT_R14);
    vctx.r15 = microkit_mr_get(SEL4_VMENTER_FAULT_R15);

    switch (f_reason) {
    case CPUID:
        success = emulate_cpuid(&vctx);
        break;
    case RDMSR:
        success = emulate_rdmsr(&vctx);
        break;
    case WRMSR:
        success = emulate_wrmsr(&vctx);
        break;
    case EPT_VIOLATION:
        decoded_ins = decode_instruction(vcpu_id, rip, ins_len);
        assert(decoded_ins.type == INSTRUCTION_MEMORY);
        success = handle_ept_fault(&vctx, qualification, decoded_ins.decoded.memory_instruction);
        break;
    case IO:
        success = handle_pio_fault(&vctx, qualification);
        break;
    case INTERRUPT_WINDOW:
        lapic_maintenance();
        success = true;
        break;
    case XSETBV:
        success = true;
        break;
    default:
        LOG_VMM_ERR("unhandled fault: 0x%x\n", f_reason);
    };

    *new_rip = rip;
    if (success && f_reason != INTERRUPT_WINDOW) {
        microkit_vcpu_x86_write_regs(vcpu_id, &vctx);
        *new_rip = rip + ins_len;
    } else if (!success) {
        LOG_VMM_ERR("failed handling fault: 0x%x\n", f_reason);
        vcpu_print_regs(vcpu_id);
    }

    return success;
}