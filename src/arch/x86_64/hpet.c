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
#include <libvmm/arch/x86_64/guest_time.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/guest.h>
#include <sel4/arch/vmenter.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>

/* Implements a minimum HPET specification (10Mhz counter, 3 comparators with 1 being periodic capable) */

/* Document referenced:
 * https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf
 */

#define GENERAL_CAP_REG_MMIO_OFF 0x0
#define GENERAL_CAP_REG_HIGH_MMIO_OFF 0x4

#define GENERAL_CONFIG_REG_MMIO_OFF 0x10
#define GENERAL_CONFIG_REG_HIGH_MMIO_OFF 0x14

#define GENERAL_ISR_MMIO_OFF 0x20

#define MAIN_COUNTER_VALUE_MMIO_OFF 0xf0
#define MAIN_COUNTER_VALUE_HIGH_MMIO_OFF 0xf4

#define HPET_MAIN_COUNTER_HZ 10000000
// General Capability register
#define NS_IN_FS 1000000ul
// Main counter tick period in femtosecond. 10MHz tick
#define COUNTER_CLK_PERIOD_VAL (NS_IN_FS * 100)
#define COUNTER_CLK_PERIOD_SHIFT 32
// Legacy IRQ replacement capable (replace the old PIT)
#define LEG_RT_CAP BIT(15)
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
#define Tn_PER_INT_CAP BIT(4) // Periodic capable
#define Tn_INT_ENB_CNF BIT(2) // irq on
#define Tn_INT_TYPE_CNF BIT(1) // irq type, 0 = edge, 1 = level
#define Tn_TYPE_CNF BIT(3) // periodic mode on
#define Tn_INT_ROUTE_CNF_SHIFT 9
#define Tn_INT_ROUTE_CAP_SHIFT 32

// I/O APIC routing if no legacy
#define TIM0_IOAPIC_PIN 13ull
#define TIM1_IOAPIC_PIN 14ull
#define TIM2_IOAPIC_PIN 15ull

struct comparator_regs {
    uint64_t config;
    uint64_t config_mask;
    uint32_t current_comparator;
    uint32_t armed_comparator;
    uint32_t comparator_increment;

    bool timeout_handle_valid;
    guest_timeout_handle_t timeout_handle;
};

struct hpet_regs {
    uint64_t general_capabilities; // RO
    uint64_t general_config; // RW
    uint64_t isr; // RW
    struct comparator_regs comparators[NUM_TIM_CAP_VAL + 1];
};

static uint32_t hpet_frozen_counter = 0;
static uint32_t hpet_counter_offset = 0;

#define GENERAL_CAP_MASK ((REV_ID | (NUM_TIM_CAP_VAL << NUM_TIM_CAP_SHIFT) | LEG_RT_CAP | (COUNTER_CLK_PERIOD_VAL << COUNTER_CLK_PERIOD_SHIFT)) | VENDOR_ID)
#define TIM0_CONF_MASK (Tn_PER_INT_CAP | (BIT(TIM0_IOAPIC_PIN) << Tn_INT_ROUTE_CAP_SHIFT))
#define TIM1_CONF_MASK ((BIT(TIM1_IOAPIC_PIN) << Tn_INT_ROUTE_CAP_SHIFT))
#define TIM2_CONF_MASK ((BIT(TIM2_IOAPIC_PIN) << Tn_INT_ROUTE_CAP_SHIFT))

static struct hpet_regs hpet_regs = {
    // 32-bit main counter, 3 comparators (only 1 periodic capable), legacy IRQ routing capable, tick rate = 10MHz
    .general_capabilities = GENERAL_CAP_MASK,
    .comparators[0] = { .config = TIM0_CONF_MASK, .config_mask = TIM0_CONF_MASK, .timeout_handle_valid = false },
    .comparators[1] = { .config = TIM1_CONF_MASK, .config_mask = TIM1_CONF_MASK, .timeout_handle_valid = false },
    .comparators[2] = { .config = TIM2_CONF_MASK, .config_mask = TIM2_CONF_MASK, .timeout_handle_valid = false },
};

static uint32_t time_now_32(void)
{
    // Convert TSC to 10 Mhz
    return convert_ticks_by_frequency(guest_time_tsc_now(), guest_time_tsc_hz(), HPET_MAIN_COUNTER_HZ) & 0xffffffff;
}

static bool counter_on(void)
{
    return (hpet_regs.general_config & ENABLE_CNF);
}

static uint32_t main_counter_value(void)
{
    if (counter_on()) {
        return time_now_32() - hpet_counter_offset;
    } else {
        return hpet_frozen_counter;
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
    return counter_on() && !!(hpet_regs.comparators[n].config & Tn_INT_ENB_CNF);
}

static bool timer_n_in_periodic_mode(int n)
{
    return !!(hpet_regs.comparators[n].config & Tn_TYPE_CNF);
}

static bool timer_n_irq_edge_triggered(int n)
{
    return (hpet_regs.comparators[n].config & Tn_INT_TYPE_CNF) == 0;
}

uint64_t timer_n_compute_timeout_delta_as_tsc(int n, uint32_t main_counter_val)
{
    uint32_t delay_hpet_ticks = hpet_regs.comparators[n].current_comparator - main_counter_val;
    return convert_ticks_by_frequency(delay_hpet_ticks, HPET_MAIN_COUNTER_HZ, guest_time_tsc_hz());
}

void hpet_maintenance(uint8_t comparator);

void hpet_handle_timer_ntfn(uint64_t comparator)
{
    assert(comparator <= NUM_TIM_CAP_VAL);

    if (!counter_on()) {
        return;
    }

    if (timer_n_can_interrupt(comparator)) {
        int ioapic_pin = get_timer_n_ioapic_pin(comparator);
        inject_ioapic_irq(comparator, ioapic_pin);
    }

    /* Comparator 0 is periodic capable. */
    if (comparator == 0 && timer_n_in_periodic_mode(0) && hpet_regs.comparators[0].comparator_increment) {
        uint32_t main_counter_val = main_counter_value();
        hpet_regs.comparators[0].current_comparator = main_counter_val + hpet_regs.comparators[0].comparator_increment;
        hpet_maintenance(0);
    }
}

void hpet_maintenance(uint8_t comparator)
{
    assert(comparator <= NUM_TIM_CAP_VAL);

    if (!counter_on()) {
        return;
    }

    /* Special case for comparator 0 since it is periodic capable. */
    if (comparator == 0 && timer_n_in_periodic_mode(0) && hpet_regs.comparators[0].comparator_increment == 0) {
        /* Halted */
        return;
    }

    uint32_t main_counter_val = main_counter_value();
    uint64_t delay_tsc_ticks = timer_n_compute_timeout_delta_as_tsc(comparator, main_counter_val);

    if (delay_tsc_ticks) {
        if (hpet_regs.comparators[comparator].timeout_handle_valid) {
            guest_time_cancel_timeout(hpet_regs.comparators[comparator].timeout_handle);
            hpet_regs.comparators[comparator].timeout_handle_valid = false;
        }

        hpet_regs.comparators[comparator].timeout_handle = guest_time_request_timeout(
            delay_tsc_ticks, &hpet_handle_timer_ntfn, comparator);
        assert(hpet_regs.comparators[comparator].timeout_handle != TIMEOUT_HANDLE_INVALID);
        hpet_regs.comparators[comparator].timeout_handle_valid = true;

        hpet_regs.comparators[comparator].armed_comparator = hpet_regs.comparators[comparator].current_comparator;
    }
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
    assert(comparator <= NUM_TIM_CAP_VAL);
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
    assert(comparator <= NUM_TIM_CAP_VAL);
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
    assert(comparator <= NUM_TIM_CAP_VAL);
    bool periodic_old = timer_n_in_periodic_mode(comparator);

    struct comparator_regs *regs = &hpet_regs.comparators[comparator];

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

    regs->config |= regs->config_mask;

    bool periodic_new = timer_n_in_periodic_mode(comparator);

    if (periodic_old && !periodic_new) {
        assert(comparator == 0);
    }

    return true;
}

static bool hpet_fault_handle_comparator_write(uint8_t comparator, uint64_t data, decoded_instruction_ret_t decoded_ins)
{
    assert(comparator <= 2);

    struct comparator_regs *regs = &hpet_regs.comparators[comparator];
    if (timer_n_in_periodic_mode(comparator)) {
        // only first comparator expected to be periodic.
        assert(comparator == 0);

        if (regs->config & Tn_VAL_SET_CNF) {
            // writing expiry
            regs->current_comparator = data;
            regs->config &= ~Tn_VAL_SET_CNF;
            hpet_maintenance(comparator);
            return true;
        } else {
            // writing the increment
            regs->comparator_increment = data;
            return true;
        }
    } else {
        regs->current_comparator = data;
        regs->comparator_increment = 0;
        regs->config &= ~Tn_VAL_SET_CNF;
        hpet_maintenance(comparator);
        return true;
    }
}

bool hpet_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                       decoded_instruction_ret_t decoded_ins)
{
    uint8_t comparator;
    if (ept_fault_is_read(qualification)) {
        uint64_t data;
        if (offset == GENERAL_CAP_REG_MMIO_OFF) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                data = hpet_regs.general_capabilities & 0xffffffff;
            } else if (mem_access_width_to_bytes(decoded_ins) == 8) {
                data = hpet_regs.general_capabilities;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%lx\n", offset);
                return false;
            }

        } else if (offset == GENERAL_CAP_REG_HIGH_MMIO_OFF) {
            if (mem_access_width_to_bytes(decoded_ins) == 4) {
                data = hpet_regs.general_capabilities >> 32;
            } else {
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%lx\n", offset);
                return false;
            }

        } else if (offset == GENERAL_CONFIG_REG_MMIO_OFF) {
            data = hpet_regs.general_config;

        } else if (offset == MAIN_COUNTER_VALUE_MMIO_OFF) {
            data = main_counter_value();

        } else if (offset == MAIN_COUNTER_VALUE_HIGH_MMIO_OFF) {
            /* 32-bit counter */
            data = 0;

        } else if (hpet_fault_on_config(offset, &comparator)) {
            return hpet_fault_handle_config_read(comparator, &data, decoded_ins);
        } else if (hpet_fault_on_comparator(offset, &comparator)) {
            return hpet_fault_handle_comparator_read(comparator, &data, decoded_ins);
        } else {
            LOG_VMM_ERR("Reading unknown HPET register offset 0x%lx\n", offset);
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
                LOG_VMM_ERR("Unsupported access width on HPET offset 0x%lx\n", offset);
                return false;
            }

            if (!(old_config & ENABLE_CNF) && hpet_regs.general_config & ENABLE_CNF) {
                // Restarts the timers if the main counter have been restarted
                reset_main_counter();
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
            LOG_VMM_ERR("Writing unknown HPET register offset 0x%lx\n", offset);
            return false;
        }
    }

    return true;
}
