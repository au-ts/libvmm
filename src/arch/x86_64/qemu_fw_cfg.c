#include <inttypes.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/qemu_fw_cfg.h>
#include <libvmm/arch/x86_64/e820.h>

/* See references:
 * https://wiki.osdev.org/QEMU_fw_cfg for reference.
 * https://www.qemu.org/docs/master/specs/fw_cfg.html.
 */

#define FW_CFG_PORT_SEL     0x510
#define FW_CFG_PORT_DATA    0x511
#define FW_CFG_PORT_DMA     0x514

#define FW_CFG_ID_TRADITIONAL BIT(0)
#define FW_CFG_ID_DMA BIT(1)

#define FW_CFG_SIGNATURE_STR "QEMU"

// This will be populated by uefi_setup_images()
struct fw_cfg_blobs fw_cfg_blobs;

uint16_t selector;
// TODO: maybe find a better way of state tracking.
size_t selected_data_idx = 0;

// static bool emulate_qemu_fw_cfg_signature(seL4_VCPUContext *vctx, bool is_read) {
//     if (is_read) {
//         vctx->eax = fw_cfg_signature[sig_idx];
//         // TODO
//         if (sig_idx >= 4) {
//             vctx->eax = 0;
//         } else {
//             sig_idx++;
//         }

//         return true;
//     } else {
//         return false;
//     }
// }

static bool emulate_qemu_fw_cfg_id(seL4_VCPUContext *vctx, bool is_read) {
    if (!is_read) {
        return false;
    }

    vctx->eax = FW_CFG_ID_TRADITIONAL;

    return true;
}

// bool qemu_fw_cfg_add()

// @billn revisit buffer overflows
bool emulate_qemu_fw_cfg_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, bool is_string, bool is_rep, ioport_access_width_t access_width) {
    // LOG_VMM("port_addr: 0x%x\n", port_addr);
    switch (port_addr) {
    case FW_CFG_PORT_SEL:
        if (!is_read) {
            assert(!is_string);
            assert(!is_rep);
            selector = vctx->eax;
            selected_data_idx = 0;

            // LOG_VMM("fw cfg port_sel is 0x%x\n", port_sel);
        } else {
            /* FW_CFG_PORT_SEL is write-only register. */
            return false;
        }
        break;
    case FW_CFG_PORT_DATA:
        assert(access_width == IOPORT_BYTE_ACCESS_QUAL);

        if (is_read && is_string) {
            switch (selector) {
            case FW_CFG_SIGNATURE:
                assert(selected_data_idx < strlen(FW_CFG_SIGNATURE_STR));
                selected_data_idx += emulate_ioport_string_read(vctx, FW_CFG_SIGNATURE_STR, strlen(FW_CFG_SIGNATURE_STR), is_rep, access_width);
                break;
            case FW_CFG_ID: {
                uint32_t id = FW_CFG_ID_TRADITIONAL;
                selected_data_idx += emulate_ioport_string_read(vctx, (char *) &id, sizeof(uint32_t), is_rep, access_width);
                break;
            }
            case FW_CFG_FILE_DIR: {
                selected_data_idx += emulate_ioport_string_read(vctx, &((char *) &fw_cfg_blobs.fw_cfg_file_dir)[selected_data_idx], sizeof(fw_cfg_blobs.fw_cfg_file_dir) - selected_data_idx, is_rep, access_width);
                break;
            }
            case FW_CFG_E820: {
                selected_data_idx += emulate_ioport_string_read(vctx, &((char *) &fw_cfg_blobs.fw_cfg_e820_map)[selected_data_idx], sizeof(fw_cfg_blobs.fw_cfg_e820_map) - selected_data_idx, is_rep, access_width);
                break;
            }
            case FW_CFG_BOOT_MENU: {
                uint16_t boot_menu = 0;
                selected_data_idx += emulate_ioport_string_read(vctx, (char *) &boot_menu, sizeof(uint16_t), is_rep, access_width);
                break;
            }
            case FW_CFG_NB_CPUS: {
                // TODO: revisit
                uint16_t nb_cpus = 1;
                selected_data_idx += emulate_ioport_string_read(vctx, (char *) &nb_cpus, sizeof(uint16_t), is_rep, access_width);
                break;
            }
            default:
                LOG_VMM_ERR("unknown fw cfg selector for port data string: 0x%x\n", selector);
                return false;
            }
        } else if (is_read && !is_string) {
            LOG_VMM_ERR("implement me\n");
            return false;
        } else {
            /* FW_CFG_PORT_DATA is read-only. */
            return false;
        }
        break;
    default:
        LOG_VMM_ERR("unknown fw cfg IO port 0x%x\n", port_addr);
        return false;
    }

    return true;
}
