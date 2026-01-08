#include <inttypes.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/qemu/fw_cfg.h>
#include <libvmm/arch/x86_64/qemu/ramfb.h>
#include <libvmm/arch/x86_64/e820.h>

// TODO: handle this
// If the DMA interface is available, then reading the DMA Address Register returns 0x51454d5520434647 (QEMU CFG in big-endian format).

// TODO: sort out and rename
uint64_t fb_vaddr = 0xa0000000;
uint64_t fb_paddr = 0x7000000;

/* See references:
 * https://wiki.osdev.org/QEMU_fw_cfg
 * https://www.qemu.org/docs/master/specs/fw_cfg.html
 */

#define FW_CFG_PORT_SEL     0x510
#define FW_CFG_PORT_DATA    0x511
#define FW_CFG_PORT_DMA     0x514

#define FW_CFG_ID_TRADITIONAL BIT(0)
#define FW_CFG_ID_DMA BIT(1)

#define FW_CFG_DEFAULT_ID (FW_CFG_ID_TRADITIONAL | FW_CFG_ID_DMA)

#define FW_CFG_SIGNATURE_STR "QEMU"

// This will be populated by uefi_setup_images()
struct fw_cfg_blobs fw_cfg_blobs;

// fw_cfg DMA commands
typedef enum fw_cfg_ctl_t {
    fw_ctl_error = 1,
    fw_ctl_read = 2,
    fw_ctl_skip = 4,
    fw_ctl_select = 8,
    fw_ctl_write = 16
} fw_cfg_ctl_t;

typedef struct FWCfgDmaAccess {
    uint32_t control;
    uint32_t length;
    uint64_t address;
} FWCfgDmaAccess;

struct FWCfgFiles {
    uint32_t count;
    struct FWCfgFile files[];
} __attribute__((packed));

uint16_t selector;
// TODO: maybe find a better way of state tracking.
size_t selected_data_idx = 0;

static void fw_cfg_dma_write(uint32_t control, uint32_t length, uint64_t address);
static void fw_cfg_select(uint16_t port);
static bool fw_cfg_find_file(struct FWCfgFile *out, const char *name);

// @billn revisit buffer overflows
bool emulate_qemu_fw_cfg_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, bool is_string, bool is_rep, ioport_access_width_t access_width) {
    LOG_VMM("fw cfg access on port 0x%x\n", port_addr);
    switch (port_addr) {
    case FW_CFG_PORT_SEL:
        if (!is_read) {
            assert(!is_string);
            assert(!is_rep);
            selector = vctx->eax;
            selected_data_idx = 0;

            LOG_VMM("fw cfg selected port 0x%x\n", selector);
        } else {
            /* FW_CFG_PORT_SEL is write-only register. */
            LOG_VMM_ERR("attempted to write to FW_CFG_PORT_SEL\n");
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
                uint32_t id = FW_CFG_DEFAULT_ID;
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
            case FW_CFG_ACPI_TABLES: {
                selected_data_idx += emulate_ioport_string_read(vctx, &((char *) &fw_cfg_blobs.fw_acpi_tables)[selected_data_idx], sizeof(fw_cfg_blobs.fw_acpi_tables) - selected_data_idx, is_rep, access_width);
                break;
            }
            case FW_CFG_ACPI_RSDP: {
                selected_data_idx += emulate_ioport_string_read(vctx, &((char *) &fw_cfg_blobs.fw_xsdp)[selected_data_idx], sizeof(fw_cfg_blobs.fw_xsdp) - selected_data_idx, is_rep, access_width);
                break;
            }
            case FW_CFG_TABLE_LOADER: {
                selected_data_idx += emulate_ioport_string_read(vctx, &((char *) &fw_cfg_blobs.fw_table_loader)[selected_data_idx], sizeof(fw_cfg_blobs.fw_table_loader) - selected_data_idx, is_rep, access_width);
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
            LOG_VMM_ERR("attempted to write to FW_CFG_PORT_DATA\n");
            return false;
        }
        break;
    case FW_CFG_PORT_DMA:
    case FW_CFG_PORT_DMA + 4: {
        assert(!is_read);

        assert(access_width == IOPORT_DWORD_ACCESS_QUAL);
        uint64_t gpa = __builtin_bswap32(vctx->eax);
        // TODO: this is wrong, right now we assume all DMA accesses are with 32-bit memory
        if (gpa == 0) {
            return true;
        }
        FWCfgDmaAccess *cmd = (FWCfgDmaAccess *)gpa_to_vaddr(gpa);

        uint32_t control = __builtin_bswap32(cmd->control);
        // TODO: sanitize length
        uint32_t length = __builtin_bswap32(cmd->length);
        uint64_t dma_gpa = __builtin_bswap64(cmd->address);
        void *dma_vaddr = gpa_to_vaddr(dma_gpa);
        LOG_VMM("DMA with data at GPA 0x%lx, length: 0x%lx, control: 0x%lx\n", dma_gpa, length, control);
        switch (control) {
            case fw_ctl_read: {
                LOG_VMM("fw cfg DMA read (item 0x%x)\n", selector);
                if (selector == FW_CFG_FILE_DIR) {
                    memcpy(dma_vaddr, &((char *) &fw_cfg_blobs.fw_cfg_file_dir)[selected_data_idx], length);
                } else if (selector == FW_CFG_SIGNATURE) {
                    // TODO: should take into account selected_data_idx?
                    assert(length <= strlen(FW_CFG_SIGNATURE_STR));
                    memcpy(dma_vaddr, FW_CFG_SIGNATURE_STR, length);
                } else if (selector == FW_CFG_ID) {
                    assert(length <= sizeof(uint32_t));
                    uint32_t id = FW_CFG_DEFAULT_ID;
                    memcpy(dma_vaddr, (char *)&id, length);
                } else if (selector == FW_CFG_E820) {
                    memcpy(dma_vaddr, &((char *) &fw_cfg_blobs.fw_cfg_e820_map)[selected_data_idx], length);
                } else if (selector == FW_CFG_BOOT_MENU) {
                    assert(length <= sizeof(uint16_t));
                    uint16_t boot_menu = 0;
                    memcpy(dma_vaddr, (char *) &boot_menu, length);
                } else if (selector == FW_CFG_NB_CPUS) {
                    assert(length <= sizeof(uint16_t));
                    // TODO: revisit
                    uint16_t nb_cpus = 1;
                    memcpy(dma_vaddr, (char *) &nb_cpus, length);
                } else if (selector == FW_CFG_ACPI_TABLES) {
                    memcpy(dma_vaddr, &((char *) &fw_cfg_blobs.fw_acpi_tables)[selected_data_idx], length);
                } else if (selector == FW_CFG_ACPI_RSDP) {
                    memcpy(dma_vaddr, &((char *) &fw_cfg_blobs.fw_xsdp)[selected_data_idx], length);
                } else if (selector == FW_CFG_TABLE_LOADER) {
                    memcpy(dma_vaddr, &((char *) &fw_cfg_blobs.fw_table_loader)[selected_data_idx], length);
                } else {
                    LOG_VMM_ERR("unknown fw cfg DMA read for selector 0x%x\n", selector);
                    return false;
                }
                selected_data_idx += length;
                /* To indicate to guest that the DMA command was successful. */
                cmd->control = 0;
                break;
            }
            case fw_ctl_write: {
                LOG_VMM("=============== fw cfg DMA write (item 0x%x)\n", selector);
                if (selector == FW_CFG_FRAMEBUFFER) {
                    struct QemuRamFBCfg *guest_ramfb_cfg = (struct QemuRamFBCfg *)dma_vaddr;
                    assert(length == sizeof(struct QemuRamFBCfg));
                    LOG_VMM("RAM FB cfg DMA, width: %d, height: %d\n",
                            __builtin_bswap32(guest_ramfb_cfg->width), __builtin_bswap32(guest_ramfb_cfg->height));

                    // We need to take this config and apply it to the real QEMU config.
                    // 1. Find the real QEMU ramfb
                    struct FWCfgFile ramfb_file;
                    bool found = fw_cfg_find_file(&ramfb_file, FRAMEBFUFER_FWCFG_FILE);
                    assert(found);
                    // 2. Select it with the real QEMU fw-cfg
                    fw_cfg_select(__builtin_bswap16(ramfb_file.select));
                    // 3. Take the guest's framebuffer config, copy it to our DMA region and then do the DMA write.
                    memcpy((void *)fb_vaddr, (void *)guest_ramfb_cfg, sizeof(struct QemuRamFBCfg));
                    fw_cfg_dma_write(fw_ctl_write, sizeof(struct QemuRamFBCfg), fb_paddr);
                    // 4. Check that the DMA write with QEMU succeeded.
                    FWCfgDmaAccess *cmd = (FWCfgDmaAccess *)fb_vaddr;
                    assert(!cmd->control);
                } else {
                    LOG_VMM_ERR("unknown fw cfg DMA write for selector 0x%x\n", selector);
                    return false;
                }
                /* To indicate to guest that the DMA command was successful. */
                selected_data_idx += length;
                cmd->control = 0;
                break;
            }
            default:
                LOG_VMM_ERR("unknown fw cfg DMA control field 0x%x (item 0x%x)\n", control, selector);
                return false;
        }

        break;
    }
    default:
        LOG_VMM_ERR("unknown fw cfg IO port 0x%x\n", port_addr);
        return false;
    }

    return true;
}

// TODO: should be able to make these microkit_ioport type but it looks like Microkit does not
// allow non-word sized setvars...
uint64_t qemu_fw_cfg_port_addr_sel;
uint64_t qemu_fw_cfg_port_addr_dma;

static void fw_cfg_select(uint16_t port) {
    microkit_x86_ioport_write_16(qemu_fw_cfg_port_addr_sel, FW_CFG_PORT_SEL, port);
}

static void fw_cfg_read_buf(char *buf, int length) {
    for (int i = 0; i < length; i++) {
        buf[i] = microkit_x86_ioport_read_8(qemu_fw_cfg_port_addr_sel, FW_CFG_PORT_DATA);
    }
}

static uint32_t fw_cfg_read_u32() {
    uint32_t a = microkit_x86_ioport_read_8(qemu_fw_cfg_port_addr_sel, FW_CFG_PORT_DATA);
    uint32_t b = microkit_x86_ioport_read_8(qemu_fw_cfg_port_addr_sel, FW_CFG_PORT_DATA);
    uint32_t c = microkit_x86_ioport_read_8(qemu_fw_cfg_port_addr_sel, FW_CFG_PORT_DATA);
    uint32_t d = microkit_x86_ioport_read_8(qemu_fw_cfg_port_addr_sel, FW_CFG_PORT_DATA);

    return (d << 24) | (c << 16) | (b << 8) | a;
}

uint64_t fw_cfg_dma_cmd_paddr = 0x6000000;
uint64_t fw_cfg_dma_cmd_vaddr = 0xb0000000;

static void fw_cfg_dma_write(uint32_t control, uint32_t length, uint64_t address) {
    FWCfgDmaAccess *cmd = (FWCfgDmaAccess *)fw_cfg_dma_cmd_vaddr;
    cmd->control = __builtin_bswap32(control);
    cmd->length = __builtin_bswap32(length);
    cmd->address = __builtin_bswap64(address);
    LOG_VMM("dma control before write: 0x%x\n", __builtin_bswap32(cmd->control));

    assert(fw_cfg_dma_cmd_paddr < (2ULL << 32) - 1);
    microkit_x86_ioport_write_32(qemu_fw_cfg_port_addr_dma, FW_CFG_PORT_DMA + 4, __builtin_bswap32(fw_cfg_dma_cmd_paddr));

    LOG_VMM("dma control after write: 0x%x\n", __builtin_bswap32(cmd->control));
}

/* Code for actually accessing the QEMU fw cfg, rather than emulating it. */
static bool fw_cfg_find_file(struct FWCfgFile *out, const char *name) {
    LOG_VMM("selecting signature\n");
    fw_cfg_select(FW_CFG_SIGNATURE);

    char signature_buf[5];
    signature_buf[4] = '\0';
    fw_cfg_read_buf(signature_buf, 4);

    printf("signature: '%s'\n", signature_buf);

    fw_cfg_select(FW_CFG_ID);

    uint32_t id = fw_cfg_read_u32();
    LOG_VMM("ID: 0x%x\n", id);
    if (id & FW_CFG_ID_TRADITIONAL) {
        LOG_VMM("traditional fw cfg\n");
    }
    if (id & FW_CFG_ID_DMA) {
        LOG_VMM("fw cfg supports DMA\n");
    }

    fw_cfg_select(FW_CFG_FILE_DIR);

    uint32_t files_count = fw_cfg_read_u32();
    files_count = __builtin_bswap32(files_count);
    LOG_VMM("RAMFB: files_count: %u\n", files_count);

    for (int i = 0; i < files_count; i++) {
        struct FWCfgFile file;
        fw_cfg_read_buf((char *)&file, sizeof(struct FWCfgFile));

        LOG_VMM("RAMFB: file %s, file.select: 0x%x\n", file.name, __builtin_bswap16(file.select));
        if (!memcmp(name, file.name, strlen(FRAMEBFUFER_FWCFG_FILE))) {
            LOG_VMM("RAMFB: found %s, file.select: 0x%x\n", file.name, __builtin_bswap16(file.select));
            file.size = file.size;
            file.select = file.select;
            *out = file;

            return true;
        }
    }

    return false;
}
