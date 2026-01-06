#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/qemu_fw_cfg.h>

/* See https://wiki.osdev.org/QEMU_fw_cfg for reference. */

#define FW_CFG_PORT_SEL     0x510
#define FW_CFG_PORT_DATA    0x511
#define FW_CFG_PORT_DMA     0x514

uint16_t port_sel;

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
        } else {
            /* FW_CFG_PORT_DATA is read-only. */
            return false;
        }
    }

    return true;
}
