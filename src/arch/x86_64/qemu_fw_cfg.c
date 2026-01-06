#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/qemu_fw_cfg.h>

/* See references:
 * https://wiki.osdev.org/QEMU_fw_cfg for reference.
 * https://www.qemu.org/docs/master/specs/fw_cfg.html.
 */

#define FW_CFG_PORT_SEL     0x510
#define FW_CFG_PORT_DATA    0x511
#define FW_CFG_PORT_DMA     0x514

#define FW_CFG_SIGNATURE    0x0000
#define FW_CFG_ID           0x0001
#define FW_CFG_FILE_DIR     0x0019

/* Currently selected port to control. */
uint16_t port_sel;

const char *fw_cfg_signature = "QEMU";
// TODO: maybe find a better way of state tracking.
size_t sig_idx = 0;

static bool emulate_qemu_fw_cfg_signature(seL4_VCPUContext *vctx, bool is_read) {
    if (is_read) {
        vctx->eax = fw_cfg_signature[sig_idx];
        // TODO
        if (sig_idx >= 4) {
            vctx->eax = 0;
        } else {
            sig_idx++;
        }

        return true;
    } else {
        return false;
    }
}

static bool emulate_qemu_fw_cfg_

bool emulate_qemu_fw_cfg(seL4_VCPUContext *vctx, bool is_read, uint16_t port_addr) {
    switch (port_addr) {
    case FW_CFG_PORT_SEL:
        if (!is_read) {
            port_sel = vctx->eax;
            LOG_VMM("fw cfg port_sel is 0x%x\n", port_sel);
        } else {
            /* FW_CFG_PORT_SEL is write-only register. */
            return false;
        }
        break;
    case FW_CFG_PORT_DATA:
        if (is_read) {
            switch (port_sel) {
            case FW_CFG_SIGNATURE:
                vctx->eax = 0;
                break;
            default:
                return false;
            }
        } else {
            /* FW_CFG_PORT_DATA is read-only. */
            return false;
        }
    }

    return true;
}
