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

struct lapic_regs {
    uint32_t id;
};

struct lapic_regs lapic_regs;

bool apic_handle_read(uint64_t offset) {
    switch (offset) {
    case REG_LAPIC_ID:
        return lapic_regs.id;
    default:
        LOG_VMM_ERR("unknown LAPIC offset: 0x%lx\n", offset);
        return false;
    }
}

bool apic_fault_handle(uint64_t offset, seL4_Word qualification) {
    // TODO: support other alignments?
    assert(offset % 4 == 0);
    if (fault_is_read(qualification)) {
        LOG_APIC("handling read at offset 0x%lx\n", offset);
    } else if (fault_is_write(qualification)) {
        LOG_APIC("handling write at offset 0x%lx\n", offset);
    }

    return false;
}
