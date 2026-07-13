/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/guest_time.h>
#include <libvmm/arch/x86_64/cmos.h>
#include <sddf/timer/client.h>

/* Documents referenced:
 * https://web.stanford.edu/class/cs140/projects/pintos/specs/mc146818a.pdf
 * https://wiki.osdev.org/CMOS
 */

#define CMOS_REG_SECONDS 0x0
#define CMOS_REG_SECONDS_ALARM 0x1
#define CMOS_REG_MINUTES 0x2
#define CMOS_REG_MINUTES_ALARM 0x3
#define CMOS_REG_HOURS 0x4
#define CMOS_REG_HOURS_ALARM 0x5
#define CMOS_REG_WEEKDAY 0x6
#define CMOS_REG_DAY_OF_MONTH 0x7
#define CMOS_REG_MONTH 0x8
#define CMOS_REG_YEAR 0x9
#define CMOS_STS_A 0xA
#define CMOS_STS_B 0xB
#define CMOS_STS_C 0xC

#define CMOS_STS_B_UIE 4 /* Update-ended Interrupt Enable */
#define CMOS_STS_B_DM 2 /* Binary format. */
#define CMOS_STS_B_24 1 /* 24 hours time. */

#define CMOS_STS_C_UF 4 /* Update-ended Interrupt Flag */
#define CMOS_STS_C_IRQF 7 /* IRQ flag */

#define CMOS_IRQ_PIN 8

struct cmos_state {
    uint64_t latched_abs_seconds;
    uint8_t select_reg;
    uint8_t status_a;
    uint8_t status_b;
};

static struct cmos_state cmos_state = (struct cmos_state) {
    .status_a = 0x26, /* Sane default: 32.768 kHz time base */
    .status_b = BIT(CMOS_STS_B_DM) | BIT(CMOS_STS_B_24),
};

static bool handle_select_port(size_t qualification, seL4_VCPUContext *vctx)
{
    if (pio_fault_is_read(qualification)) {
        pio_emulate_read(qualification, vctx, cmos_state.select_reg);
    } else {
        cmos_state.select_reg = pio_get_write_data(qualification, vctx);
    }
    return true;
}

static uint8_t selected_cmos_port(void)
{
    /* The highest bit is the NMI enable bit. */
    return cmos_state.select_reg & 0x7F;
}

static void cmos_update_latched_time(void)
{
    /* This is kind of a hack, we just set the CMOS time to be equal to the TSC. */
    cmos_state.latched_abs_seconds = guest_time_tsc_now() / guest_time_tsc_hz();
}

static uint64_t cmos_absolute_seconds(void)
{
    return cmos_state.latched_abs_seconds;
}

static uint8_t cmos_seconds(void)
{
    return cmos_absolute_seconds() % 60;
}

static uint8_t cmos_minutes(void)
{
    return (cmos_absolute_seconds() / 60) % 60;
}

static uint8_t cmos_hours(void)
{
    return (cmos_absolute_seconds() / 3600) % 24;
}

static bool handle_data_port(size_t qualification, seL4_VCPUContext *vctx)
{
    if (pio_fault_is_read(qualification)) {
        uint8_t result;
        switch (selected_cmos_port()) {
        case CMOS_REG_SECONDS:
            /* We need the latch to protect against cases where the minutes or hours
             * overflows while the guest is reading the time. */
            cmos_update_latched_time();
            result = cmos_seconds();
            break;
        case CMOS_REG_MINUTES:
            result = cmos_minutes();
            break;
        case CMOS_REG_HOURS:
            result = cmos_hours();
            break;
        case CMOS_REG_SECONDS_ALARM:
        case CMOS_REG_MINUTES_ALARM:
        case CMOS_REG_HOURS_ALARM:
        case CMOS_STS_C:
            result = 0;
            break;
        case CMOS_REG_DAY_OF_MONTH:
            result = 0x29; /* seL4 day (https://sel4.discourse.group/t/happy-sel4-day-2025/999) */
            break;
        case CMOS_REG_MONTH:
            result = 0x7;
            break;
        case CMOS_REG_YEAR:
            result = 0x26;
            break;
        case CMOS_STS_A:
            result = cmos_state.status_a;
            break;
        case CMOS_STS_B:
            result = cmos_state.status_b;
            break;
        default:
            LOG_VMM_ERR("read from unimplemented CMOS reg 0x%x\n", selected_cmos_port());
            result = 0;
            break;
        }
        pio_emulate_read(qualification, vctx, result);
    } else {
        switch (selected_cmos_port()) {
        case CMOS_STS_A:
            cmos_state.status_a = pio_get_write_data(qualification, vctx);
            break;
        case CMOS_STS_B: {
            uint8_t old_sts_b = cmos_state.status_b;
            cmos_state.status_b = pio_get_write_data(qualification, vctx);
            uint8_t new_sts_b = cmos_state.status_b;
            if (!(old_sts_b & BIT(CMOS_STS_B_UIE)) && (new_sts_b & BIT(CMOS_STS_B_UIE))) {
                LOG_VMM_ERR("rtc update ended interrupt unimplemented\n");
                return false;
            }

            break;
        }
        default:
            LOG_VMM_ERR("write to unimplemented CMOS reg 0x%x, data 0x%x\n", selected_cmos_port(),
                        pio_get_write_data(qualification, vctx));
        }
    }

    return true;
}

bool cmos_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification, seL4_VCPUContext *vctx, void *cookie)
{
    if (port_offset == 0) {
        return handle_select_port(qualification, vctx);
    } else if (port_offset == 1) {
        return handle_data_port(qualification, vctx);
    } else {
        LOG_VMM_ERR("unknown port offset for CMOS: 0x%x\n", port_offset);
        return false;
    }
}