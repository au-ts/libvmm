#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/hpet.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/guest.h>
#include <sel4/arch/vmenter.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>

/* Uncomment this to enable debug logging */
#define DEBUG_HPET

#if defined(DEBUG_HPET)
#define LOG_HPET(...) do{ printf("%s|HPET: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_HPET(...) do{}while(0)
#endif

#define GENERAL_CAP_REG_MMIO_OFF 0x0
#define GENERAL_CAP_REG_HIGH_MMIO_OFF 0x4

#define GENERAL_CONFIG_REG_MMIO_OFF 0x10
#define GENERAL_CONFIG_REG_HIGH_MMIO_OFF 0x14

#define GENERAL_ISR_MMIO_OFF 0x20
#define MAIN_COUNTER_VALUE_MMIO_OFF 0xf0

struct hpet_regs {
    uint64_t general_capabilities; // RO
    uint64_t general_config; // RW
    uint64_t isr; // RW

};

// Capability register
#define NS_IN_FS 1000000ul
// Main counter tick period in femtosecond.
// Make the main counter tick in 1ns, same as sDDF timer interface to keep things sane.
#define COUNTER_CLK_PERIOD_VAL NS_IN_FS
#define COUNTER_CLK_PERIOD_SHIFT 32
// Legacy IRQ replacement capable (replace the old PIT)
#define LEG_RT_CAP BIT(15)
// 64-bit main counter
#define COUNT_SIZE_CAP BIT(13)
// 3 comparators
#define NUM_TIM_CAP_VAL 3ul
#define NUM_TIM_CAP_SHIFT 8
#define REV_ID 1ul

// Config register
#define LEG_RT_CNF BIT(1)
#define ENABLE_CNF BIT(0)

static struct hpet_regs hpet_regs = {
    .general_capabilities = REV_ID | (NUM_TIM_CAP_VAL << NUM_TIM_CAP_SHIFT) | COUNT_SIZE_CAP | LEG_RT_CAP | (COUNTER_CLK_PERIOD_VAL << COUNTER_CLK_PERIOD_SHIFT),
    .general_config = LEG_RT_CNF | ENABLE_CNF,
};

bool hpet_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                        memory_instruction_data_t decoded_mem_ins)
{
    uint64_t *vctx_raw = (uint64_t *)vctx;

    if (ept_fault_is_read(qualification)) {
        LOG_HPET("handling HPET read at offset 0x%lx\n", offset);
    } else if (ept_fault_is_write(qualification)) {
        LOG_HPET("handling HPET write at offset 0x%lx, value 0x%x\n", offset, vctx_raw[decoded_mem_ins.target_reg]);
    }

    if (ept_fault_is_read(qualification)) {
        switch (offset) {
            case GENERAL_CAP_REG_MMIO_OFF:
                if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                    vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.general_capabilities & 0xffffffff;
                } else if (decoded_mem_ins.access_width == QWORD_ACCESS_WIDTH) {
                    vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.general_capabilities;
                } else {
                    LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                    return false;
                }
                break;
            case GENERAL_CAP_REG_HIGH_MMIO_OFF:
                if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                    vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.general_capabilities >> 32;
                } else {
                    LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                    return false;
                }
            case GENERAL_CONFIG_REG_MMIO_OFF:
                vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.general_config;
                break;
            case MAIN_COUNTER_VALUE_MMIO_OFF:
                vctx_raw[decoded_mem_ins.target_reg] = sddf_timer_time_now(TIMER_DRV_CH_FOR_HPET);
                break;
            default:
                LOG_VMM_ERR("Reading unknown HPET register offset 0x%x\n", offset);
                return false;
        }
    }

    return true;
}