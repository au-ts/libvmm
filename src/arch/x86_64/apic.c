#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/fault.h>

/* Uncomment this to enable debug logging */
#define DEBUG_APIC

#if defined(DEBUG_APIC)
#define LOG_APIC(...) do{ printf("%s|APIC: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_APIC(...) do{}while(0)
#endif

#define REG_LAPIC_ID 0x20
#define REG_LAPIC_REV 0x30

struct lapic_regs {
    uint32_t id;
};

struct lapic_regs lapic_regs;

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
                vctx_raw[decoded_mem_ins.target_reg] = 0x10;
                break;
            default:
                LOG_VMM_ERR("Reading unknown LAPIC register offset 0x%x\n", offset);
                return false;
        }
    } else {
        switch (offset) {
            default:
                LOG_VMM_ERR("Writing unknown LAPIC register offset 0x%x\n", offset);
                return false;
        }
    }

    return true;
}
