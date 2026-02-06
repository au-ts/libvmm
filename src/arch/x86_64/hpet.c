/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/hpet.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/util.h>
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
// 64-bit main counter
#define COUNT_SIZE_CAP BIT(13)
// 3 comparators
#define NUM_TIM_CAP_VAL 2ul // last index
#define NUM_TIM_CAP_SHIFT 8
#define REV_ID 1ul
#define VENDOR_ID (0x5E14ull << 16)

// General Config register
#define LEG_RT_CNF BIT(1) // legacy routing on
#define ENABLE_CNF BIT(0) // counter and irq on

// Comparator config register
#define Tn_32MODE_CNF BIT(8) // if software set this bit, comparator is in 32 bits mode
#define Tn_SIZE_CAP BIT(5) // 64-bit
#define Tn_PER_INT_CAP BIT(4) // Periodic capable
#define Tn_INT_ENB_CNF BIT(2) // irq on
#define Tn_INT_TYPE_CNF BIT(1) // irq type, 0 = edge, 1 = level
#define Tn_TYPE_CNF BIT(3) // periodic mode on

struct comparator_regs {
    uint64_t config;
    uint64_t current_comparator;
    uint64_t comparator_increment;
};

struct hpet_regs {
    uint64_t general_capabilities; // RO
    uint64_t general_config; // RW
    uint64_t isr; // RW
    struct comparator_regs comparators[NUM_TIM_CAP_VAL + 1];
};

static uint64_t hpet_counter_offset = 0;

static struct hpet_regs hpet_regs = {
    // 64-bit main counter, 3 comparators (only 1 periodic capable), legacy IRQ routing capable, and
    // tick period = 1ns, same as sDDF timer interface.
    .general_capabilities = (REV_ID | (NUM_TIM_CAP_VAL << NUM_TIM_CAP_SHIFT) | LEG_RT_CAP
                             | (COUNTER_CLK_PERIOD_VAL << COUNTER_CLK_PERIOD_SHIFT))
                          | VENDOR_ID | COUNT_SIZE_CAP,
    .comparators[0] = { .config = Tn_SIZE_CAP | Tn_PER_INT_CAP | BIT(42) }, // ioapic pin 10, if no legacy
    .comparators[1] = { .config = Tn_SIZE_CAP | BIT(43) }, // ioapic pin 11, if no legacy
    .comparators[2] = { .config = Tn_SIZE_CAP | BIT(44) }, // ioapic pin 12, if no legacy
};

static uint64_t time_now_64(void)
{
    return sddf_timer_time_now(TIMER_DRV_CH_FOR_HPET_CH0) - hpet_counter_offset;
}

static uint64_t main_counter_value(void)
{
    if (hpet_regs.general_config & ENABLE_CNF) {
        return time_now_64();
    } else {
        return 0;
    }
}

static void reset_main_counter(void)
{
    hpet_counter_offset = time_now_64();
}

static bool timer_n_forced_32(int n)
{
    return !!(hpet_regs.comparators->config & Tn_32MODE_CNF);
}

static uint64_t counter_value_in_terms_of_timer(int n)
{
    uint64_t counter = main_counter_value();
    if (timer_n_forced_32(n)) {
        counter = counter & 0xffffffff;
    }
    return counter;
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
        // legacy routing
        if (n == 0) {
            return 2;
        } else if (n == 1) {
            return 8;
        }
    }
    return (hpet_regs.comparators[n].config >> 9) & 0x1f; // Tn_INT_ROUTE_CNF
}

static bool timer_n_can_interrupt(int n)
{
    return (hpet_regs.general_config & ENABLE_CNF) && (hpet_regs.comparators[n].config & Tn_INT_ENB_CNF);
}

static bool timer_n_in_periodic_mode(int n)
{
    return hpet_regs.comparators[n].config & Tn_TYPE_CNF;
}

static bool timer_n_irq_edge_triggered(int n)
{
    return (hpet_regs.comparators[n].config & Tn_INT_TYPE_CNF) == 0;
}

void hpet_handle_timer_ntfn(microkit_channel ch)
{
    // bool maintenance = false;

    if (ch == TIMER_DRV_CH_FOR_HPET_CH0) {
        int ioapic_pin = get_timer_n_ioapic_pin(0);
        if (!inject_ioapic_irq(0, ioapic_pin)) {
            LOG_VMM_ERR("IRQ dropped on HPET comp 0\n");
        } else {
            LOG_HPET("IRQ injected on HPET comp #0\n");
        }

        if (timer_n_in_periodic_mode(0) && timer_n_can_interrupt(0)) {
            uint64_t main_counter_val = counter_value_in_terms_of_timer(0);
            hpet_regs.comparators[0].current_comparator = main_counter_val
                                                        + hpet_regs.comparators[0].comparator_increment;
            if (main_counter_val < hpet_regs.comparators[0].current_comparator) {
                uint64_t delay_ns = hpet_regs.comparators[0].current_comparator - main_counter_val;
                sddf_timer_set_timeout(TIMER_DRV_CH_FOR_HPET_CH0, delay_ns);
            }
        }
    } else if (ch == TIMER_DRV_CH_FOR_HPET_CH1) {
        int ioapic_pin = get_timer_n_ioapic_pin(1);
        if (!inject_ioapic_irq(0, ioapic_pin)) {
            LOG_VMM_ERR("IRQ dropped on HPET comp 1\n");
        } else {
            LOG_HPET("IRQ injected on HPET comp #1\n");
        }

        assert(!timer_n_in_periodic_mode(1));
    } else if (ch == TIMER_DRV_CH_FOR_HPET_CH2) {
        int ioapic_pin = get_timer_n_ioapic_pin(2);
        if (!inject_ioapic_irq(0, ioapic_pin)) {
            LOG_VMM_ERR("IRQ dropped on HPET comp 2\n");
        } else {
            LOG_HPET("IRQ injected on HPET comp #2\n");
        }

        assert(!timer_n_in_periodic_mode(2));
    } else {
        assert(false);
    }
}

uint64_t timer_n_compute_timeout_ns(int n)
{
    uint64_t main_counter_val = counter_value_in_terms_of_timer(n);
    uint64_t delay_ns = 0;

    if (main_counter_val < hpet_regs.comparators[n].current_comparator) {
        delay_ns = hpet_regs.comparators[n].current_comparator - main_counter_val;
    }

    // detect counter overflow in 32-bit mode, which can happen frequently as our HPET is 1GHz lol
    if (!delay_ns && timer_n_forced_32(n)) {
        // if (main_counter_val + hpet_regs.comparators[n].current_comparator >= (1 << 32)) {
        delay_ns += (1ull << 32) - main_counter_val;
        delay_ns += hpet_regs.comparators[n].current_comparator;
        LOG_HPET("handling overflow, %ld + %ld = %ld\n", (1 << 32) - main_counter_val, hpet_regs.comparators[n].current_comparator, delay_ns);
        // }
        if (delay_ns > (1ull << 32)) {
            LOG_HPET("absurd!!\n");
            delay_ns = 0;
        }
    }

    return delay_ns;
}

bool hpet_maintenance(void)
{
    // @billn sus
    // assert(timer_n_irq_edge_triggered(0));
    // assert(!timer_n_can_interrupt(1));
    // assert(!timer_n_can_interrupt(2));

    // LOG_VMM("hpet maintenance, main_counter_val = %lu, comp0 = %lu\n", main_counter_val, hpet_regs.comparators[0].comparator);
    LOG_HPET("timer 0 can irq %d, is periodic %d, is 32b %d\n", timer_n_can_interrupt(0), timer_n_in_periodic_mode(0), timer_n_forced_32(0));
    if (timer_n_can_interrupt(0)) {
        uint64_t main_counter_val = counter_value_in_terms_of_timer(0);

        if (timer_n_in_periodic_mode(0)) {
            hpet_regs.comparators[0].current_comparator = main_counter_val
                                                        + hpet_regs.comparators[0].comparator_increment;
        }
        if (timer_n_forced_32(0)) {
            hpet_regs.comparators[0].current_comparator &= 0xffffffff;
        }

        LOG_HPET("timer 0: counter < cur comp | 0x%lx < 0x%lx\n", main_counter_val,
                 hpet_regs.comparators[0].current_comparator);

        uint64_t delay_ns = timer_n_compute_timeout_ns(0);

        if (delay_ns) {
            LOG_HPET("HPET timeout requested, delay ns = %u, is periodic %d\n", delay_ns, timer_n_in_periodic_mode(0));
            LOG_HPET("... is edge triggered %u, ioapic pin %d\n", timer_n_irq_edge_triggered(0),
                     get_timer_n_ioapic_pin(0));
            sddf_timer_set_timeout(TIMER_DRV_CH_FOR_HPET_CH0, delay_ns);
        }
    }

    if (timer_n_can_interrupt(1)) {
        uint64_t main_counter_val = counter_value_in_terms_of_timer(1);
        if (main_counter_val < hpet_regs.comparators[1].current_comparator) {
            uint64_t delay_ns = hpet_regs.comparators[1].current_comparator - main_counter_val;
            LOG_HPET("HPET timeout requested, delay ns = %u, is periodic %d\n", delay_ns, timer_n_in_periodic_mode(1));
            LOG_HPET("... is edge triggered %u, ioapic pin %d\n", timer_n_irq_edge_triggered(1),
                     get_timer_n_ioapic_pin(1));
            sddf_timer_set_timeout(TIMER_DRV_CH_FOR_HPET_CH1, delay_ns);
        }
    }

    if (timer_n_can_interrupt(2)) {
        uint64_t main_counter_val = counter_value_in_terms_of_timer(2);
        if (main_counter_val < hpet_regs.comparators[2].current_comparator) {
            uint64_t delay_ns = hpet_regs.comparators[2].current_comparator - main_counter_val;
            LOG_HPET("HPET timeout requested, delay ns = %u, is periodic %d\n", delay_ns, timer_n_in_periodic_mode(2));
            LOG_HPET("... is edge triggered %u, ioapic pin %d\n", timer_n_irq_edge_triggered(2),
                     get_timer_n_ioapic_pin(2));
            sddf_timer_set_timeout(TIMER_DRV_CH_FOR_HPET_CH2, delay_ns);
        }
    }

    return true;
}

bool hpet_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                       decoded_instruction_ret_t decoded_ins)
{
    if (ept_fault_is_read(qualification)) {
        assert(decoded_ins.type == INSTRUCTION_MEMORY);
        LOG_HPET("handling HPET read at offset 0x%lx, acc width %d bytes, reg idx %d\n", offset,
                 mem_access_width_to_bytes(decoded_ins), decoded_ins.decoded.memory_instruction.target_reg);
    } else if (ept_fault_is_write(qualification)) {
        uint64_t data;
        assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));
        LOG_HPET("handling HPET write at offset 0x%lx, value 0x%x\n", offset, data);
    }

    // vcpu_print_regs(0);

    if (ept_fault_is_read(qualification)) {
        uint64_t data;
        if (offset == GENERAL_CAP_REG_MMIO_OFF) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                data = hpet_regs.general_capabilities & 0xffffffff;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                data = hpet_regs.general_capabilities;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == GENERAL_CAP_REG_HIGH_MMIO_OFF) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                data = hpet_regs.general_capabilities >> 32;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == GENERAL_CONFIG_REG_MMIO_OFF) {
            data = hpet_regs.general_config;

        } else if (offset == MAIN_COUNTER_VALUE_MMIO_OFF) {
            data = main_counter_value() & 0xffffffff;

        } else if (offset == MAIN_COUNTER_VALUE_HIGH_MMIO_OFF) {
            data = main_counter_value() >> 32;

        } else if (offset == timer_n_config_reg_mmio_off(0)) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                data = hpet_regs.comparators[0].config & 0xffffffff;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                data = hpet_regs.comparators[0].config;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_config_reg_mmio_off(1)) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                data = hpet_regs.comparators[1].config & 0xffffffff;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                data = hpet_regs.comparators[1].config;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_config_reg_mmio_off(2)) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                data = hpet_regs.comparators[2].config & 0xffffffff;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                data = hpet_regs.comparators[2].config;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_comparator_mmio_off(0)) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                data = hpet_regs.comparators[0].current_comparator & 0xffffffff;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                data = hpet_regs.comparators[0].current_comparator;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_comparator_mmio_off(2)) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                data = hpet_regs.comparators[2].current_comparator & 0xffffffff;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                data = hpet_regs.comparators[2].current_comparator;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else {
            LOG_VMM_ERR("Reading unknown HPET register offset 0x%x\n", offset);
            return false;
        }
        assert(mem_read_set_data(decoded_ins, qualification, vctx, data));
    } else {
        uint64_t data;
        assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));

        if (offset == GENERAL_CONFIG_REG_MMIO_OFF) {
            uint64_t old_config = hpet_regs.general_config;

            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                uint64_t curr_hi = (hpet_regs.general_config >> 32) << 32;
                uint64_t new_low = data & 0xffffffff;
                hpet_regs.general_config = curr_hi | new_low;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                hpet_regs.general_config = data;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

            if (!(old_config & ENABLE_CNF) && hpet_regs.general_config & ENABLE_CNF) {
                reset_main_counter();
            }

        } else if (offset == MAIN_COUNTER_VALUE_MMIO_OFF || offset == MAIN_COUNTER_VALUE_HIGH_MMIO_OFF) {
            assert(data == 0);
            hpet_counter_offset = time_now_64();

        } else if (offset == timer_n_config_reg_mmio_off(0)) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                uint64_t curr_hi = (hpet_regs.comparators[0].config >> 32) << 32;
                uint64_t new_low = data & 0xffffffff;
                hpet_regs.comparators[0].config = curr_hi | new_low;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                hpet_regs.comparators[0].config = data;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_config_reg_mmio_off(1)) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                uint64_t curr_hi = (hpet_regs.comparators[1].config >> 32) << 32;
                uint64_t new_low = data & 0xffffffff;
                hpet_regs.comparators[1].config = curr_hi | new_low;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                hpet_regs.comparators[1].config = data;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_config_reg_mmio_off(2)) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                uint64_t curr_hi = (hpet_regs.comparators[2].config >> 32) << 32;
                uint64_t new_low = data & 0xffffffff;
                hpet_regs.comparators[2].config = curr_hi | new_low;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                hpet_regs.comparators[2].config = data;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == timer_n_comparator_mmio_off(0)) {
            if (timer_n_in_periodic_mode(0)) {
                hpet_regs.comparators[0].comparator_increment = data;
            } else {
                hpet_regs.comparators[0].comparator_increment = 0;
                hpet_regs.comparators[0].current_comparator = data;
            }
        } else if (offset == timer_n_comparator_mmio_off(1)) {
            // @billn todo, implement periodic for this comparator
            assert(!timer_n_in_periodic_mode(1));

            hpet_regs.comparators[1].current_comparator = data;
            hpet_regs.comparators[1].comparator_increment = 0;
        } else if (offset == timer_n_comparator_mmio_off(2)) {
            // @billn todo, implement periodic for this comparator
            assert(!timer_n_in_periodic_mode(2));

            hpet_regs.comparators[2].current_comparator = data;
            hpet_regs.comparators[2].comparator_increment = 0;
        } else {
            LOG_VMM_ERR("Writing unknown HPET register offset 0x%x\n", offset);
            return false;
        }

        hpet_maintenance();
    }

    return true;
}