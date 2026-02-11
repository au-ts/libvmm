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

// Implements a minimum HPET specification (3 comparators with 1 being periodic capable)

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
#define Tn_VAL_SET_CNF BIT(6) // Software writes 1 to this bit to change the comparator
#define Tn_SIZE_CAP BIT(5) // 64-bit
#define Tn_PER_INT_CAP BIT(4) // Periodic capable
#define Tn_INT_ENB_CNF BIT(2) // irq on
#define Tn_INT_TYPE_CNF BIT(1) // irq type, 0 = edge, 1 = level
#define Tn_TYPE_CNF BIT(3) // periodic mode on
#define Tn_INT_ROUTE_CNF_SHIFT 9
#define Tn_INT_ROUTE_CAP_SHIFT 32

// I/O APIC routing if no legacy
#define TIM0_IOAPIC_PIN 13
#define TIM1_IOAPIC_PIN 14
#define TIM2_IOAPIC_PIN 15

struct comparator_regs {
    uint64_t config;
    uint64_t config_mask;
    uint64_t current_comparator;
    uint64_t comparator_increment;
};

struct hpet_regs {
    uint64_t general_capabilities; // RO
    uint64_t general_config; // RW
    uint64_t isr; // RW
    struct comparator_regs comparators[NUM_TIM_CAP_VAL + 1];
};

static uint64_t hpet_frozen_counter = 0;
static uint64_t hpet_counter_offset = 0;

#define GENERAL_CAP_MASK ((REV_ID | (NUM_TIM_CAP_VAL << NUM_TIM_CAP_SHIFT) | LEG_RT_CAP | (COUNTER_CLK_PERIOD_VAL << COUNTER_CLK_PERIOD_SHIFT)) | VENDOR_ID | COUNT_SIZE_CAP)
#define TIM0_CONF_MASK (Tn_SIZE_CAP | Tn_PER_INT_CAP | (TIM0_IOAPIC_PIN << Tn_INT_ROUTE_CNF_SHIFT) | (BIT(TIM0_IOAPIC_PIN) << Tn_INT_ROUTE_CAP_SHIFT))
#define TIM1_CONF_MASK (Tn_SIZE_CAP | (TIM1_IOAPIC_PIN << Tn_INT_ROUTE_CNF_SHIFT) | (BIT(TIM1_IOAPIC_PIN) << Tn_INT_ROUTE_CAP_SHIFT))
#define TIM2_CONF_MASK (Tn_SIZE_CAP | (TIM2_IOAPIC_PIN << Tn_INT_ROUTE_CNF_SHIFT) | (BIT(TIM2_IOAPIC_PIN) << Tn_INT_ROUTE_CAP_SHIFT))

static struct hpet_regs hpet_regs = {
    // 64-bit main counter, 3 comparators (only 1 periodic capable), legacy IRQ routing capable, and
    // tick period = 1ns, same as sDDF timer interface.
    .general_capabilities = GENERAL_CAP_MASK,
    .comparators[0] = { .config = TIM0_CONF_MASK, .config_mask = TIM0_CONF_MASK },
    .comparators[1] = { .config = TIM1_CONF_MASK, .config_mask = TIM1_CONF_MASK },
    .comparators[2] = { .config = TIM2_CONF_MASK, .config_mask = TIM2_CONF_MASK },
};

static uint64_t time_now_64(void)
{
    return sddf_timer_time_now(TIMER_DRV_CH_FOR_HPET_CH0);
}

static bool counter_on(void)
{
    return (hpet_regs.general_config & ENABLE_CNF);
}

static uint64_t main_counter_value(void)
{
    if (counter_on()) {
        return time_now_64()  - hpet_counter_offset;
    } else {
        return hpet_frozen_counter;
    }
}

static void reset_main_counter(void)
{
    hpet_counter_offset = time_now_64();
}

static bool timer_n_forced_32(int n)
{
    return !!(hpet_regs.comparators->config[n] & Tn_32MODE_CNF);
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
    return counter_on() && (hpet_regs.comparators[n].config & Tn_INT_ENB_CNF);
}

static bool timer_n_in_periodic_mode(int n)
{
    return hpet_regs.comparators[n].config & Tn_TYPE_CNF;
}

static bool timer_n_irq_edge_triggered(int n)
{
    return (hpet_regs.comparators[n].config & Tn_INT_TYPE_CNF) == 0;
}

uint64_t timer_n_compute_timeout_ns(int n)
{
    uint64_t main_counter_val = counter_value_in_terms_of_timer(n);
    uint64_t delay_ns = 0;

    if (main_counter_val < hpet_regs.comparators[n].current_comparator) {
        delay_ns = hpet_regs.comparators[n].current_comparator - main_counter_val;
    }

    // detect counter overflow in 32-bit mode, which can happen frequently as our HPET is 1GHz lol

    if (!delay_ns) {
        LOG_HPET("comparator %d may have overflown. counter = %ld, comparator = %ld, is 32b %d\n", n, main_counter_val,
                 hpet_regs.comparators[n].current_comparator, timer_n_forced_32(n));
    }

    if (!delay_ns && timer_n_forced_32(n)) {
        // if (main_counter_val + hpet_regs.comparators[n].current_comparator >= (1 << 32)) {
        delay_ns += (1ull << 32) - main_counter_val;
        delay_ns += hpet_regs.comparators[n].current_comparator;
        LOG_HPET("handling overflow, %ld + %ld = %ld\n", (1ULL << 32) - main_counter_val,
                 hpet_regs.comparators[n].current_comparator, delay_ns);
        // }
        if (delay_ns > (1ull << 32)) {
            LOG_VMM_ERR("absurd!!\n");
            delay_ns = 0;
        }
    }

    return delay_ns;
}

void hpet_handle_timer_ntfn(microkit_channel ch)
{
    // bool maintenance = false;

    if (!counter_on()) {
        return;
    }

    if (ch == TIMER_DRV_CH_FOR_HPET_CH0) {
        if (timer_n_can_interrupt(0)) {
            int ioapic_pin = get_timer_n_ioapic_pin(0);
            if (!inject_ioapic_irq(0, ioapic_pin)) {
                LOG_VMM_ERR("IRQ dropped on HPET comp 0, pin %d\n", ioapic_pin);
            }
        }
        if (timer_n_in_periodic_mode(0) && hpet_regs.comparators[0].comparator_increment) {
            uint64_t main_counter_val = counter_value_in_terms_of_timer(0);
            hpet_regs.comparators[0].current_comparator = main_counter_val
                                                        + hpet_regs.comparators[0].comparator_increment;
            if (timer_n_forced_32(0)) {
                hpet_regs.comparators[0].current_comparator &= 0xffffffff;
            }

            uint64_t delay_ns = timer_n_compute_timeout_ns(0);
            if (delay_ns) {
                LOG_HPET("t0 re-arm for %ld ns\n", delay_ns);
                sddf_timer_set_timeout(TIMER_DRV_CH_FOR_HPET_CH0, delay_ns);
            }
        }
    } else if (ch == TIMER_DRV_CH_FOR_HPET_CH1) {
        if (timer_n_can_interrupt(1)) {
            int ioapic_pin = get_timer_n_ioapic_pin(1);
            if (!inject_ioapic_irq(0, ioapic_pin)) {
                LOG_VMM_ERR("IRQ dropped on HPET comp 1, pin %d\n", ioapic_pin);
            }
        }

        assert(!timer_n_in_periodic_mode(1));
    } else if (ch == TIMER_DRV_CH_FOR_HPET_CH2) {
        if (timer_n_can_interrupt(2)) {
            int ioapic_pin = get_timer_n_ioapic_pin(2);
            if (!inject_ioapic_irq(0, ioapic_pin)) {
                LOG_VMM_ERR("IRQ dropped on HPET comp 2, pin %d\n", ioapic_pin);
            }
        }

        assert(!timer_n_in_periodic_mode(2));
    }
}

bool hpet_maintenance(uint8_t comparator)
{
    if (!counter_on()) {
        return true;
    }

    // @billn sus
    assert(timer_n_irq_edge_triggered(0));
    assert(timer_n_irq_edge_triggered(1));
    assert(timer_n_irq_edge_triggered(2));

    if (comparator == 0) {
        // uint64_t main_counter_val = counter_value_in_terms_of_timer(0);

        if (timer_n_in_periodic_mode(0) && hpet_regs.comparators[0].comparator_increment == 0) {
            // Halted
            return true;
        }

        // No need to update the comparator, as the guest's first write to comparator is already
        // timestamp + time out.

        if (timer_n_forced_32(0)) {
            hpet_regs.comparators[0].current_comparator &= 0xffffffff;
        }

        LOG_HPET("hpet_maintenance(): timer 0 can irq %d, is periodic %d, is 32b %d\n", timer_n_can_interrupt(0),
                 timer_n_in_periodic_mode(0), timer_n_forced_32(0));
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

    if (comparator == 1) {
        LOG_HPET("hpet_maintenance(): timer 1 can irq %d, is periodic %d, is 32b %d\n", timer_n_can_interrupt(1),
                 timer_n_in_periodic_mode(1), timer_n_forced_32(1));
        uint64_t delay_ns = timer_n_compute_timeout_ns(1);
        if (delay_ns) {
            LOG_HPET("HPET timeout requested, delay ns = %u, is periodic %d\n", delay_ns, timer_n_in_periodic_mode(1));
            LOG_HPET("... is edge triggered %u, ioapic pin %d\n", timer_n_irq_edge_triggered(1),
                     get_timer_n_ioapic_pin(1));
            sddf_timer_set_timeout(TIMER_DRV_CH_FOR_HPET_CH1, delay_ns);
        }
    }

    if (comparator == 2) {
        LOG_HPET("hpet_maintenance(): timer 2 can irq %d, is periodic %d, is 32b %d\n", timer_n_can_interrupt(2),
                 timer_n_in_periodic_mode(2), timer_n_forced_32(2));
        uint64_t delay_ns = timer_n_compute_timeout_ns(2);
        if (delay_ns) {
            LOG_HPET("HPET timeout requested, delay ns = %u, is periodic %d\n", delay_ns, timer_n_in_periodic_mode(2));
            LOG_HPET("... is edge triggered %u, ioapic pin %d\n", timer_n_irq_edge_triggered(2),
                     get_timer_n_ioapic_pin(2));
            sddf_timer_set_timeout(TIMER_DRV_CH_FOR_HPET_CH2, delay_ns);
        }
    }

    return true;
}

static bool hpet_fault_on_config(uint64_t offset, uint8_t *comparator)
{
    if (offset == timer_n_config_reg_mmio_off(0)) {
        *comparator = 0;
    } else if (offset == timer_n_config_reg_mmio_off(1)) {
        *comparator = 1;
    } else if (offset == timer_n_config_reg_mmio_off(2)) {
        *comparator = 2;
    } else {
        return false;
    }

    return true;
}

static bool hpet_fault_on_comparator(uint64_t offset, uint8_t *comparator)
{
    if (offset == timer_n_comparator_mmio_off(0)) {
        *comparator = 0;
    } else if (offset == timer_n_comparator_mmio_off(1)) {
        *comparator = 1;
    } else if (offset == timer_n_comparator_mmio_off(2)) {
        *comparator = 2;
    } else {
        return false;
    }

    return true;
}

static bool hpet_fault_handle_config_read(uint8_t comparator, uint64_t *data, decoded_instruction_ret_t decoded_ins)
{
    assert(comparator <= 2);
    if (mem_access_width_to_bytes(decoded_ins) == 4) {
        *data = hpet_regs.comparators[comparator].config & 0xffffffff;
    } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
        *data = hpet_regs.comparators[comparator].config;
    } else {
        LOG_VMM_ERR("Unsupported access width on HPET config register, comparator 0x%x\n", comparator);
        return false;
    }

    return true;
}

static bool hpet_fault_handle_comparator_read(uint8_t comparator, uint64_t *data, decoded_instruction_ret_t decoded_ins)
{
    assert(comparator <= 2);
    if (mem_access_width_to_bytes(decoded_ins) == 4) {
        *data = hpet_regs.comparators[comparator].current_comparator & 0xffffffff;
    } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
        *data = hpet_regs.comparators[comparator].current_comparator;
    } else {
        LOG_VMM_ERR("Unsupported access width on HPET comparator register, comparator 0x%x\n", comparator);
        return false;
    }

    return true;
}

static bool hpet_fault_handle_config_write(uint8_t comparator, uint64_t data, decoded_instruction_ret_t decoded_ins)
{
    assert(comparator <= 2);
    bool periodic_old = timer_n_in_periodic_mode(comparator);

    struct comparator_regs *regs = &hpet_regs.comparators[comparator];
    // uint64_t reg_old = regs->config;

    if (mem_access_width_to_bytes(decoded_ins) == 4) {
        uint64_t curr_hi = (regs->config >> 32) << 32;
        uint64_t new_low = data & 0xffffffff;
        regs->config = curr_hi | new_low;
    } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
        regs->config = data;
    } else {
        LOG_VMM_ERR("Unsupported access width on HPET comparator 0x%x\n", comparator);
        return false;
    }

    LOG_HPET("comp %d write %lx, old 0x%lx\n", comparator, data, reg_old);

    regs->config |= regs->config_mask;

    bool periodic_new = timer_n_in_periodic_mode(comparator);

    if (periodic_old && !periodic_new) {
        assert(comparator == 0);
        LOG_HPET("timer 0 periodic off!\n");
    }

    return true;
}

static bool hpet_fault_handle_comparator_write(uint8_t comparator, uint64_t data, decoded_instruction_ret_t decoded_ins)
{
    assert(comparator <= 2);

    struct comparator_regs *regs = &hpet_regs.comparators[comparator];
    if (timer_n_in_periodic_mode(comparator)) {
        // TODO: only first comparator expected to be periodic.
        assert(comparator == 0);
        regs->current_comparator = counter_value_in_terms_of_timer(0) + data;
        regs->comparator_increment = data;
    } else {
        regs->current_comparator = data;
        regs->comparator_increment = 0;
    }

    return hpet_maintenance(comparator);
}

bool hpet_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                       decoded_instruction_ret_t decoded_ins)
{
    // if (ept_fault_is_read(qualification)) {
    //     assert(decoded_ins.type == INSTRUCTION_MEMORY);
    //     LOG_HPET("handling HPET read at offset 0x%lx, acc width %d bytes, reg idx %d\n", offset,
    //              mem_access_width_to_bytes(decoded_ins), decoded_ins.decoded.memory_instruction.target_reg);
    // } else if (ept_fault_is_write(qualification)) {
    //     uint64_t data;
    //     assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));
    //     LOG_HPET("handling HPET write at offset 0x%lx, value 0x%x\n", offset, data);
    // }

    // vcpu_print_regs(0);

    uint8_t comparator;
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
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                data = main_counter_value() & 0xffffffff;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                data = main_counter_value();
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%x\n", offset);
                return false;
            }

        } else if (offset == MAIN_COUNTER_VALUE_HIGH_MMIO_OFF) {
            data = main_counter_value() >> 32;

        } else if (hpet_fault_on_config(offset, &comparator)) {
            return hpet_fault_handle_config_read(comparator, &data, decoded_ins);
        } else if (hpet_fault_on_comparator(offset, &comparator)) {
            return hpet_fault_handle_comparator_read(comparator, &data, decoded_ins);
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

            hpet_regs.general_config |= GENERAL_CAP_MASK;

            if (!(old_config & ENABLE_CNF) && hpet_regs.general_config & ENABLE_CNF) {
                // Restarts the timers if the main counter have been restarted
                reset_main_counter();
                LOG_HPET("main counter restart!\n");
                hpet_maintenance(0);
                hpet_maintenance(1);
                hpet_maintenance(2);
            } else if ((old_config & ENABLE_CNF) && !(hpet_regs.general_config & ENABLE_CNF)) {
                hpet_frozen_counter = main_counter_value();
            }

        } else if (offset == MAIN_COUNTER_VALUE_MMIO_OFF || offset == MAIN_COUNTER_VALUE_HIGH_MMIO_OFF) {
            reset_main_counter();
        } else if (hpet_fault_on_config(offset, &comparator)) {
            return hpet_fault_handle_config_write(comparator, data, decoded_ins);
        } else if (hpet_fault_on_comparator(offset, &comparator)) {
            return hpet_fault_handle_comparator_write(comparator, data, decoded_ins);
        } else {
            LOG_VMM_ERR("Writing unknown HPET register offset 0x%x\n", offset);
            return false;
        }
    }

    return true;
}
