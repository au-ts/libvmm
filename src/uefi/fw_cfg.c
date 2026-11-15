/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <libvmm/guest_ram.h>
#include <microkit.h>
#include <libvmm/uefi/fw_cfg.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

/*
 * Implements the QEMU Firmware Configuration (fw_cfg) Device
 * As described in https://github.com/qemu/qemu/blob/master/docs/specs/fw_cfg.rst
 */

static char fw_cfg_sig[] = "QEMU";

#define FW_CFG_DMA_MAGIC_LOW    __builtin_bswap32(0x51454d55)
#define FW_CFG_DMA_MAGIC_HIGH   __builtin_bswap32(0x20434647)

#define FW_CFG_ID_TRADITIONAL BIT(0)
#define FW_CFG_ID_DMA         BIT(1)
static uint32_t fw_cfg_id = FW_CFG_ID_TRADITIONAL | FW_CFG_ID_DMA;

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

#define MAX_FW_CFG_NAMED_FILES 8 /* Can be increased safely if necessary. */

struct fw_cfg_files {       /* the entire file directory fw_cfg item */
    uint32_t count_be;      /* number of entries, in big-endian format */
    struct fw_cfg_file f[MAX_FW_CFG_NAMED_FILES]; /* array of file entries */
} __attribute__((packed));

#define FW_CFG_CONTROL_ERROR    BIT(0)
#define FW_CFG_CONTROL_READ     BIT(1)
#define FW_CFG_CONTROL_SKIP     BIT(2)
#define FW_CFG_CONTROL_SELECT   BIT(3)
#define FW_CFG_CONTROL_WRITE    BIT(4)

struct fw_cfg_dma {
    uint32_t control;
    uint32_t length;
    uint64_t address;
};

#define MAX_SELECT_KEY (QEMU_FW_CFG_FILE_FIRST + MAX_FW_CFG_NAMED_FILES) /* Exclusive */

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

    uint64_t dma_address;

    /* Data for QEMU_FW_CFG_ITEM_FILE_DIR */
    size_t num_named_files;
    struct fw_cfg_files named_files;

    /* Book keeping for both unnamed and named files. */
    struct fw_cfg_file_bookkeep bookkeeped_files[MAX_SELECT_KEY];
};

static struct fw_cfg_state fw_cfg_state;

#if defined(CONFIG_ARCH_X86)
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/fault.h>

#define QEMU_FW_CFG_X86_SELECT_PORT 0x510
#define QEMU_FW_CFG_X86_DATA_PORT 0x511
#define QEMU_FW_CFG_X86_DMA_PORT 0x514

static void qemu_fw_cfg_select_item(uint32_t sel)
{
    fw_cfg_state.selected = sel;
    fw_cfg_state.selected_data = NULL;
    fw_cfg_state.selected_index = 0;
    fw_cfg_state.selected_size = 0;

    if (fw_cfg_state.selected < MAX_SELECT_KEY) {
        fw_cfg_state.selected_data = fw_cfg_state.bookkeeped_files[fw_cfg_state.selected].data;
        fw_cfg_state.selected_size = fw_cfg_state.bookkeeped_files[fw_cfg_state.selected].size;
    }
}

static void qemu_fw_cfg_read_content(void *dst, size_t size)
{
    /* QEMU allows to read OOB and non-present items. In such case DMA unfilled buffer is just
     * zeroed. See fw_cfg_data_read() in qemu sources. */
    if (fw_cfg_state.selected_size) {
        size_t available = fw_cfg_state.selected_size - fw_cfg_state.selected_index;
        size_t can_read = MIN(available, size);
        uint8_t *data_ptr = fw_cfg_state.selected_data + fw_cfg_state.selected_index;

        memcpy(dst, data_ptr, can_read);
        memset(dst + can_read, 0, size - can_read);
        fw_cfg_state.selected_index += can_read;
    } else {
        memset(dst, 0, size);
    }
}

static bool qemu_fw_cfg_select_handler(size_t vcpu_id, uint16_t port_offset, size_t qualification,
                                       seL4_VCPUContext *vctx, void *cookie)
{
    if (pio_fault_is_read(qualification)) {
        if (!pio_emulate_read(qualification, vctx, fw_cfg_state.selected)) {
            LOG_VMM_ERR("failed to set read operand\n");
            return false;
        }
    } else {
        if (pio_fault_is_string_op(qualification)) {
            LOG_VMM_ERR("cannot use string op on fw cfg select port\n");
            return false;
        }

        uint32_t sel;
        if (!pio_get_write_data(qualification, vctx, &sel)) {
            LOG_VMM_ERR("failed to get write operand\n");
            return false;
        }

        qemu_fw_cfg_select_item(sel);
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
            if (pio_fault_to_access_width_bytes(qualification) != 1) {
                LOG_VMM_ERR("access width must be one byte\n");
                return false;
            }
            uint32_t data = 0;

            qemu_fw_cfg_read_content(&data, 1);

            if (!pio_emulate_read(qualification, vctx, data)) {
                LOG_VMM_ERR("failed to set read operand\n");
                return false;
            }
        }
    }
    return true;
}

static void qemu_fw_cfg_do_dma(struct fw_cfg_dma *dma)
{
    uint32_t control = __builtin_bswap32(dma->control);
    uint32_t length = __builtin_bswap32(dma->length);
    uint64_t address = __builtin_bswap64(dma->address);
    bool success = true;

    if (control & FW_CFG_CONTROL_SELECT) {
        qemu_fw_cfg_select_item(control >> 16);
    }

    if (control & FW_CFG_CONTROL_READ) {
        void *buffer = gpa_to_hva(address, length);

        if (buffer) {
            qemu_fw_cfg_read_content(buffer, length);
        } else {
            success = false;
        }
    } else if (control & FW_CFG_CONTROL_WRITE) {
        LOG_VMM_ERR("Unsupported write request\n");
        success = false;
    } else if (control & FW_CFG_CONTROL_SKIP) {
        fw_cfg_state.selected_index = MIN(fw_cfg_state.selected_size, fw_cfg_state.selected_index + length);
    }

    dma->control = __builtin_bswap32(success ? 0 : FW_CFG_CONTROL_ERROR);
}

static bool qemu_fw_cfg_dma_handler(size_t vcpu_id, uint16_t port_offset, size_t qualification, seL4_VCPUContext *vctx,
                                    void *cookie)
{
    if (pio_fault_to_access_width_bytes(qualification) != 4) {
        LOG_VMM_ERR("Invalid DMA port access size\n");
        return false;
    }

    if (pio_fault_is_string_op(qualification)) {
        LOG_VMM_ERR("String ops are not supported on DMA port\n");
        return false;
    }

    if (pio_fault_is_read(qualification)) {
        /* See QEMU spec. It's 'QEMU CFG' in big-endian format. */
        switch (port_offset) {
        case 0:
            if (!pio_emulate_read(qualification, vctx, FW_CFG_DMA_MAGIC_LOW)) {
                LOG_VMM_ERR("failed to set read operand\n");
                return false;
            }
            break;
        case 4:
            if (!pio_emulate_read(qualification, vctx, FW_CFG_DMA_MAGIC_HIGH)) {
                LOG_VMM_ERR("failed to set read operand\n");
                return false;
            }
            break;
        default:
            LOG_VMM_ERR("Invalid DMA port access\n");
            return false;
        }

        return true;
    } else {
        uint32_t data;
        uint64_t gpa;
        struct fw_cfg_dma dma;
        void *guest_dma_req;

        pio_get_write_data(qualification, vctx, &data);

        switch (port_offset) {
        case 0:
            fw_cfg_state.dma_address &= 0xffffffff;
            fw_cfg_state.dma_address |= (uint64_t)__builtin_bswap32(data) << 32;
            return true;
        case 4:
            gpa = fw_cfg_state.dma_address | __builtin_bswap32(data);

            /* Reset before each transfer, so a guest that only
             * writes the low half gets a zero high half. */
            fw_cfg_state.dma_address = 0;
            break;
        default:
            LOG_VMM_ERR("Invalid DMA port access\n");
            return false;
        }

        guest_dma_req = gpa_to_hva(gpa, sizeof(dma));
        if (!guest_dma_req) {
            LOG_VMM_ERR("Invalid DMA gpa\n");
            return false;
        }

        memcpy(&dma, guest_dma_req, sizeof(dma));
        qemu_fw_cfg_do_dma(&dma);
        memcpy((uint8_t *)guest_dma_req + offsetof(struct fw_cfg_dma, control), &dma.control, sizeof(dma.control));

        return true;
    }
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
    success = fault_register_pio_exception_handler(QEMU_FW_CFG_X86_DMA_PORT, 8, &qemu_fw_cfg_dma_handler, NULL);
    if (!success) {
        LOG_VMM_ERR("Failed to register PIO handler for fw cfg dma port\n");
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

bool fw_cfg_add_named_file(char *name, size_t name_len, uint8_t *data, size_t data_size)
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

bool fw_cfg_add_file(uint16_t select_key, uint8_t *data, size_t data_size)
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

    if (select_key > QEMU_FW_CFG_ITEM_FILE_DIR) {
        LOG_VMM_ERR("select key 0x%x must be less than or equal to 0x%x\n", select_key, QEMU_FW_CFG_ITEM_FILE_DIR);
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

    success = fw_cfg_add_file(QEMU_FW_CFG_ITEM_SIGNATURE, (uint8_t *)fw_cfg_sig, strlen(fw_cfg_sig));
    if (!success) {
        LOG_VMM_ERR("failed to initialise fw cfg\n");
        fw_cfg_state.initialised = false;
        return false;
    }

    success = fw_cfg_add_file(QEMU_FW_CFG_ITEM_INTERFACE_VERSION, (uint8_t *)&fw_cfg_id, sizeof(uint32_t));
    if (!success) {
        LOG_VMM_ERR("failed to initialise fw cfg\n");
        fw_cfg_state.initialised = false;
        return false;
    }

    success = fw_cfg_add_file(QEMU_FW_CFG_ITEM_FILE_DIR, (uint8_t *)&fw_cfg_state.named_files,
                              sizeof(struct fw_cfg_files));
    if (!success) {
        LOG_VMM_ERR("failed to initialise fw cfg\n");
        fw_cfg_state.initialised = false;
        return false;
    }

    return success;
}
