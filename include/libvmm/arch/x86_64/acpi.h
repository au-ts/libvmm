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
 *     [1d] "5.2.12 Multiple APIC Description Table (MADT)"
 *     [1e] "5.2.10 Firmware ACPI Control Structure (FACS)"
 *     [1f] "4.8.3.3 Power Management Timer (PM_TMR)"
 *     [1g] "5.2.9 Fixed ACPI Description Table (FADT)"
 * [2] IA-PC HPET (High Precision Event Timers) Specification Revision: 1.0a Date: October 2004
 *     https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf
 *     [2a] "3.2.4 The ACPI 2.0 HPET Description Table (HPET)"
 * [3] PCI SIG, PCI Firmware Specification Revision 3.3 January 20, 2021
 *     [3a] "Table 4-2: MCFG Table to Support Enhanced Configuration Space Access"
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
    uint32_t rsdp_gpa;

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

/* [1d] Multiple APIC Description Table */
#define MADT_ENTRY_TYPE_LAPIC 0x0
#define MADT_ENTRY_TYPE_IOAPIC 0x1

#define MADT_ENTRY_LAPIC_LENGTH 0x8
#define MADT_ENTRY_IOAPIC_LENGTH 0xc

struct madt_irq_controller {
    uint8_t type;
    /* Note that the length includes this MADT IRQ controller header. */
    uint8_t length;
} __attribute__((packed));

#define MADT_LAPIC_FLAGS BIT(0) /* Enabled bit */

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

#define MADT_REVISION 5
#define MADT_FLAGS BIT(0) // PCAT_COMPAT, dual 8259 present as a direct mapping to I/O APIC 0

struct madt {
    struct dst_header h;
    uint32_t apic_addr;
    uint32_t flags;

    // @billn doing it this way make it hard for user code to add more entries,
    // but for now it make things way easier as we can just do sizeof(struct madt) as it
    // is statically sized.
    struct madt_lapic lapic_entry;
    struct madt_ioapic ioapic_0_entry;
    struct madt_ioapic_source_override hpet_source_override_entry;
} __attribute__((packed));

////////////////////////////////////////

/* [2a] HPET Description Table */
#define HPET_SIGNATURE "HPET"
#define HPET_REVISION 0x1

#define HPET_VENDOR_MASK (0x5E14ull << 16)
#define HPET_64B_COUNTER BIT(13)
#define HPET_LEGACY_IRQ_CAPABLE BIT(15)
#define HPET_NUM_COMPARATORS_VALUE 3
#define HPET_NUM_COMPARATORS_SHIFT 8

struct address_structure {
    uint8_t address_space_id;    // 0 - MMIO, 1 - I/O port, 2 - PCI config space
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t access_size; // 0 - undefied, 1 - byte, 2 - word, 3 - dword, 4 - qword
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

/* [3a] PCI Express Memory-mapped Configuration Space base address description table */
#define MCFG_SIGNATURE "MCFG"
#define MCFG_REVISION 1
#define MCFG_NUM_CONFIG_SPACES 1

struct pcie_config_space_addr_structure {
    uint64_t base_address;
    uint16_t pci_segment_group;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
} __attribute__((packed));

struct mcfg {
    struct dst_header h;
    uint8_t reserved[8];
    struct pcie_config_space_addr_structure config_spaces[MCFG_NUM_CONFIG_SPACES];
} __attribute__((packed));

////////////////////////////////////////

/* [1e] Firmware ACPI Control Structure (FACS) */
#define FACS_SIGNATURE "FACS"

struct facs {
    uint32_t signature;
    uint32_t length;
    uint32_t hw_sig;
    uint32_t fw_waking_vector;
    uint32_t global_lock;
    uint32_t flags;
    uint64_t x_fw_waking_vector;
    uint8_t version;
    uint8_t _reserved1[3];
    uint32_t ospm_flags;
    uint8_t _reserved2[24];
};

_Static_assert(sizeof(struct facs) == 64, "FACS must be 64 bytes large");

////////////////////////////////////////

/* [1g] Fixed ACPI Description Table (FADT) */

/* Arbitrary addresses chosen to not collide with anything else */
#define PM1A_EVT_BLK_PIO_ADDR 0x600
#define PM1A_EVT_BLK_PIO_LEN 4
#define PM1A_CNT_BLK_PIO_ADDR 0x604
#define PM1A_CNT_BLK_PIO_LEN 2
#define PM_TMR_BLK_PIO_ADDR 0x608
#define PM_TMR_BLK_PIO_LEN 4
#define SMI_CMD_PIO_ADDR 0x60c

#define ACPI_ENABLE 0x12
#define ACPI_DISABLE 0x13

// @billn have a way to detect and warn about collision
#define ACPI_SCI_IRQ_PIN 7
#define ACPI_PMT_FREQ_HZ 3579545 // [1f]
#define ACPI_PMT_MAX_COUNT (1 << 24)

struct fadt {
    struct dst_header h;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t Reserved;

    uint8_t PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t AcpiEnable;
    uint8_t AcpiDisable;
    uint8_t S4BIOS_REQ;
    uint8_t PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t PM1EventLength;
    uint8_t PM1ControlLength;
    uint8_t PM2ControlLength;
    uint8_t PMTimerLength;
    uint8_t GPE0Length;
    uint8_t GPE1Length;
    uint8_t GPE1Base;
    uint8_t CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t DutyOffset;
    uint8_t DutyWidth;
    uint8_t DayAlarm;
    uint8_t MonthAlarm;
    uint8_t Century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t BootArchitectureFlags;

    uint8_t Reserved2;
    uint32_t Flags;

    struct address_structure ResetReg;

    uint8_t ResetValue;
    uint8_t Reserved3[3];

    // 64bit pointers - Available on ACPI 2.0+
    uint64_t X_FirmwareControl;
    uint64_t X_Dsdt;

    struct address_structure X_PM1aEventBlock;
    struct address_structure X_PM1bEventBlock;
    struct address_structure X_PM1aControlBlock;
    struct address_structure X_PM1bControlBlock;
    struct address_structure X_PM2ControlBlock;
    struct address_structure X_PMTimerBlock;
    struct address_structure X_GPE0Block;
    struct address_structure X_GPE1Block;
} __attribute__((packed));

////////////////////////////////////////

size_t facs_build(struct facs *facs);
size_t madt_build(struct madt *madt);
size_t hpet_build(struct hpet *hpet);
size_t fadt_build(struct fadt *fadt, uint64_t dsdt_gpa, uint64_t facs_gpa);
size_t xsdt_build(struct xsdt *xsdt, uint64_t *table_ptrs, size_t num_table_ptrs);
size_t mcfg_build(struct mcfg *mcfg);
size_t xsdp_build(struct xsdp *xsdp, uint64_t xsdt_gpa);

// Create all the ACPI tables in guest RAM starting from `acpi_gpa_start`.
// Returns GPA of the XSDP and the sum of the tables' size in bytes will be written to `num_bytes_used`
// Returns 0 and `num_bytes_used` will be 0 if out of guest memory
uint64_t acpi_build_all(uint64_t acpi_gpa_start, void *dsdt_blob, uint64_t dsdt_blob_size, uint64_t *num_bytes_used);
