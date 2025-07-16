#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/riscv/plic.h>
#include <libvmm/arch/riscv/sbi.h>

/*
 * We emulate SBI based on version 2.0 of the specification.
 * Most legacy extensions are not supported as this was written after those
 * SBI calls were renamed to 'legacy' and given how new the H-extension and RISC-V
 * in general is, I doubt any guests are expecting legacy calls.
 *
 */

// #define DEBUG_SBI

#if defined(DEBUG_SBI)
#define LOG_SBI(...) do{ LOG_VMM("SBI: "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_SBI(...) do{}while(0)
#endif

/* We support version 2.0 of SBI */
#define SBI_SPEC_MAJOR_VERSION 2
#define SBI_SPEC_MINOR_VERSION 0
#define SBI_SPEC_VERSION (SBI_SPEC_MAJOR_VERSION << 24) | (SBI_SPEC_MINOR_VERSION)

#define MACHINE_ARCH_ID 0
#define MACHINE_IMPL_ID 0
#define MACHINE_VENDOR_ID 0

/*
 * List of SBI extensions. Note that what extensions
 * we actually support is a subset of this list.
 */
enum sbi_extensions {
    SBI_EXTENSION_LEGACY_CONSOLE_PUTCHAR = 0x1,
    SBI_EXTENSION_LEGACY_CONSOLE_GETCHAR = 0x2,
    SBI_EXTENSION_BASE = 0x10,
    SBI_EXTENSION_TIMER = 0x54494d45,
    SBI_EXTENSION_HART_STATE_MANAGEMENT = 0x48534d,
    SBI_EXTENSION_SYSTEM_RESET = 0x53525354,
    SBI_EXTENSION_DEBUG_CONSOLE = 0x4442434e,
};

enum sbi_debug_console_function {
    SBI_DEBUG_CONSOLE_WRITE = 0,
};

enum sbi_timer_function {
    SBI_TIMER_SET = 0,
};

enum sbi_base_function {
    SBI_BASE_GET_SBI_SPEC_VERSION = 0,
    SBI_BASE_GET_SBI_IMPL_ID = 1,
    SBI_BASE_GET_SBI_IMPL_VERSION = 2,
    SBI_BASE_PROBE_EXTENSION_ID = 3,
    SBI_BASE_GET_MACHINE_VENDOR_ID = 4,
    SBI_BASE_GET_MACHINE_ARCH_ID = 5,
    SBI_BASE_GET_MACHINE_IMPL_ID = 6,
};

enum sbi_system_reset_function {
    SBI_SYSTEM_RESET = 0,
};

enum sbi_system_reset_type {
    SBI_SYSTEM_RESET_SHUTDOWN = 0,
    SBI_SYSTEM_RESET_COLD_REBOOT = 1,
    SBI_SYSTEM_RESET_WARM_REBOOT = 2,
};

enum sbi_return_code {
    SBI_SUCCESS = 0,
    SBI_ERR_FAILED = -1,
    SBI_ERR_NOT_SUPPORTED = -2,
    SBI_ERR_INVALID_PARAM = -3,
    SBI_ERR_DENIED = -4,
    SBI_ERR_INVALID_ADDRESS = -5,
    SBI_ERR_ALREADY_AVAILABLE = -6,
    SBI_ERR_ALREADY_STARTED = -7,
    SBI_ERR_ALREADY_STOPPED = -8,
    SBI_ERR_NO_SHMEM = -9,
};

static bool sbi_system_reset(size_t vcpu_id, seL4_Word sbi_fid, seL4_UserContext *regs)
{
    switch (sbi_fid) {
    case SBI_SYSTEM_RESET: {
        uint32_t reset_type = regs->a0;
        uint32_t reset_reason = regs->a1;
        switch (reset_type) {
        case SBI_SYSTEM_RESET_SHUTDOWN:
            LOG_VMM("guest requested shutdown via SBI (reset reason: 0x%x)\n", reset_reason);
            guest_stop(vcpu_id);
            return true;
        default:
            LOG_VMM_ERR("unhandled SBI system reset type: 0x%x with reset reason: 0x%x\n", reset_type, reset_reason);
            return false;
        }
        return true;
    }
    default:
        LOG_VMM_ERR("invalid SBI system reset FID 0x%lx\n", sbi_fid);
        return false;
    }
}

static bool sbi_debug_console(seL4_Word sbi_fid, seL4_UserContext *regs)
{
    switch (sbi_fid) {
    case SBI_DEBUG_CONSOLE_WRITE: {
        uint32_t num_bytes = regs->a0;
        uint64_t base_addr_lo = regs->a1;
        uint64_t base_addr_hi = regs->a2 << 32;
        char *bytes = (char *)(base_addr_lo | (base_addr_hi << 32));
        for (int i = 0; i < num_bytes; i++) {
            printf("%c", bytes[i]);
        }
        return true;
    }
    default:
        LOG_VMM_ERR("invalid SBI debug console FID 0x%lx\n", sbi_fid);
        return false;
    }
}

static bool sbi_timer(size_t vcpu_id, seL4_Word sbi_fid, seL4_UserContext *regs)
{
    switch (sbi_fid) {
    case SBI_TIMER_SET: {
        /* stime_value is always 64-bit */
        uint64_t stime_value = regs->a0;

        seL4_RISCV_VCPU_ReadRegs_t res = seL4_RISCV_VCPU_ReadRegs(BASE_VCPU_CAP + vcpu_id, seL4_VCPUReg_SIP);
        assert(!res.error);
        res.value &= ~SIP_TIMER;
        seL4_RISCV_VCPU_WriteRegs(BASE_VCPU_CAP + vcpu_id, seL4_VCPUReg_SIP, res.value);

        uint64_t curr_time = 0;
        asm volatile("rdtime %0" : "=r"(curr_time));
        if (curr_time >= stime_value) {
            /* In this case we are already past the target time, so we immediately inject an IRQ. */
            // TODO: check return value
            plic_inject_timer_irq(vcpu_id);
        } else {
            seL4_Error err = seL4_RISCV_VCPU_WriteRegs(BASE_VCPU_CAP + vcpu_id, seL4_VCPUReg_TIMER, stime_value);
            assert(!err);
        }
        // LOG_VMM("setting timer stime_value: 0x%lx, curr_time: 0x%lx\n", stime_value, curr_time);

        regs->a0 = SBI_SUCCESS;

        return true;
    }
    default:
        return false;
    }
}

static bool sbi_base(seL4_Word sbi_fid, seL4_UserContext *regs)
{
    switch (sbi_fid) {
    case SBI_BASE_GET_SBI_SPEC_VERSION:
        regs->a0 = SBI_SUCCESS;
        regs->a1 = SBI_SPEC_VERSION;
        return true;
    case SBI_BASE_GET_SBI_IMPL_ID:
    case SBI_BASE_GET_SBI_IMPL_VERSION:
        /* We do not emulate a specific SBI implementation. */
        regs->a0 = SBI_ERR_NOT_SUPPORTED;
        return true;
    case SBI_BASE_GET_MACHINE_VENDOR_ID:
        regs->a0 = SBI_SUCCESS;
        regs->a1 = MACHINE_VENDOR_ID;
        return true;
    case SBI_BASE_GET_MACHINE_ARCH_ID:
        regs->a0 = SBI_SUCCESS;
        regs->a1 = MACHINE_ARCH_ID;
        return true;
    case SBI_BASE_GET_MACHINE_IMPL_ID:
        regs->a0 = SBI_SUCCESS;
        regs->a1 = MACHINE_IMPL_ID;
        return true;
    case SBI_BASE_PROBE_EXTENSION_ID: {
        seL4_Word probe_eid = regs->a0;
        switch (probe_eid) {
        case SBI_EXTENSION_BASE:
        case SBI_EXTENSION_TIMER:
        case SBI_EXTENSION_HART_STATE_MANAGEMENT:
        case SBI_EXTENSION_SYSTEM_RESET:
        case SBI_EXTENSION_DEBUG_CONSOLE:
            regs->a0 = SBI_SUCCESS;
            regs->a1 = 1;
            return true;
        default:
            LOG_VMM("guest probed for unhandled SBI extension (EID 0x%lx)\n", probe_eid);
            regs->a0 = SBI_ERR_NOT_SUPPORTED;
            return true;
        }
    }
    default:
        LOG_VMM_ERR("could not handle SBI base extension call with FID: 0x%lx\n", sbi_fid);
        break;
    }

    return false;
}

bool fault_handle_sbi(size_t vcpu_id, seL4_UserContext *regs)
{
    /* SBI extension ID */
    seL4_Word sbi_eid = regs->a7;
    /* SBI function ID for the given extension */
    seL4_Word sbi_fid = regs->a6;

    LOG_SBI("handling EID 0x%lx, FID 0x%lx\n", sbi_eid, sbi_fid);

    bool success = false;
    switch (sbi_eid) {
    case SBI_EXTENSION_BASE:
        success = sbi_base(sbi_fid, regs);
        break;
    case SBI_EXTENSION_TIMER:
        success = sbi_timer(vcpu_id, sbi_fid, regs);
        break;
    case SBI_EXTENSION_LEGACY_CONSOLE_PUTCHAR:
        printf("%c", regs->a0);
        regs->a0 = SBI_SUCCESS;
        success = true;
        break;
    case SBI_EXTENSION_LEGACY_CONSOLE_GETCHAR:
        /* Not supported by our SBI emulation. On legacy SBI we are supposed to just return -1 for failure. */
        regs->a0 = -1;
        success = true;
        break;
    case SBI_EXTENSION_DEBUG_CONSOLE:
        success = sbi_debug_console(sbi_fid, regs);
        break;
    case SBI_EXTENSION_SYSTEM_RESET:
        LOG_VMM("handling system reset\n");
        success = sbi_system_reset(vcpu_id, sbi_fid, regs);
        break;
    default: {
        LOG_VMM_ERR("unhandled sbi_eid: 0x%lx, sbi_fid 0x%lx\n", sbi_eid, sbi_fid);
    }
    }

    if (success) {
        regs->pc += 4;
        seL4_Error err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + vcpu_id, false, 0, SEL4_USER_CONTEXT_SIZE, regs);
        assert(err == seL4_NoError);
        return (err == seL4_NoError);
    }

    return success;
}
