/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>

/* Documents referenced:
 * [1] Advanced Configuration and Power Interface (ACPI) Specification Release 6.5 UEFI Forum, Inc. Aug 29, 2022
 *     [1a] "5.2.5.3 Root System Description Pointer (RSDP) Structure"
 *     [1b] "5.2.6 System Description Table Header"
 *     [1c] "5.2.8 Extended System Description Table (XSDT)"
 * [2] IA-PC HPET (High Precision Event Timers) Specification Revision: 1.0a Date: October 2004
 *     https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf
 *     [2a] "3.2.4 The ACPI 2.0 HPET Description Table (HPET)"
 */

/* XSDP (Root System Description Pointer) -> XSDT (Extended System Description Table)
 *                                                               |
 *                                                               v
 *                                                    other ACPI tables
 */

/* [1a] */
#define XSDP_SIGNATURE "RSD PTR "
#define XSDP_SIG_LEN 8
#define XSDP_OEM_ID_LEN 6
#define XSDP_REVISION 2
#define XSDP_ALIGN 16
struct xsdp {
    char signature[XSDP_SIG_LEN];
    uint8_t checksum;
    char oem_id[XSDP_OEM_ID_LEN];
    uint8_t revision;
    uint32_t _deprecated;

    uint32_t length;
    uint64_t xsdt_gpa;
    uint8_t ext_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

////////////////////////////////////////

/* [1b] All system description tables begin with the following structure */
#define DST_HDR_SIG_LEN 4
#define DST_HDR_OEM_ID_LEN 6
#define DST_HDR_OEM_TABLE_ID_LEN 8
struct dst_header {
    char signature[DST_HDR_SIG_LEN];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[DST_HDR_OEM_ID_LEN];
    char oem_table_id[DST_HDR_OEM_TABLE_ID_LEN];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

////////////////////////////////////////

/* [1c] Root System Description Table */
#define XSDT_SIGNATURE "XSDT"
#define XSDT_ENTRIES 3
// #define XSDT_MAX_ENTRIES 16 // arbitrary, can be increased easily by editing this number
struct xsdt {
    struct dst_header h;
    /* A list of guest physical addresses to other ACPI tables. */
    uint64_t tables[XSDT_ENTRIES];
} __attribute__((packed));

////////////////////////////////////////

#define MADT_ENTRY_TYPE_LAPIC 0x0
#define MADT_ENTRY_TYPE_IOAPIC 0x1

#define MADT_ENTRY_LAPIC_LENGTH 0x8
#define MADT_ENTRY_IOAPIC_LENGTH 0xc

// TODO: do not hard-code this address
#define IOAPIC_ADDRESS 0xFEC00000

struct madt_irq_controller {
    uint8_t type;
    /* Note that the length includes this MADT IRQ controller header. */
    uint8_t length;
} __attribute__((packed));

#define MADT_LAPIC_FLAGS (1 << 0) | (1 << 1)

struct madt_lapic {
    struct madt_irq_controller entry;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

struct madt_ioapic {
    struct madt_irq_controller entry;
    uint8_t id;
    /* Reserved */
    uint8_t res;
    uint32_t address;
    uint32_t global_system_irq_base;
} __attribute__((packed));

struct madt_ioapic_source_override {
    struct madt_irq_controller entry;
    uint8_t bus;
    uint8_t source;
    uint32_t gsi;
    uint16_t flags;
} __attribute__((packed));

// @billn dedup
// This just matches the x86 CPU default physical address from
// See bit 9 of 'Table 1-20. More on Feature Information Returned in the EDX Register'.
#define MADT_LOCAL_APIC_ADDR 0xFEE00000

#define MADT_REVISION 5
#define MADT_FLAGS BIT(0) // PCAT_COMPAT, dual 8259 present as a direct mapping to I/O APIC 0

struct madt {
    struct dst_header h;
    uint32_t apic_addr;
    uint32_t flags;
} __attribute__((packed));

////////////////////////////////////////

/* [2a] HPET Description Table */
#define HPET_SIGNATURE "HPET"
#define HPET_REVISION 0x1

#define HPET_LEGACY_IRQ_CAPABLE BIT(15)
#define HPET_NUM_COMPARATORS_VALUE 3
#define HPET_NUM_COMPARATORS_SHIFT 8

#define HPET_GPA 0xfed00000

struct address_structure {
    uint8_t address_space_id;    // 0 - MMIO, 1 - I/O port, 2 - PCI config space
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
} __attribute__((packed));

struct hpet {
    struct dst_header h;
    uint32_t evt_timer_block_id;
    struct address_structure address_desc;
    uint8_t hpet_number;
    uint16_t minimum_clk_tick;
    uint8_t page_protection;
} __attribute__((packed));

////////////////////////////////////////

/* PCI Express Memory-mapped Configuration Space base address description table */
#define MCFG_SIGNATURE "MCFG"
#define MCFG_REVISION 1
#define MCFG_NUM_CONFIG_SPACES 1

#define ECAM_GPA 0xe0000000

struct pcie_config_space_addr_structure {
    uint64_t base_address;
    uint16_t pci_segment_group;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
} __attribute__((packed));;

struct mcfg {
    struct dst_header h;
    struct pcie_config_space_addr_structure config_spaces[MCFG_NUM_CONFIG_SPACES];
} __attribute__((packed));;

////////////////////////////////////////

struct FADT {
    struct   dst_header h;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t  Reserved;

    uint8_t  PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t  AcpiEnable;
    uint8_t  AcpiDisable;
    uint8_t  S4BIOS_REQ;
    uint8_t  PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t  PM1EventLength;
    uint8_t  PM1ControlLength;
    uint8_t  PM2ControlLength;
    uint8_t  PMTimerLength;
    uint8_t  GPE0Length;
    uint8_t  GPE1Length;
    uint8_t  GPE1Base;
    uint8_t  CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t  DutyOffset;
    uint8_t  DutyWidth;
    uint8_t  DayAlarm;
    uint8_t  MonthAlarm;
    uint8_t  Century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t BootArchitectureFlags;

    uint8_t  Reserved2;
    uint32_t Flags;

    struct address_structure ResetReg;

    uint8_t  ResetValue;
    uint8_t  Reserved3[3];

    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                X_FirmwareControl;
    uint64_t                X_Dsdt;

    struct address_structure X_PM1aEventBlock;
    struct address_structure X_PM1bEventBlock;
    struct address_structure X_PM1aControlBlock;
    struct address_structure X_PM1bControlBlock;
    struct address_structure X_PM2ControlBlock;
    struct address_structure X_PMTimerBlock;
    struct address_structure X_GPE0Block;
    struct address_structure X_GPE1Block;
} __attribute__((packed));;

////////////////////////////////////////

// Note that the MADT is variable-sized depends on how many entries it have! Caller
// must ensure sufficient memory
size_t madt_build(struct madt *madt);

size_t hpet_build(struct hpet *hpet);
size_t fadt_build(struct FADT *fadt, uint64_t dsdt_gpa);
size_t xsdt_build(struct xsdt *xsdt, uint64_t *table_ptrs, size_t num_table_ptrs);
size_t xsdp_build(struct xsdp *xsdp, uint64_t xsdt_gpa);

// Create all the ACPI tables from guest's ram_top.
// Returns GPA of the XSDP.
// The contiguous range of memory used by all ACPI tables are written to acpi_start_gpa and acpi_end_gpa
// Assume that the Guest-Physical Address of RAM is 0!!!
uint64_t acpi_build_all(uintptr_t guest_ram_vaddr, void *dsdt_blob, uint64_t dsdt_blob_size, uint64_t ram_top,
                        uint64_t *acpi_start_gpa, uint64_t *acpi_end_gpa);
