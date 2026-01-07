/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/cmos.h>

// Emulates enough of the CMOS and Real-Time Clock (RTC) needed for OVMF UEFI firmware.
// https://wiki.osdev.org/CMOS
// https://www.ti.com/lit/ds/symlink/bq3285.pdf?ts=1767742813264&ref_url=https%253A%252F%252Fwww.google.com%252F

/* Uncomment this to enable debug logging */
// #define DEBUG_CMOS

#if defined(DEBUG_CMOS)
#define LOG_CMOS(...) do{ printf("%s|CMOS: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_CMOS(...) do{}while(0)
#endif

#define STATUS_REG_A 0xa
#define STATUS_REG_B 0xb
#define STATUS_REG_C 0xc
#define STATUS_REG_D 0xd

struct cmos_regs {
    uint8_t selected_reg;
    uint8_t status_registers[4]; // status regs A/B/C/D
};

static struct cmos_regs cmos_regs = {
    .status_registers[3] = 0x80 // VRT bit set meaning device OK.
};

bool emulate_cmos_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, ioport_access_width_t access_width)
{
    assert(access_width == IOPORT_BYTE_ACCESS_QUAL);

    if (port_addr == 0x70) {
        if (is_read) {
            vctx->eax = cmos_regs.selected_reg;
        } else {
            cmos_regs.selected_reg = vctx->eax & 0x7f;
        }
    } else if (port_addr == 0x71) {
        if (is_read) {
            LOG_CMOS("reading CMOS register 0x%x\n", cmos_regs.selected_reg);
            switch (cmos_regs.selected_reg) {
                case STATUS_REG_A:
                    vctx->eax = cmos_regs.status_registers[0];
                    break;
                case STATUS_REG_B:
                    vctx->eax = cmos_regs.status_registers[1];
                    break;
                case STATUS_REG_C:
                    vctx->eax = cmos_regs.status_registers[2];
                    // whenever this register is read it is cleared
                    cmos_regs.status_registers[2] = 0;
                    break;
                case STATUS_REG_D:
                    vctx->eax = cmos_regs.status_registers[3];
                    break;
            }
        } else {
            LOG_CMOS("writing CMOS register 0x%x, value 0x%x\n", cmos_regs.selected_reg, vctx->eax);
            switch (cmos_regs.selected_reg) {
                case STATUS_REG_A:
                    cmos_regs.status_registers[0] = vctx->eax;

                    // uint8_t freq_select = cmos_regs.status_registers[0] & 0xf;
                    // uint8_t oscillator_ctl = (cmos_regs.status_registers[0] >> 4) & 0x7;

                    break;
                case STATUS_REG_B:
                    cmos_regs.status_registers[1] = vctx->eax;
                    break;
                case STATUS_REG_C:
                    cmos_regs.status_registers[2] = vctx->eax;
                    break;
                case STATUS_REG_D:
                    // cmos_regs.status_registers[3] = vctx->eax;
                    break;
            }
        }

    } else {
        LOG_VMM_ERR("emulate_cmos_access() called with unknown port address 0x%x\n", port_addr);
        return false;
    }

    return true;
}