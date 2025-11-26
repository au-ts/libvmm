#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/fault.h>

// https://wiki.osdev.org/APIC

/* Uncomment this to enable debug logging */
// #define DEBUG_APIC

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
// @billn can make this less hardcodey
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
#define REG_LAPIC_ESR 0x280
#define REG_LAPIC_ICR 0x300
#define REG_LAPIC_LVT_LINT0 0x350
#define REG_LAPIC_LVT_LINT1 0x360
#define REG_LAPIC_LVT_ERR 0x370

#define REG_IOAPIC_IOREGSEL_MMIO_OFF 0x0
#define REG_IOAPIC_IOWIN_MMIO_OFF 0x10
#define REG_IOAPIC_IOAPICID_REG_OFF 0x0
#define REG_IOAPIC_IOAPICVER_REG_OFF 0x1
#define REG_IOAPIC_IOAPICARB_REG_OFF 0x2
#define REG_IOAPIC_IOREDTBL_FIRST_OFF 0x10
#define REG_IOAPIC_IOREDTBL_LAST_OFF 0x40

struct lapic_regs {
    uint32_t id;
    uint32_t revision;
    uint32_t svr;
    uint32_t tpr;
    uint32_t isr[8];
    uint32_t irr[8];
    uint32_t icr;
    uint32_t lint0;
    uint32_t lint1;
    // uint32_t esr;
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
    .isr = { 0 },
    // Interrupt Request Register
    .irr = { 0 },
    // Interrupt Command Register
    .icr = 0,
    // "Specifies interrupt delivery when an interrupt is signaled at the LINT0 pin"
    // Figure 11-8. Local Vector Table (LVT)
    .lint0 = 0x10000, // reset value
    .lint1 = 0x10000, // reset value
};

#define IOAPIC_LAST_INDIRECT_INDEX 0x17

struct ioapic_regs {
    uint32_t selected_reg;

    uint32_t ioapicid;
    uint32_t ioapicver;
    uint32_t ioapicarb;
    uint64_t ioredtbl[IOAPIC_LAST_INDIRECT_INDEX + 1]
};

// https://pdos.csail.mit.edu/6.828/2016/readings/ia32/ioapic.pdf
struct ioapic_regs ioapic_regs = {
    .selected_reg = 0,

    .ioapicid = 0,
    // default value for the Intel 82093AA IOAPIC.
    // supports 0x17 indirection entries.
    .ioapicver = 0x11 | (IOAPIC_LAST_INDIRECT_INDEX << 16),
    .ioapicarb = 0,
};

bool lapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                        memory_instruction_data_t decoded_mem_ins)
{
    uint64_t *vctx_raw = (uint64_t *)vctx;

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
        case REG_LAPIC_LVT_ERR:
            break;
        case REG_LAPIC_ESR:
            vctx_raw[decoded_mem_ins.target_reg] = 0;
            break;
        case REG_LAPIC_ICR:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.icr;
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
        case REG_LAPIC_ICR:
            lapic_regs.icr = vctx_raw[decoded_mem_ins.target_reg];
            break;
        case REG_LAPIC_ESR:
        case REG_LAPIC_LVT_ERR:
            // lapic_regs.esr = 0;
            break;
        default:
            LOG_VMM_ERR("Writing unknown LAPIC register offset 0x%x, value 0x%lx\n", offset,
                        vctx_raw[decoded_mem_ins.target_reg]);
            return false;
        }
    }

    return true;
}

bool ioapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                         memory_instruction_data_t decoded_mem_ins)
{
    if (ept_fault_is_read(qualification)) {
        LOG_APIC("handling I/O APIC read at MMIO offset 0x%lx\n", offset);
    } else if (ept_fault_is_write(qualification)) {
        LOG_APIC("handling I/O APIC write at MMIO offset 0x%lx\n", offset);
    }

    uint64_t *vctx_raw = (uint64_t *)vctx;

    if (ept_fault_is_read(qualification)) {
        if (offset == REG_IOAPIC_IOWIN_MMIO_OFF) {
            if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICID_REG_OFF) {
                vctx_raw[decoded_mem_ins.target_reg] = ioapic_regs.ioapicid;
            } else if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICVER_REG_OFF) {
                vctx_raw[decoded_mem_ins.target_reg] = ioapic_regs.ioapicver;
            } else if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICARB_REG_OFF) {
                vctx_raw[decoded_mem_ins.target_reg] = ioapic_regs.ioapicarb;
            } else if (ioapic_regs.selected_reg >= REG_IOAPIC_IOREDTBL_FIRST_OFF
                       && ioapic_regs.selected_reg < REG_IOAPIC_IOREDTBL_LAST_OFF) {
                int redirection_reg_idx = ((offset - REG_IOAPIC_IOREDTBL_FIRST_OFF) & ~((uint64_t)1)) / 2;
                int is_high = offset & 0x1;

                if (is_high) {
                    vctx_raw[decoded_mem_ins.target_reg] = ioapic_regs.ioredtbl[redirection_reg_idx] >> 32;
                } else {
                    vctx_raw[decoded_mem_ins.target_reg] = ioapic_regs.ioredtbl[redirection_reg_idx] & 0xffffffff;
                }

            } else {
                LOG_VMM_ERR("Reading unknown I/O APIC register offset 0x%x\n", ioapic_regs.selected_reg);
                return false;
            }
        } else {
            LOG_VMM_ERR("Reading unknown I/O APIC MMIO register 0x%x\n", offset);
            return false;
        }
    } else {
        if (offset == REG_IOAPIC_IOREGSEL_MMIO_OFF) {
            ioapic_regs.selected_reg = vctx_raw[decoded_mem_ins.target_reg] & 0xff;
            LOG_APIC("selecting I/O APIC register 0x%x\n", ioapic_regs.selected_reg);
        } else if (offset == REG_IOAPIC_IOWIN_MMIO_OFF) {
            if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICID_REG_OFF) {
                LOG_APIC("Written to I/O APIC ID register: 0x%lx\n", vctx_raw[decoded_mem_ins.target_reg]);
                ioapic_regs.ioapicid = vctx_raw[decoded_mem_ins.target_reg];
            } else if (ioapic_regs.selected_reg >= REG_IOAPIC_IOREDTBL_FIRST_OFF
                       && ioapic_regs.selected_reg < REG_IOAPIC_IOREDTBL_LAST_OFF) {
                int redirection_reg_idx = ((offset - REG_IOAPIC_IOREDTBL_FIRST_OFF) & ~((uint64_t)1)) / 2;
                int is_high = offset & 0x1;

                if (is_high) {
                    uint64_t new_high = vctx_raw[decoded_mem_ins.target_reg] << 32;
                    uint64_t low = ioapic_regs.ioredtbl[redirection_reg_idx] & 0xffffffff;
                    ioapic_regs.ioredtbl[redirection_reg_idx] = low | new_high;
                } else {
                    uint64_t high = (ioapic_regs.ioredtbl[redirection_reg_idx] >> 32) << 32;
                    uint64_t new_low = vctx_raw[decoded_mem_ins.target_reg] & 0xffffffff;
                    ioapic_regs.ioredtbl[redirection_reg_idx] = new_low | high;
                }
            } else {
                LOG_VMM_ERR("Writing unknown I/O APIC register offset 0x%x\n", ioapic_regs.selected_reg);
                return false;
            }
        } else {
            LOG_VMM_ERR("Writing unknown I/O APIC MMIO register 0x%x\n", offset);
            return false;
        }
    }

    return true;
}
