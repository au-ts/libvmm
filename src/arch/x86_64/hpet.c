#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/hpet.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/guest.h>
#include <sel4/arch/vmenter.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>

// @billn address all manner of sus in this code.
// @billn handle overflows gracefully.

// https://wiki.osdev.org/HPET
// https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf

/* Uncomment this to enable debug logging */
// #define DEBUG_HPET

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
#define MAIN_COUNTER_VALUE_HIGH_MMIO_OFF 0xf4

// General Capability register
#define NS_IN_FS 1000000ul
// Main counter tick period in femtosecond.
// Make the main counter tick in 1ns, same as sDDF timer interface to keep things sane.
#define COUNTER_CLK_PERIOD_VAL NS_IN_FS
#define COUNTER_CLK_PERIOD_SHIFT 32
// Legacy IRQ replacement capable (replace the old PIT)
#define LEG_RT_CAP BIT(15)
// 3 comparators
#define NUM_TIM_CAP_VAL 2ul // last index
#define NUM_TIM_CAP_SHIFT 8
#define REV_ID 1ul

// General Config register
#define LEG_RT_CNF BIT(1) // legacy routing on
#define ENABLE_CNF BIT(0) // counter and irq on

// Comparator config register
#define Tn_PER_INT_CAP BIT(4) // Periodic capable
#define Tn_INT_ENB_CNF BIT(2) // irq on
#define Tn_INT_TYPE_CNF BIT(1) // irq type, 0 = edge, 1 = level
#define Tn_TYPE_CNF BIT(3) // periodic mode on

struct comparator_regs {
    uint64_t config;
    uint32_t current_comparator;
    uint32_t comparator_increment;
};

struct hpet_regs {
    uint64_t general_capabilities; // RO
    uint64_t general_config; // RW
    uint64_t isr; // RW
    struct comparator_regs comparators[NUM_TIM_CAP_VAL + 1];
};

static uint32_t hpet_counter_offset = 0;

static struct hpet_regs hpet_regs = {
    // 32-bit main counter, 3 comparators (only 1 periodic capable), legacy IRQ routing capable, and
    // tick period = 1ns, same as sDDF timer interface.
    .general_capabilities = (REV_ID | (NUM_TIM_CAP_VAL << NUM_TIM_CAP_SHIFT) | LEG_RT_CAP
                             | (COUNTER_CLK_PERIOD_VAL << COUNTER_CLK_PERIOD_SHIFT)),
    .comparators[0] = { .config = Tn_PER_INT_CAP | BIT(42) }, // ioapic pin 10, if no legacy
    .comparators[1] = { .config = BIT(43) }, // ioapic pin 11, if no legacy
    .comparators[2] = { .config = BIT(44) }, // ioapic pin 12, if no legacy
};

static uint32_t time_now_32(void)
{
    return sddf_timer_time_now(TIMER_DRV_CH_FOR_HPET) & 0xffffffff;
}

static uint32_t main_counter_value(void)
{
    if (hpet_regs.general_config & ENABLE_CNF) {
        return time_now_32() - hpet_counter_offset;
    } else {
        return 0;
    }
}

static void reset_main_counter(void)
{
    hpet_counter_offset = time_now_32();
}

static int timer_n_config_reg_mmio_off(int n)
{
    return 0x100 + (0x20 * n);
}

static int timer_n_comparator_mmio_off(int n)
{
    return 0x108 + (0x20 * n);
}

static uint8_t get_timer_n_ioapic_pin(int n)
{
    assert(n <= NUM_TIM_CAP_VAL);
    if (hpet_regs.general_config & LEG_RT_CNF) {
        if (n == 0) {
            return 2;
        } else if (n == 1) {
            return 8;
        } else {
            return hpet_regs.comparators[n].config >> 32;
        }
    } else {
        return hpet_regs.comparators[n].config >> 32;
    }
}

static bool timer_n_can_interrupt(int n)
{
    return hpet_regs.general_config & ENABLE_CNF && hpet_regs.comparators[n].config & Tn_INT_ENB_CNF;
}

static bool timer_n_in_periodic_mode(int n)
{
    return hpet_regs.comparators[n].config & Tn_TYPE_CNF;
}

static bool timer_n_irq_edge_triggered(int n)
{
    return (hpet_regs.comparators[n].config & Tn_INT_TYPE_CNF) == 0;
}

void hpet_handle_timer_ntfn(void)
{
    // sus, handle timer 1 and 2 as well.
    int ioapic_pin = get_timer_n_ioapic_pin(0);

    // don't check for error, as the pin may be masked in the I/O APIC in early boot.
    inject_ioapic_irq(GUEST_BOOT_VCPU_ID, ioapic_pin);

    if (timer_n_in_periodic_mode(0) && timer_n_can_interrupt(0)) {
        uint32_t main_counter_val = main_counter_value();
        hpet_regs.comparators[0].current_comparator = main_counter_val + hpet_regs.comparators[0].comparator_increment;
        if (main_counter_val < hpet_regs.comparators[0].current_comparator) {
            uint64_t delay_ns = hpet_regs.comparators[0].current_comparator - main_counter_val;
            sddf_timer_set_timeout(TIMER_DRV_CH_FOR_HPET, delay_ns);
        }
    }
}

bool hpet_maintenance(void)
{
    // @billn sus
    assert(timer_n_irq_edge_triggered(0));
    assert(!timer_n_can_interrupt(1));
    assert(!timer_n_can_interrupt(2));
    
    // LOG_VMM("hpet maintenance, main_counter_val = %lu, comp0 = %lu\n", main_counter_val, hpet_regs.comparators[0].comparator);
    if (timer_n_can_interrupt(0)) {
        uint32_t main_counter_val = main_counter_value();
        hpet_regs.comparators[0].current_comparator = main_counter_val + hpet_regs.comparators[0].comparator_increment;
        if (main_counter_val < hpet_regs.comparators[0].current_comparator) {
            uint64_t delay_ns = hpet_regs.comparators[0].current_comparator - main_counter_val;
            LOG_VMM("HPET timeout requested, delay ns = %u, is periodic %d\n", delay_ns, timer_n_in_periodic_mode(0));
            LOG_VMM("... is edge triggered %u, ioapic pin %d\n", timer_n_irq_edge_triggered(0), get_timer_n_ioapic_pin(0));
            sddf_timer_set_timeout(TIMER_DRV_CH_FOR_HPET, delay_ns);
        }
    }

    return true;
}

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
        if (offset == GENERAL_CAP_REG_MMIO_OFF) {
            if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.general_capabilities & 0xffffffff;
            } else if (decoded_mem_ins.access_width == QWORD_ACCESS_WIDTH) {
                vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.general_capabilities;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == GENERAL_CAP_REG_HIGH_MMIO_OFF) {
            if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.general_capabilities >> 32;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == GENERAL_CONFIG_REG_MMIO_OFF) {
            vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.general_config;

        } else if (offset == MAIN_COUNTER_VALUE_MMIO_OFF) {
            vctx_raw[decoded_mem_ins.target_reg] = main_counter_value();

        } else if (offset == MAIN_COUNTER_VALUE_HIGH_MMIO_OFF) {
            vctx_raw[decoded_mem_ins.target_reg] = 0;

        } else if (offset == timer_n_config_reg_mmio_off(0)) {
            if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.comparators[0].config & 0xffffffff;
            } else if (decoded_mem_ins.access_width == QWORD_ACCESS_WIDTH) {
                vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.comparators[0].config;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_config_reg_mmio_off(1)) {
            if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.comparators[1].config & 0xffffffff;
            } else if (decoded_mem_ins.access_width == QWORD_ACCESS_WIDTH) {
                vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.comparators[1].config;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_config_reg_mmio_off(2)) {
            if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.comparators[2].config & 0xffffffff;
            } else if (decoded_mem_ins.access_width == QWORD_ACCESS_WIDTH) {
                vctx_raw[decoded_mem_ins.target_reg] = hpet_regs.comparators[2].config;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else {
            LOG_VMM_ERR("Reading unknown HPET register offset 0x%x\n", offset);
            return false;
        }

    } else {
        if (offset == GENERAL_CONFIG_REG_MMIO_OFF) {
            uint64_t old_config = hpet_regs.general_config;

            if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                uint64_t curr_hi = (hpet_regs.general_config >> 32) << 32;
                uint64_t new_low = vctx_raw[decoded_mem_ins.target_reg] & 0xffffffff;
                hpet_regs.general_config = curr_hi | new_low;
            } else if (decoded_mem_ins.access_width == QWORD_ACCESS_WIDTH) {
                hpet_regs.general_config = vctx_raw[decoded_mem_ins.target_reg];
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

            if (!(old_config & ENABLE_CNF )&& hpet_regs.general_config & ENABLE_CNF) {
                reset_main_counter();
            }

        } else if (offset == MAIN_COUNTER_VALUE_MMIO_OFF || offset == MAIN_COUNTER_VALUE_HIGH_MMIO_OFF) {
            assert(vctx_raw[decoded_mem_ins.target_reg] == 0);
            hpet_counter_offset = time_now_32();

        } else if (offset == timer_n_config_reg_mmio_off(0)) {
            if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                uint64_t curr_hi = (hpet_regs.comparators[0].config >> 32) << 32;
                uint64_t new_low = vctx_raw[decoded_mem_ins.target_reg] & 0xffffffff;
                hpet_regs.comparators[0].config = curr_hi | new_low;
            } else if (decoded_mem_ins.access_width == QWORD_ACCESS_WIDTH) {
                hpet_regs.comparators[0].config = vctx_raw[decoded_mem_ins.target_reg];
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_config_reg_mmio_off(1)) {
            if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                uint64_t curr_hi = (hpet_regs.comparators[1].config >> 32) << 32;
                uint64_t new_low = vctx_raw[decoded_mem_ins.target_reg] & 0xffffffff;
                hpet_regs.comparators[1].config = curr_hi | new_low;
            } else if (decoded_mem_ins.access_width == QWORD_ACCESS_WIDTH) {
                hpet_regs.comparators[1].config = vctx_raw[decoded_mem_ins.target_reg];
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_config_reg_mmio_off(2)) {
            if (decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH) {
                uint64_t curr_hi = (hpet_regs.comparators[2].config >> 32) << 32;
                uint64_t new_low = vctx_raw[decoded_mem_ins.target_reg] & 0xffffffff;
                hpet_regs.comparators[2].config = curr_hi | new_low;
            } else if (decoded_mem_ins.access_width == QWORD_ACCESS_WIDTH) {
                hpet_regs.comparators[2].config = vctx_raw[decoded_mem_ins.target_reg];
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_comparator_mmio_off(0)) {
            assert(decoded_mem_ins.access_width == DWORD_ACCESS_WIDTH);

            // @billn todo, the sematic is different for one shot
            assert(timer_n_in_periodic_mode(0));

            hpet_regs.comparators[0].comparator_increment = vctx_raw[decoded_mem_ins.target_reg];

        } else {
            LOG_VMM_ERR("Writing unknown HPET register offset 0x%x\n", offset);
            return false;
        }

        hpet_maintenance();
    }

    return true;
}