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

/*
 * Implements the QEMU Firmware Configuration (fw_cfg) Device
 * As described in https://github.com/qemu/qemu/blob/master/docs/specs/fw_cfg.rst
 */

#define FW_CFG_SIGNATURE_STR "QEMU"
#define FW_CFG_ID_TRADITIONAL BIT(0)

/* "File Directory (Key 0x0019, FW_CFG_FILE_DIR)
 * Firmware configuration items stored at selector keys 0x0020 or higher
 * (FW_CFG_FILE_FIRST or higher) have an associated entry in a directory
 * structure, which makes it easier for guest-side firmware to identify
 * and retrieve them."" */

struct fw_cfg_file {        /* an individual file entry, 64 bytes total */
    uint32_t size;          /* size of referenced fw_cfg item, big-endian */
    uint16_t select;        /* selector key of fw_cfg item, big-endian */
    uint16_t reserved;
    char name[QEMU_FW_CFG_FNAME_SIZE]; /* fw_cfg item name, NUL-terminated ascii */
} __attribute__((packed));

struct fw_cfg_files {       /* the entire file directory fw_cfg item */
    uint32_t count;         /* number of entries, in big-endian format */
    struct fw_cfg_file f[]; /* array of file entries */
};

#define MAX_FW_CFG_NAMED_FILES 4 /* Can be increased safely if necessary. */

struct fw_cfg_state {
    bool initialised;

    /* State of the data pointer */
    uint16_t selected;
    uint8_t *data;
    size_t index;
    size_t size;

    size_t num_named_files;
    struct fw_cfg_file named_files[MAX_FW_CFG_NAMED_FILES];
};

static struct fw_cfg_state fw_cfg_state;

/* Essential fw_cfg blobs for OVMF in QEMU_FW_CFG_ITEM_FILE_DIR */
/* https://github.com/tianocore/edk2/blob/b03a21a63e3bd001f52c527e5a57feddb53a690b/OvmfPkg/Library/PlatformInitLib/MemDetect.c#L401 */
#define E820_FWCFG_FILENAME "etc/e820"
/* https://github.com/tianocore/edk2/blob/f49f209c4f4c8b817d290f78e785099e8c51589f/OvmfPkg/Library/AcpiPlatformLib/QemuFwCfgAcpi.c#L1121 */
#define TABLE_LOADER_FWCFG_FILENAME "etc/table-loader"

#if defined(CONFIG_ARCH_X86)
#define QEMU_FW_CFG_X86_SELECT_PORT 0x510
#define QEMU_FW_CFG_X86_DATA_PORT 0x511

static bool qemu_fw_cfg_access_handler(size_t vcpu_id, uint16_t port_offset, size_t qualification,
                                       seL4_VCPUContext *vctx, void *cookie)
{

    return true;
}

static bool arch_init_fw_cfg(void)
{
    return true;
}

#elif defined(CONFIG_ARCH_ARM)
static bool arch_init_fw_cfg(void)
{
    return false;
}
#endif

bool initialise_fw_cfg(void)
{
    memset(&fw_cfg_state, 0, sizeof(struct fw_cfg_state));
    return arch_init_fw_cfg();
}