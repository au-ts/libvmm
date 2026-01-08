/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/pit.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/guest.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>

// Programmable Interval Timer virtualisation

#define CH0_PORT 0x40
#define CH1_PORT 0x41
#define CH2_PORT 0x42
#define CMD_PORT 0x43

#define PIT_FREQUENCY_MHZ 1.193182
#define PIT_TICK_TIME_US (1 / PIT_FREQUENCY_MHZ)

typedef enum pit_state {
    RESET,
    CH0_MODE2_WAITING_RELOAD_VALUE_LOW,
    CH0_MODE2_WAITING_RELOAD_VALUE_HIGH,
    CH0_MODE2_TICKING,
} pit_state_t;

struct pit {
    pit_state_t state;
    uint16_t ch0_reload;
};

static struct pit global_pit = { .state = RESET };

bool emulate_pit_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read)
{
    return true;

    switch (port_addr) {
    case CH0_PORT: {
        uint8_t val = vctx->eax & 0xff;
        if (global_pit.state == CH0_MODE2_WAITING_RELOAD_VALUE_LOW) {
            global_pit.ch0_reload = val;
            global_pit.state = CH0_MODE2_WAITING_RELOAD_VALUE_HIGH;
        } else if (global_pit.state == CH0_MODE2_WAITING_RELOAD_VALUE_HIGH) {
            global_pit.ch0_reload |= ((uint16_t) val) << 8;
            global_pit.state = CH0_MODE2_TICKING;

            if (global_pit.ch0_reload == 1) {
                LOG_VMM_ERR("Reload value of 1 cannot be used for PIT mode 2!\n");
                return false;
            }

            LOG_VMM("PIT ch0 mode2 reload value 0x%x\n", global_pit.ch0_reload);
            double tick_us = global_pit.ch0_reload * PIT_TICK_TIME_US;
            uint64_t tick_ns = (uint64_t) (tick_us * NS_IN_US);

            sddf_timer_set_timeout(TIMER_DRV_CH_FOR_PIT, tick_ns * 10);
        } else {
            LOG_VMM_ERR("Invalid PIT state\n");
        }
        break;
    }
    case CMD_PORT: {
        uint8_t cmd = vctx->eax & 0xff;
        uint8_t is_read_back = ((cmd >> 6) & 0x3) == 0x3;
        if (is_read_back) {
            // uint8_t read_back_ch0 = ((global_pit.cmd >> 1) & 0x1);
            // uint8_t read_back_ch1 = ((global_pit.cmd >> 2) & 0x1);
            // uint8_t read_back_ch2 = ((global_pit.cmd >> 3) & 0x1);
            // uint8_t latch_status_flag = ((global_pit.cmd >> 4) & 0x1);
            // uint8_t latch_count_flag = ((global_pit.cmd >> 5) & 0x1);
            LOG_VMM_ERR("unimplemented pit readback\n");
            return false;
        } else {
            uint8_t channel_select = (vctx->eax >> 6) & 0x3;
            // uint8_t access_mode = (vctx->eax >> 4) & 0x3;
            uint8_t operating_mode = (vctx->eax >> 1) & 0x7;
            uint8_t is_binary = !(vctx->eax & 0x1);

            if (is_read) {
                LOG_VMM_ERR("Read of write-only PIT command register\n");
                return false;
            }
            if (!is_binary) {
                LOG_VMM_ERR("PIT BCD mode unimplemented\n");
                return false;
            }

            // LOG_VMM("channel_select %d\n", channel_select);
            // LOG_VMM("access_mode %d\n", access_mode);
            // LOG_VMM("operating_mode %d\n", operating_mode);
            // LOG_VMM("is_binary %d\n", is_binary);

            if (channel_select == 0) {
                if (operating_mode == 2) {
                    global_pit.state = CH0_MODE2_WAITING_RELOAD_VALUE_LOW;
                    return true;
                }
            }
        }
    }
    default:
        LOG_VMM_ERR("unhandled pit io port 0x%x\n", port_addr);
        return false;
    }

    return true;
}

// @billn revisit whether it is correct to do it like this, looks sus af
extern struct lapic_regs lapic_regs;
void pit_handle_timer_ntfn(void)
{
    assert(global_pit.state == CH0_MODE2_TICKING);
    double tick_us = global_pit.ch0_reload * PIT_TICK_TIME_US;
    uint64_t tick_ns = (uint64_t) (tick_us * NS_IN_US);
    sddf_timer_set_timeout(TIMER_DRV_CH_FOR_PIT, tick_ns * 10);

    // PIT ch0 irq is always 0
    if (!(lapic_regs.lint0 & BIT(16))) {
        inject_lapic_irq(GUEST_BOOT_VCPU_ID, lapic_regs.lint0 & 0xff);
    }
}
