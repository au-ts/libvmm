/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <microkit.h>
#include <libvmm/uefi/fw_cfg.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

/*
 * Implements the QEMU Firmware Configuration (fw_cfg) Device
 * As described in https://github.com/qemu/qemu/blob/master/docs/specs/fw_cfg.rst
 */

/* Uncomment this to enable debug logging */
#define DEBUG_FWCFG

#if defined(DEBUG_FWCFG)
#define LOG_FWCFG(...) do{ printf("%s|FWCFG: ", microkit_name); printf(__VA_ARGS__); } while(0)
#else
#define LOG_FWCFG(...) do{}while(0)
#endif

static char fw_cfg_sig[] = "QEMU";

#define FW_CFG_ID_TRADITIONAL BIT(0)
static uint32_t fw_cfg_id = FW_CFG_ID_TRADITIONAL;

/* "File Directory (Key 0x0019, FW_CFG_FILE_DIR)
 * Firmware configuration items stored at selector keys 0x0020 or higher
 * (FW_CFG_FILE_FIRST or higher) have an associated entry in a directory
 * structure, which makes it easier for guest-side firmware to identify
 * and retrieve them."" */

struct fw_cfg_file {        /* an individual file entry, 64 bytes total */
    uint32_t size_be;       /* size of referenced fw_cfg item, big-endian */
    uint16_t select_be;     /* selector key of fw_cfg item, big-endian */
    uint16_t reserved;
    char name[QEMU_FW_CFG_FNAME_SIZE]; /* fw_cfg item name, NUL-terminated ascii */
} __attribute__((packed));

#define MAX_FW_CFG_NAMED_FILES 4 /* Can be increased safely if necessary. */

struct fw_cfg_files {       /* the entire file directory fw_cfg item */
    uint32_t count_be;      /* number of entries, in big-endian format */
    struct fw_cfg_file f[MAX_FW_CFG_NAMED_FILES]; /* array of file entries */
} __attribute__((packed));

#define MAX_SELECT_KEY (QEMU_FW_CFG_ITEM_FILE_DIR + MAX_FW_CFG_NAMED_FILES) /* Inclusive */

struct fw_cfg_file_bookkeep {
    uint8_t *data;
    size_t size;
};

struct fw_cfg_state {
    bool initialised;

    /* State of the data pointer */
    uint16_t selected;
    uint8_t *selected_data;
    size_t selected_index;
    size_t selected_size;

    /* Data for QEMU_FW_CFG_ITEM_FILE_DIR */
    size_t num_named_files;
    struct fw_cfg_files named_files;

    /* Book keeping for both unnamed and named files. */
    struct fw_cfg_file_bookkeep bookkeeped_files[MAX_SELECT_KEY];
};

static struct fw_cfg_state fw_cfg_state;

/* Essential fw_cfg blobs for OVMF in QEMU_FW_CFG_ITEM_FILE_DIR */
/* https://github.com/tianocore/edk2/blob/b03a21a63e3bd001f52c527e5a57feddb53a690b/OvmfPkg/Library/PlatformInitLib/MemDetect.c#L401 */
#define E820_FWCFG_FILENAME "etc/e820"
/* https://github.com/tianocore/edk2/blob/f49f209c4f4c8b817d290f78e785099e8c51589f/OvmfPkg/Library/AcpiPlatformLib/QemuFwCfgAcpi.c#L1121 */
#define TABLE_LOADER_FWCFG_FILENAME "etc/table-loader"

#if defined(CONFIG_ARCH_X86)
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/fault.h>

#define QEMU_FW_CFG_X86_SELECT_PORT 0x510
#define QEMU_FW_CFG_X86_DATA_PORT 0x511

static bool qemu_fw_cfg_select_handler(size_t vcpu_id, uint16_t port_offset, size_t qualification,
                                       seL4_VCPUContext *vctx, void *cookie)
{
    if (pio_fault_is_read(qualification)) {
        pio_emulate_read(qualification, vctx, fw_cfg_state.selected);
    } else {
        assert(!pio_fault_is_string_op(qualification));

        fw_cfg_state.selected = pio_get_write_data(qualification, vctx);
        fw_cfg_state.selected_data = NULL;
        fw_cfg_state.selected_index = 0;
        fw_cfg_state.selected_size = 0;

        if (fw_cfg_state.selected <= MAX_SELECT_KEY) {
            fw_cfg_state.selected_data = fw_cfg_state.bookkeeped_files[fw_cfg_state.selected].data;
            fw_cfg_state.selected_size = fw_cfg_state.bookkeeped_files[fw_cfg_state.selected].size;
            LOG_FWCFG("selector key 0x%x valid\n", fw_cfg_state.selected);
        } else {
            LOG_FWCFG("selector key 0x%x NOT valid\n", fw_cfg_state.selected);
        }
    }
    return true;
}

static bool qemu_fw_cfg_data_handler(size_t vcpu_id, uint16_t port_offset, size_t qualification, seL4_VCPUContext *vctx,
                                     void *cookie)
{
    if (pio_fault_is_read(qualification)) {
        if (pio_fault_is_string_op(qualification)) {
            if (fw_cfg_state.selected_size && fw_cfg_state.selected_index < fw_cfg_state.selected_size) {
                uint8_t *data_ptr = fw_cfg_state.selected_data + fw_cfg_state.selected_index;
                fw_cfg_state.selected_index += pio_emulate_string_read(
                    qualification, vctx, data_ptr, fw_cfg_state.selected_size - fw_cfg_state.selected_index);
            }
        } else {
            assert(pio_fault_to_access_width_bytes(qualification) == 1);
            uint32_t data = 0;
            if (fw_cfg_state.selected_size && fw_cfg_state.selected_index < fw_cfg_state.selected_size) {
                uint8_t *data_ptr = fw_cfg_state.selected_data + fw_cfg_state.selected_index;
                data = *data_ptr;
                fw_cfg_state.selected_index++;
            }
            pio_emulate_read(qualification, vctx, data);
        }
    }
    return true;
}

static bool arch_init_fw_cfg(void)
{
    bool success = fault_register_pio_exception_handler(QEMU_FW_CFG_X86_SELECT_PORT, 1, &qemu_fw_cfg_select_handler,
                                                        NULL);
    if (!success) {
        LOG_VMM_ERR("Failed to register PIO handler for fw cfg select port\n");
        return false;
    }
    success = fault_register_pio_exception_handler(QEMU_FW_CFG_X86_DATA_PORT, 1, &qemu_fw_cfg_data_handler, NULL);
    if (!success) {
        LOG_VMM_ERR("Failed to register PIO handler for fw cfg data port\n");
        return false;
    }

    return success;
}

#elif defined(CONFIG_ARCH_ARM)
static bool arch_init_fw_cfg(void)
{
    return false;
}
#endif

bool add_fw_cfg_named_file(char *name, size_t name_len, uint8_t *data, size_t data_size)
{
    if (!fw_cfg_state.initialised) {
        LOG_VMM_ERR("fw cfg is uninitialised\n");
        return false;
    }

    if (!name) {
        LOG_VMM_ERR("name is NULL\n");
        return false;
    }

    if (!name_len || name_len >= QEMU_FW_CFG_FNAME_SIZE) {
        LOG_VMM_ERR("name length %lu is not valid\n", name_len);
        return false;
    }

    if (!data) {
        LOG_VMM_ERR("data is NULL\n");
        return false;
    }

    if (!data_size) {
        LOG_VMM_ERR("data_size is 0\n");
        return false;
    }

    if (fw_cfg_state.num_named_files >= MAX_FW_CFG_NAMED_FILES) {
        LOG_VMM_ERR("out of space for named fw cfg file\n");
        return false;
    }

    uint16_t select_key = QEMU_FW_CFG_FILE_FIRST + fw_cfg_state.num_named_files;

    fw_cfg_state.bookkeeped_files[select_key].data = data;
    fw_cfg_state.bookkeeped_files[select_key].size = data_size;

    fw_cfg_state.named_files.f[fw_cfg_state.num_named_files].size_be = __builtin_bswap32(data_size);
    fw_cfg_state.named_files.f[fw_cfg_state.num_named_files].select_be = __builtin_bswap16(select_key);

    memcpy(fw_cfg_state.named_files.f[fw_cfg_state.num_named_files].name, name, name_len);
    /* No need to nul terminate as we already zero init the buffer */

    fw_cfg_state.num_named_files++;
    fw_cfg_state.named_files.count_be = __builtin_bswap32(fw_cfg_state.num_named_files);

    return true;
}

bool add_fw_cfg_file(uint16_t select_key, uint8_t *data, size_t data_size)
{
    if (!fw_cfg_state.initialised) {
        LOG_VMM_ERR("Failed to add named file because fw cfg is uninitialised\n");
        return false;
    }

    if (!data) {
        LOG_VMM_ERR("data is NULL\n");
        return false;
    }

    if (!data_size) {
        LOG_VMM_ERR("data_size is 0\n");
        return false;
    }

    if (select_key >= QEMU_FW_CFG_ITEM_FILE_DIR) {
        LOG_VMM_ERR("select key 0x%x must be less than 0x%x\n", select_key, QEMU_FW_CFG_ITEM_FILE_DIR);
        return false;
    }

    if (fw_cfg_state.bookkeeped_files[select_key].size) {
        LOG_VMM_ERR("select key 0x%x is already used for data item size 0x%lx\n", select_key,
                    fw_cfg_state.bookkeeped_files[select_key].size);
        return false;
    }

    fw_cfg_state.bookkeeped_files[select_key].data = data;
    fw_cfg_state.bookkeeped_files[select_key].size = data_size;

    return true;
}

bool initialise_fw_cfg(void)
{
    memset(&fw_cfg_state, 0, sizeof(struct fw_cfg_state));
    bool success = arch_init_fw_cfg();
    if (!success) {
        LOG_VMM_ERR("failed to initialise fw cfg\n");
        return false;
    }
    fw_cfg_state.initialised = true;

    success = add_fw_cfg_file(QEMU_FW_CFG_ITEM_SIGNATURE, (uint8_t *)fw_cfg_sig, strlen(fw_cfg_sig));
    if (!success) {
        LOG_VMM_ERR("failed to initialise fw cfg\n");
        fw_cfg_state.initialised = false;
        return false;
    }

    success = add_fw_cfg_file(QEMU_FW_CFG_ITEM_INTERFACE_VERSION, (uint8_t *)&fw_cfg_id, sizeof(uint32_t));
    if (!success) {
        LOG_VMM_ERR("failed to initialise fw cfg\n");
        fw_cfg_state.initialised = false;
        return false;
    }

    return success;
}