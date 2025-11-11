#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/fault.h>

// https://wiki.osdev.org/APIC

/* Uncomment this to enable debug logging */
#define DEBUG_APIC

#if defined(DEBUG_APIC)
#define LOG_APIC(...) do{ printf("%s|APIC: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_APIC(...) do{}while(0)
#endif

#define REG_LAPIC_ID 0x20
#define REG_LAPIC_REV 0x30
#define REG_LAPIC_SVR 0xf0

#define REG_IOAPIC_IOREGSEL_OFF 0x0
#define REG_IOAPIC_IOWIN_OFF 0x10

struct lapic_regs {
    uint32_t id;
    uint32_t svr;
};

struct lapic_regs lapic_regs;

struct ioapic_regs {
    uint32_t selected_reg;
};

struct ioapic_regs ioapic_regs;

bool lapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification, memory_instruction_data_t decoded_mem_ins) {
    // TODO: support other alignments?
    assert(offset % 4 == 0);
    if (ept_fault_is_read(qualification)) {
        LOG_APIC("handling LAPIC read at offset 0x%lx\n", offset);
    } else if (ept_fault_is_write(qualification)) {
        LOG_APIC("handling LAPIC write at offset 0x%lx\n", offset);
    }

    uint64_t *vctx_raw = (uint64_t *) vctx;

    if (ept_fault_is_read(qualification)) {
        switch (offset) {
            case REG_LAPIC_ID:
                vctx_raw[decoded_mem_ins.target_reg] = 0;
                break;
            case REG_LAPIC_REV:
                // Figure 11-7. Local APIC Version Register
                // 32 local vector table entries. @billn is enough??
                vctx_raw[decoded_mem_ins.target_reg] = 0x10 | 32 << 16;
                break;
            case REG_LAPIC_SVR:
                // Figure 11-23. Spurious-Interrupt Vector Register (SVR)
                vctx_raw[decoded_mem_ins.target_reg] = 0xff; // reset value
                break;
            default:
                LOG_VMM_ERR("Reading unknown LAPIC register offset 0x%x\n", offset);
                return false;
        }
    } else {
        switch (offset) {
            default:
                LOG_VMM_ERR("Writing unknown LAPIC register offset 0x%x, value 0x%lx\n", offset, vctx_raw[decoded_mem_ins.target_reg]);
                return false;
        }
    }

    return true;
}

bool ioapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification, memory_instruction_data_t decoded_mem_ins) {
    if (ept_fault_is_read(qualification)) {
        LOG_APIC("handling I/O APIC read at offset 0x%lx\n", offset);
    } else if (ept_fault_is_write(qualification)) {
        LOG_APIC("handling I/O APIC write at offset 0x%lx\n", offset);
    }

    uint64_t *vctx_raw = (uint64_t *) vctx;

    if (ept_fault_is_read(qualification)) {
        if (offset == REG_IOAPIC_IOWIN_OFF) {
            switch (ioapic_regs.selected_reg) {
                case 0:
                    vctx_raw[decoded_mem_ins.target_reg] = 0;
                    break;
                case 1:
                    // supports 64 irqs
                    // @billn move to #define
                    vctx_raw[decoded_mem_ins.target_reg] = 64 << 16;
                    break;
                case 2:
                    vctx_raw[decoded_mem_ins.target_reg] = 0;
                    break;
                default:
                    LOG_VMM_ERR("Reading unknown I/O APIC register 0x%x\n", ioapic_regs.selected_reg);
                    return false;
            }
        } else {
            LOG_VMM_ERR("Reading unknown I/O APIC register offset 0x%x\n", offset);
            return false;
        }
    } else {
        if (offset == REG_IOAPIC_IOREGSEL_OFF) {
            uint64_t select = vctx_raw[decoded_mem_ins.target_reg];
            LOG_APIC("selecting I/O APIC register 0x%x\n", select);
            ioapic_regs.selected_reg = select;
        } else if (offset == REG_IOAPIC_IOWIN_OFF) {
            // implement me
            assert(false);
        } else {
            LOG_VMM_ERR("Writing unknown I/O APIC register offset 0x%x\n", offset);
            return false;
        }
    }

    return true;
}
