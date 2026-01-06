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

/* selector key values for "well-known" fw_cfg entries */
#define FW_CFG_SIGNATURE        0x00
#define FW_CFG_ID               0x01
#define FW_CFG_UUID             0x02
#define FW_CFG_RAM_SIZE         0x03
#define FW_CFG_NOGRAPHIC        0x04
#define FW_CFG_NB_CPUS          0x05
#define FW_CFG_MACHINE_ID       0x06
#define FW_CFG_KERNEL_ADDR      0x07
#define FW_CFG_KERNEL_SIZE      0x08
#define FW_CFG_KERNEL_CMDLINE   0x09
#define FW_CFG_INITRD_ADDR      0x0a
#define FW_CFG_INITRD_SIZE      0x0b
#define FW_CFG_BOOT_DEVICE      0x0c
#define FW_CFG_NUMA             0x0d
#define FW_CFG_BOOT_MENU        0x0e
#define FW_CFG_MAX_CPUS         0x0f
#define FW_CFG_KERNEL_ENTRY     0x10
#define FW_CFG_KERNEL_DATA      0x11
#define FW_CFG_INITRD_DATA      0x12
#define FW_CFG_CMDLINE_ADDR     0x13
#define FW_CFG_CMDLINE_SIZE     0x14
#define FW_CFG_CMDLINE_DATA     0x15
#define FW_CFG_SETUP_ADDR       0x16
#define FW_CFG_SETUP_SIZE       0x17
#define FW_CFG_SETUP_DATA       0x18
#define FW_CFG_FILE_DIR         0x19

#define FW_CFG_E820        0x20

#define FW_CFG_ID_TRADITIONAL BIT(0)
#define FW_CFG_ID_DMA BIT(1)

#define E820_FWCFG_FILE "etc/e820"

/* an individual file entry, 64 bytes total */
struct FWCfgFile {
    /* size of referenced fw_cfg item, big-endian */
    uint32_t size;
    /* selector key of fw_cfg item, big-endian */
    uint16_t select;
    uint16_t reserved;
    /* fw_cfg item name, NUL-terminated ascii */
    char name[56];
}  __attribute__((packed));

/* Structure of FW_CFG_FILE_DIR */
#define NUM_FW_CFG_FILES 1
struct fw_cfg_file_dir {
    uint32_t num_files; // Big endian!
    struct FWCfgFile file_entries[NUM_FW_CFG_FILES];
};

extern uint64_t guest_ram_size;

struct fw_cfg_e820_map {
    struct boot_e820_entry entries[1];
};

uint16_t selector;
// TODO: maybe find a better way of state tracking.
size_t selected_data_idx = 0;

const char *fw_cfg_signature = "QEMU";
struct fw_cfg_file_dir fw_cfg_file_dir = {
    .num_files = __builtin_bswap32(NUM_FW_CFG_FILES),
    .file_entries[0] = {
        .name = E820_FWCFG_FILE,
        .size = __builtin_bswap32(sizeof(struct fw_cfg_e820_map)),
        .select = __builtin_bswap16(FW_CFG_E820),
    }
};

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

bool emulate_qemu_fw_cfg(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, bool is_string, bool is_rep, ioport_access_width_t access_width) {
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
                assert(selected_data_idx < strlen(fw_cfg_signature));
                selected_data_idx += emulate_ioport_string_read(vctx, fw_cfg_signature, strlen(fw_cfg_signature), is_rep, access_width);
                break;
            case FW_CFG_ID: {
                uint32_t id = FW_CFG_ID_TRADITIONAL;
                selected_data_idx += emulate_ioport_string_read(vctx, (char *) &id, sizeof(uint32_t), is_rep, access_width);
                break;
            }
            case FW_CFG_FILE_DIR: {
                selected_data_idx += emulate_ioport_string_read(vctx, &((char *) &fw_cfg_file_dir)[selected_data_idx], sizeof(fw_cfg_file_dir) - selected_data_idx, is_rep, access_width);
                break;
            }
            case FW_CFG_E820: {
                // TODO: bad, being constructed on each call
                struct fw_cfg_e820_map fw_cfg_e820_map = {
                    .entries[0] = {
                        .addr = 0,
                        .size = guest_ram_size,
                        .type = E820_RAM,
                    }
                };

                selected_data_idx += emulate_ioport_string_read(vctx, &((char *) &fw_cfg_e820_map)[selected_data_idx], sizeof(fw_cfg_e820_map) - selected_data_idx, is_rep, access_width);
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
