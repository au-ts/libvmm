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

// Table 11-1. Local APIC Register Address Map
#define REG_LAPIC_ID 0x20
#define REG_LAPIC_REV 0x30
#define REG_LAPIC_TPR 0x80
#define REG_LAPIC_SVR 0xf0
#define REG_LAPIC_ISR_0 0x100
#define REG_LAPIC_ISR_1 0x110
#define REG_LAPIC_ISR_2 0x120
#define REG_LAPIC_ISR_3 0x130
#define REG_LAPIC_ISR_4 0x140
#define REG_LAPIC_ISR_5 0x150
#define REG_LAPIC_ISR_6 0x160
#define REG_LAPIC_ISR_7 0x170
#define REG_LAPIC_IRR_0 0x200
#define REG_LAPIC_IRR_1 0x210
#define REG_LAPIC_IRR_2 0x220
#define REG_LAPIC_IRR_3 0x230
#define REG_LAPIC_IRR_4 0x240
#define REG_LAPIC_IRR_5 0x250
#define REG_LAPIC_IRR_6 0x260
#define REG_LAPIC_IRR_7 0x270
#define REG_LAPIC_LVT_LINT0 0x350
#define REG_LAPIC_LVT_LINT1 0x360

#define REG_IOAPIC_IOREGSEL_OFF 0x0
#define REG_IOAPIC_IOWIN_OFF 0x10

struct lapic_regs {
    uint32_t id;
    uint32_t revision;
    uint32_t svr;
    uint32_t tpr;
    uint32_t isr[8];
    uint32_t irr[8];
    uint32_t lint0;
    uint32_t lint1;
};

struct lapic_regs lapic_regs = {
    .id = 0,
    // Figure 11-7. Local APIC Version Register
    // 32 local vector table entries. @billn is enough??
    .revision = 0x10 | 32 << 16,
    // Figure 11-18. Task-Priority Register (TPR)
    .tpr = 0, // reset value
    // Figure 11-23. Spurious-Interrupt Vector Register (SVR)
    .svr = 0xff, // reset value
    // In-Service Register
    .isr = {0},
    // Interrupt Request Register
    .irr = {0},
    // "Specifies interrupt delivery when an interrupt is signaled at the LINT0 pin"
    // Figure 11-8. Local Vector Table (LVT)
    .lint0 = 0x10000, // reset value
    .lint1 = 0x10000, // reset value
};

struct ioapic_regs {
    uint32_t selected_reg;
};

struct ioapic_regs ioapic_regs;

bool lapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification, memory_instruction_data_t decoded_mem_ins) {
    uint64_t *vctx_raw = (uint64_t *) vctx;

    // TODO: support other alignments?
    assert(offset % 4 == 0);
    if (ept_fault_is_read(qualification)) {
        LOG_APIC("handling LAPIC read at offset 0x%lx\n", offset);
    } else if (ept_fault_is_write(qualification)) {
        LOG_APIC("handling LAPIC write at offset 0x%lx, value 0x%x\n", offset, vctx_raw[decoded_mem_ins.target_reg]);
    }

    if (ept_fault_is_read(qualification)) {
        switch (offset) {
            case REG_LAPIC_ID:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.id;
                break;
            case REG_LAPIC_REV:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.revision;
                break;
            case REG_LAPIC_TPR:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.tpr;
                break;
            case REG_LAPIC_SVR:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.svr;
                break;
            case REG_LAPIC_IRR_0:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[0];
                break;
            case REG_LAPIC_IRR_1:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[1];
                break;
            case REG_LAPIC_IRR_2:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[2];
                break;
            case REG_LAPIC_IRR_3:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[3];
                break;
            case REG_LAPIC_IRR_4:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[4];
                break;
            case REG_LAPIC_IRR_5:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[5];
                break;
            case REG_LAPIC_IRR_6:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[6];
                break;
            case REG_LAPIC_IRR_7:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[7];
                break;
            case REG_LAPIC_ISR_0:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[0];
                break;
            case REG_LAPIC_ISR_1:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[1];
                break;
            case REG_LAPIC_ISR_2:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[2];
                break;
            case REG_LAPIC_ISR_3:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[3];
                break;
            case REG_LAPIC_ISR_4:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[4];
                break;
            case REG_LAPIC_ISR_5:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[5];
                break;
            case REG_LAPIC_ISR_6:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[6];
                break;
            case REG_LAPIC_ISR_7:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[7];
                break;
            case REG_LAPIC_LVT_LINT0:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.lint0;
                break;
            case REG_LAPIC_LVT_LINT1:
                vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.lint1;
                break;
            default:
                LOG_VMM_ERR("Reading unknown LAPIC register offset 0x%x\n", offset);
                return false;
        }
    } else {
        switch (offset) {
            case REG_LAPIC_TPR:
                lapic_regs.tpr = vctx_raw[decoded_mem_ins.target_reg];
                break;
            case REG_LAPIC_SVR:
                lapic_regs.svr = vctx_raw[decoded_mem_ins.target_reg];
                break;
            case REG_LAPIC_LVT_LINT0:
                lapic_regs.lint0 = vctx_raw[decoded_mem_ins.target_reg];
                break;
            case REG_LAPIC_LVT_LINT1:
                lapic_regs.lint1 = vctx_raw[decoded_mem_ins.target_reg];
                break;
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
                    // supports 64 irqs, should be enough idk
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
