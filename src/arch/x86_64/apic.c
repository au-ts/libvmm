#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>

/* Uncomment this to enable debug logging */
#define DEBUG_APIC

#if defined(DEBUG_APIC)
#define LOG_APIC(...) do{ printf("%s|APIC: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_APIC(...) do{}while(0)
#endif

bool apic_fault_handle(uint64_t offset) {
    LOG_APIC("handling fault at offset 0x%lx\n", offset);
    return false;
}
