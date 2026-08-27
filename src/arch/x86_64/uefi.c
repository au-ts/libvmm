/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <libvmm/guest.h>
#include <libvmm/guest_ram.h>
#include <libvmm/pci.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/uefi.h>
#include <libvmm/arch/x86_64/e820.h>
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/uefi/fw_cfg.h>
#include <libvmm/uefi/table_loader.h>
#include <libvmm/uefi/hardware_info.h>
#include <sddf/util/util.h>

extern guest_t guest;
uint16_t cpu_count = 1; // @billn revisit for SMP

/* Document referenced:
 * https://github.com/tianocore/edk2/blob/edk2-stable202605/OvmfPkg/README */

/* Essential fw_cfg blobs for OVMF in QEMU_FW_CFG_ITEM_FILE_DIR */
/* https://github.com/tianocore/edk2/blob/b03a21a63e3bd001f52c527e5a57feddb53a690b/OvmfPkg/Library/PlatformInitLib/MemDetect.c#L401 */
#define E820_FWCFG_FILENAME "etc/e820"
/* https://github.com/tianocore/edk2/blob/f49f209c4f4c8b817d290f78e785099e8c51589f/OvmfPkg/Library/AcpiPlatformLib/QemuFwCfgAcpi.c#L1121 */
#define TABLE_LOADER_FWCFG_FILENAME "etc/table-loader"
/* These are defined in OVMF but only on loongarch, we will reuse them for the table loader */
#define ACPI_XSDP_FWCFG_FILENAME "etc/acpi/rsdp"
#define ACPI_TABLES_FWCFG_FILENAME "etc/acpi/tables"
/* https://github.com/tianocore/edk2/blob/b03a21a63e3bd001f52c527e5a57feddb53a690b/OvmfPkg/Library/PciHostBridgeUtilityLib/PciHostBridgeUtilityLib.c#L468 */
#define HW_INFO_FWCFG_FILENAME "etc/hardware-info"

/* Backing storage for the E820 table we need to provide to the firmware, static since
 * the fw_cfg code expects the lifetime of the buffer to be equal to the lifetime of the VMM.
 * Since the guest can access the fw cfg at any time. */
struct boot_e820_entry uefi_e820[GUEST_MAX_RAM_REGIONS];

/* Same idea but for ACPI tables */
struct xsdp uefi_xsdp;

#define UEFI_DSDT_MAX_SIZE 0x1000
struct uefi_acpi_tables {
    /* very important for XSDT to be first, as we will point to this blob from "etc/acpi/rsdp" */
    struct xsdt xsdt;
    struct facs facs;
    struct fadt fadt;
    struct hpet hpet;
    struct madt madt;
    char dsdt[UEFI_DSDT_MAX_SIZE];
};
static struct uefi_acpi_tables uefi_acpi_tables;

#define UEFI_MAX_NUM_TABLE_LOADER_CMD 16
static qemu_loader_entry_t uefi_table_loader[UEFI_MAX_NUM_TABLE_LOADER_CMD];

/* Same idea but for Linux cmdline, we want our own copy to be absolutely sure that it is NULL terminated. */
#define COMMAND_LINE_SIZE (2048)
#define COMMAND_LINE_BUF_SIZE (COMMAND_LINE_SIZE + 1)
static char uefi_linux_cmdline[COMMAND_LINE_BUF_SIZE] = { 0 };
static uint32_t uefi_linux_cmdline_size = 0;

/* Same but for the PCI metadata structure. */
struct hw_info_pci_host_bridge uefi_hw_info_pci;

bool uefi_setup_images(uintptr_t firm_src, size_t firm_size, uintptr_t dsdt_src, size_t dsdt_size, uint64_t flash_gpa,
                       size_t flash_size)
{
    if (!firm_size) {
        LOG_VMM_ERR("Firmware size is zero\n");
        return false;
    }

    if (!flash_size) {
        LOG_VMM_ERR("Flash size is zero\n");
        return false;
    }

    if (!dsdt_size) {
        LOG_VMM_ERR("DSDT size is zero\n");
        return false;
    }

    if (!firm_src) {
        LOG_VMM_ERR("Firmware source is NULL\n");
        return false;
    }

    if (!dsdt_src) {
        LOG_VMM_ERR("DSDT source is NULL\n");
        return false;
    }

    if (flash_gpa + flash_size != BIT(32)) {
        LOG_VMM_ERR("Flash [0x%lx..0x%lx), size 0x%lu does not reside at top of 4GB in guest RAM.\n", flash_gpa,
                    flash_gpa + flash_size, flash_size);
        return false;
    }

    if (firm_size > flash_size) {
        LOG_VMM_ERR("Firmware size %lu bytes > flash size %lu bytes\n", firm_size, flash_gpa);
        return false;
    }

    uint64_t mmio_aperature_gpa, mmio_aperature_size;
    if (!pci_bus_get_mmio_aperature(&mmio_aperature_gpa, &mmio_aperature_size)) {
        LOG_VMM_ERR("Virtual PCI bus must be initialised for UEFI\n");
        return false;
    }

    void *dest = gpa_to_hva(flash_gpa + (flash_size - firm_size), firm_size);
    if (!dest) {
        LOG_VMM_ERR(
            "Failed to translate GPA to HVA for firmware copy. Did you add the flash as a guest memory region?\n");
        return false;
    }

    /* Places the firmware at the reset vector, see referenced document for more details. */
    memcpy(dest, (void *)firm_src, firm_size);

    /* Initialise the fw cfg device to give the E820 memory map, ACPI tables and other important
     * things to OVMF. */
    if (!initialise_fw_cfg()) {
        return false;
    }

    /* CPU count, read in PlatformMaxCpuCountInitialization() in PlatformInitLib */
    if (!fw_cfg_add_file(QEMU_FW_CFG_ITEM_SMP_CPU_COUNT, (uint8_t *)&cpu_count, sizeof(uint16_t))) {
        LOG_VMM_ERR("Failed to add CPU count file to fw cfg\n");
        return false;
    }

    /* E820 memory map */
    int num_guest_ram_regions;
    struct guest_ram_region *guest_ram_regions = guest_ram_get_regions(&num_guest_ram_regions);
    assert(num_guest_ram_regions); // impossible, should have been checked earlier

    for (int i = 0; i < num_guest_ram_regions; i++) {
        uefi_e820[i].addr = guest_ram_regions[i].gpa_start;
        uefi_e820[i].size = guest_ram_regions[i].size;
        uefi_e820[i].type = E820_RAM;
    }

    if (!fw_cfg_add_named_file(E820_FWCFG_FILENAME, strlen(E820_FWCFG_FILENAME), (uint8_t *)&uefi_e820,
                               sizeof(struct boot_e820_entry) * num_guest_ram_regions)) {
        LOG_VMM_ERR("Failed to add '%s' file to fw cfg\n", E820_FWCFG_FILENAME);
        return false;
    }

    /* Metadata about our virtual PCI bus */
    uefi_hw_info_pci.header = (struct hw_info_header) {
        .type = HW_INFO_TYPE_HOST_BRIDGE,
        .size = sizeof(struct host_bridge_info),
    };
    uefi_hw_info_pci.body = (struct host_bridge_info) {
        .flags.bits.combine_mem_pmem = 1,
        .flags.bits.dma_above_4g = 1,
        .flags.bits.no_extended_config_space = 1,
        .attributes = EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO | EFI_PCI_ATTRIBUTE_ISA_IO | EFI_PCI_ATTRIBUTE_ISA_IO_16,
        .io_start = 0,
        .io_size = 0,
        .mem_start = mmio_aperature_gpa,
        .mem_size = mmio_aperature_size,
        .pcie_config_start = 0,
        .pcie_config_size = 0,
    };

    if (!fw_cfg_add_named_file(HW_INFO_FWCFG_FILENAME, strlen(HW_INFO_FWCFG_FILENAME), (uint8_t *)&uefi_hw_info_pci,
                               sizeof(struct hw_info_pci_host_bridge))) {
        LOG_VMM_ERR("Failed to add '%s' file to fw cfg\n", HW_INFO_FWCFG_FILENAME);
        return false;
    }

    /* ACPI tables, build them all with dummy checksums and pointers, the firmware will
     * fill those in at runtime. */
    xsdp_build(&uefi_xsdp, 0);
    facs_build(&uefi_acpi_tables.facs);
    fadt_build(&uefi_acpi_tables.fadt, 0, 0);
    hpet_build(&uefi_acpi_tables.hpet);
    madt_build(&uefi_acpi_tables.madt);
    uint64_t dummy_gpas[XSDT_ENTRIES] = { 0 };
    xsdt_build(&uefi_acpi_tables.xsdt, dummy_gpas, XSDT_ENTRIES);

    if (dsdt_size > UEFI_DSDT_MAX_SIZE) {
        LOG_VMM_ERR("DSDT size is greater than max size in fw cfg (%zu > %d), please increase UEFI_DSDT_MAX_SIZE\n",
                    dsdt_size, UEFI_DSDT_MAX_SIZE);
        return false;
    }
    memcpy(&uefi_acpi_tables.dsdt, (void *)dsdt_src, dsdt_size);

    if (!fw_cfg_add_named_file(ACPI_XSDP_FWCFG_FILENAME, strlen(ACPI_XSDP_FWCFG_FILENAME), (uint8_t *)&uefi_xsdp,
                               sizeof(struct xsdp))) {
        LOG_VMM_ERR("Failed to add '%s' file to fw cfg\n", ACPI_XSDP_FWCFG_FILENAME);
        return false;
    }
    if (!fw_cfg_add_named_file(ACPI_TABLES_FWCFG_FILENAME, strlen(ACPI_TABLES_FWCFG_FILENAME),
                               (uint8_t *)&uefi_acpi_tables, sizeof(struct uefi_acpi_tables))) {
        LOG_VMM_ERR("Failed to add '%s' file to fw cfg\n", ACPI_TABLES_FWCFG_FILENAME);
        return false;
    }

    /* Now the important part, we need to tell OVMF how to load the tables, link them together and how to
     * compute the checksum.
     *
     * First step is to actually load the tables into memory from fw cfg. */
    int num_cmd = 0;
    {
        char *file_name = ACPI_XSDP_FWCFG_FILENAME;
        uint32_t alignment = 1;
        bool use_fseg = true;
        assert(table_loader_allocate(&uefi_table_loader[num_cmd], file_name, alignment, use_fseg));
        num_cmd++;
    }
    {
        char *file_name = ACPI_TABLES_FWCFG_FILENAME;
        uint32_t alignment = 1;
        bool use_fseg = false;
        assert(table_loader_allocate(&uefi_table_loader[num_cmd], file_name, alignment, use_fseg));
        num_cmd++;
    }

    /* Then connect the XSDP to the XSDT and compute XSDP checksum. */
    {
        char *dest_file = ACPI_XSDP_FWCFG_FILENAME;
        char *src_file = ACPI_TABLES_FWCFG_FILENAME;
        void *dest_blob = (void *)&uefi_xsdp;
        size_t blob_size = sizeof(struct xsdp);
        uint32_t patch_offset = offsetof(struct xsdp, xsdt_gpa);
        uint8_t patch_size = sizeof(uint64_t);
        uint32_t src_offset = offsetof(struct uefi_acpi_tables, xsdt);
        assert(table_loader_add_pointer(&uefi_table_loader[num_cmd], dest_file, src_file, dest_blob, blob_size,
                                        patch_offset, patch_size, src_offset));
        num_cmd++;
    }
    {
        char *file_name = ACPI_XSDP_FWCFG_FILENAME;
        void *blob = (void *)&uefi_xsdp;
        size_t blob_size = sizeof(struct xsdp);
        uint32_t start_offset = 0;
        uint32_t length = offsetof(struct xsdp, length); /* Checksum up only the legacy part */
        uint32_t checksum_offset = offsetof(struct xsdp, checksum);
        assert(table_loader_add_checksum(&uefi_table_loader[num_cmd], file_name, blob, blob_size, start_offset, length,
                                         checksum_offset));
        num_cmd++;
    }
    {
        char *file_name = ACPI_XSDP_FWCFG_FILENAME;
        void *blob = (void *)&uefi_xsdp;
        size_t blob_size = sizeof(struct xsdp);
        uint32_t start_offset = 0;
        uint32_t length = sizeof(struct xsdp);
        uint32_t checksum_offset = offsetof(struct xsdp, ext_checksum);
        assert(table_loader_add_checksum(&uefi_table_loader[num_cmd], file_name, blob, blob_size, start_offset, length,
                                         checksum_offset));
        num_cmd++;
    }

    /* Connect the DSDT and FACS to FADT, then checksum the FADT */
    {
        char *dest_file = ACPI_TABLES_FWCFG_FILENAME;
        char *src_file = ACPI_TABLES_FWCFG_FILENAME;
        void *dest_blob = (void *)&uefi_acpi_tables;
        size_t blob_size = sizeof(struct uefi_acpi_tables);
        uint32_t patch_offset = offsetof(struct uefi_acpi_tables, fadt.X_FirmwareControl);
        uint8_t patch_size = sizeof(uint64_t);
        uint32_t src_offset = offsetof(struct uefi_acpi_tables, facs);
        assert(table_loader_add_pointer(&uefi_table_loader[num_cmd], dest_file, src_file, dest_blob, blob_size,
                                        patch_offset, patch_size, src_offset));
        num_cmd++;
    }
    {
        char *dest_file = ACPI_TABLES_FWCFG_FILENAME;
        char *src_file = ACPI_TABLES_FWCFG_FILENAME;
        void *dest_blob = (void *)&uefi_acpi_tables;
        size_t blob_size = sizeof(struct uefi_acpi_tables);
        uint32_t patch_offset = offsetof(struct uefi_acpi_tables, fadt.X_Dsdt);
        uint8_t patch_size = sizeof(uint64_t);
        uint32_t src_offset = offsetof(struct uefi_acpi_tables, dsdt);
        assert(table_loader_add_pointer(&uefi_table_loader[num_cmd], dest_file, src_file, dest_blob, blob_size,
                                        patch_offset, patch_size, src_offset));
        num_cmd++;
    }
    {
        char *file_name = ACPI_TABLES_FWCFG_FILENAME;
        void *blob = (void *)&uefi_acpi_tables;
        size_t blob_size = sizeof(struct uefi_acpi_tables);
        uint32_t start_offset = offsetof(struct uefi_acpi_tables, fadt);
        uint32_t length = sizeof(struct fadt);
        uint32_t checksum_offset = offsetof(struct uefi_acpi_tables, fadt.h.checksum);
        assert(table_loader_add_checksum(&uefi_table_loader[num_cmd], file_name, blob, blob_size, start_offset, length,
                                         checksum_offset));
        num_cmd++;
    }

    /* Connect the FADT, HPET and MADT to XSDT, then checksum the XSDT */
    {
        char *dest_file = ACPI_TABLES_FWCFG_FILENAME;
        char *src_file = ACPI_TABLES_FWCFG_FILENAME;
        void *dest_blob = (void *)&uefi_acpi_tables;
        size_t blob_size = sizeof(struct uefi_acpi_tables);
        uint32_t patch_offset = offsetof(struct uefi_acpi_tables, xsdt.tables[0]);
        uint8_t patch_size = sizeof(uint64_t);
        uint32_t src_offset = offsetof(struct uefi_acpi_tables, fadt);
        assert(table_loader_add_pointer(&uefi_table_loader[num_cmd], dest_file, src_file, dest_blob, blob_size,
                                        patch_offset, patch_size, src_offset));
        num_cmd++;
    }
    {
        char *dest_file = ACPI_TABLES_FWCFG_FILENAME;
        char *src_file = ACPI_TABLES_FWCFG_FILENAME;
        void *dest_blob = (void *)&uefi_acpi_tables;
        size_t blob_size = sizeof(struct uefi_acpi_tables);
        uint32_t patch_offset = offsetof(struct uefi_acpi_tables, xsdt.tables[1]);
        uint8_t patch_size = sizeof(uint64_t);
        uint32_t src_offset = offsetof(struct uefi_acpi_tables, hpet);
        assert(table_loader_add_pointer(&uefi_table_loader[num_cmd], dest_file, src_file, dest_blob, blob_size,
                                        patch_offset, patch_size, src_offset));
        num_cmd++;
    }
    {
        char *dest_file = ACPI_TABLES_FWCFG_FILENAME;
        char *src_file = ACPI_TABLES_FWCFG_FILENAME;
        void *dest_blob = (void *)&uefi_acpi_tables;
        size_t blob_size = sizeof(struct uefi_acpi_tables);
        uint32_t patch_offset = offsetof(struct uefi_acpi_tables, xsdt.tables[2]);
        uint8_t patch_size = sizeof(uint64_t);
        uint32_t src_offset = offsetof(struct uefi_acpi_tables, madt);
        assert(table_loader_add_pointer(&uefi_table_loader[num_cmd], dest_file, src_file, dest_blob, blob_size,
                                        patch_offset, patch_size, src_offset));
        num_cmd++;
    }
    {
        char *file_name = ACPI_TABLES_FWCFG_FILENAME;
        void *blob = (void *)&uefi_acpi_tables;
        size_t blob_size = sizeof(struct uefi_acpi_tables);
        uint32_t start_offset = offsetof(struct uefi_acpi_tables, xsdt);
        uint32_t length = sizeof(struct xsdt);
        uint32_t checksum_offset = offsetof(struct uefi_acpi_tables, xsdt.h.checksum);
        assert(table_loader_add_checksum(&uefi_table_loader[num_cmd], file_name, blob, blob_size, start_offset, length,
                                         checksum_offset));
        num_cmd++;
    }

    assert(num_cmd < UEFI_MAX_NUM_TABLE_LOADER_CMD);

    if (!fw_cfg_add_named_file(TABLE_LOADER_FWCFG_FILENAME, strlen(TABLE_LOADER_FWCFG_FILENAME),
                               (uint8_t *)&uefi_table_loader, sizeof(qemu_loader_entry_t) * num_cmd)) {
        LOG_VMM_ERR("Failed to add '%s' file to fw cfg\n", TABLE_LOADER_FWCFG_FILENAME);
        return false;
    }

    return true;
}

bool uefi_add_linux_boot(uintptr_t kernel_src, size_t kernel_size, uintptr_t initrd_src, size_t initrd_size,
                         char *cmdline)
{
    /* See https://github.com/tianocore/edk2/blob/edk2-stable202605/OvmfPkg/QemuKernelLoaderFsDxe/QemuKernelLoaderFsDxe.c
     * for more details on the operations in this function. */

    if (!kernel_src) {
        LOG_VMM_ERR("Kernel source is NULL\n");
        return false;
    }

    if (!kernel_size) {
        LOG_VMM_ERR("Kernel size is zero\n");
        return false;
    }

    if (!initrd_src) {
        LOG_VMM_ERR("Initrd source is NULL\n");
        return false;
    }

    if (!initrd_size) {
        LOG_VMM_ERR("Initrd size is zero\n");
        return false;
    }

    if (!cmdline) {
        LOG_VMM_ERR("Cmdline source is NULL\n");
        return false;
    }

    size_t cmdline_size = strlen(cmdline);
    if (cmdline_size > COMMAND_LINE_SIZE) {
        LOG_VMM_ERR("Cmdline too long\n");
        return false;
    }
    memcpy(uefi_linux_cmdline, cmdline, cmdline_size);
    uefi_linux_cmdline_size = cmdline_size + 1;

    char *kernel_file_name = "etc/boot/kernel";
    bool success = fw_cfg_add_named_file(kernel_file_name, strlen(kernel_file_name), (void *)kernel_src, kernel_size);
    if (!success) {
        LOG_VMM_ERR("Failed to add kernel file to fw cfg\n");
        return false;
    }

    char *initrd_file_name = "etc/boot/initrd";
    success = fw_cfg_add_named_file(initrd_file_name, strlen(initrd_file_name), (void *)initrd_src, initrd_size);
    if (!success) {
        LOG_VMM_ERR("Failed to add initrd file to fw cfg\n");
        return false;
    }

    char *cmdline_file_name = "etc/boot/cmdline";
    success = fw_cfg_add_named_file(cmdline_file_name, strlen(cmdline_file_name), (void *)uefi_linux_cmdline,
                                    uefi_linux_cmdline_size);
    if (!success) {
        LOG_VMM_ERR("Failed to add cmdline file to fw cfg\n");
        return false;
    }

    /* OVMF quirk: the cmdline is loaded via legacy keyed fw cfg file rather than the named file. */
    success = fw_cfg_add_file(QEMU_FW_CFG_ITEM_COMMAND_LINE_DATA, (void *)uefi_linux_cmdline, uefi_linux_cmdline_size);
    if (!success) {
        LOG_VMM_ERR("Failed to add legacy cmdline file to fw cfg\n");
        return false;
    }

    success = fw_cfg_add_file(QEMU_FW_CFG_ITEM_COMMAND_LINE_SIZE, (void *)&uefi_linux_cmdline_size, sizeof(uint32_t));
    if (!success) {
        LOG_VMM_ERR("Failed to add legacy cmdline file to fw cfg\n");
        return false;
    }

    return true;
}