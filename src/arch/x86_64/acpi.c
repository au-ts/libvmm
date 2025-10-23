/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

#define PAGE_SIZE_4K 0x1000
#define ACPI_OEMID "libvmm"

/* Documents referenced:
 * [1] Advanced Configuration and Power Interface (ACPI) Specification Release 6.5 UEFI Forum, Inc. Aug 29, 2022
 *     [1a] "5.2.5.3 Root System Description Pointer (RSDP) Structure"
 *     [1b] "5.2.6 System Description Table Header"
 *     [1c] "5.2.8 Extended System Description Table (XSDT)"
 */

static uint8_t acpi_table_sum(const char *table, int size) {
    uint8_t sum = 0;
    for (int i = 0; i < size; i++) {
        sum += table[i];
    }
    return sum;
}

static bool acpi_checksum_ok(const char *table, int size) {
    return acpi_table_sum(table, size) == 0;
}

static uint8_t acpi_compute_checksum(const char *table, int size) {
    return 0x100 - acpi_table_sum(table, size);
}

/* XSDP (Root System Description Pointer) -> XDST (Extended System Description Table)
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
} __attribute__ ((packed));

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
};

/* [1c] Root System Description Table */
#define XSDT_SIGNATURE "XSDT"
#define XSDT_MAX_ENTRIES 16 // arbitrary, can be increased easily by editing this number
struct xsdt {
    struct dst_header h;
    uint64_t gpa_to_other_sdt[XSDT_MAX_ENTRIES];
};

// typedef struct {
//     uint8_t AddressSpace;
//     uint8_t BitWidth;
//     uint8_t BitOffset;
//     uint8_t AccessSize;
//     uint64_t Address;
// } GenericAddressStructure;

// struct FADT {
//     struct   ACPISDTHeader h;
//     uint32_t FirmwareCtrl;
//     uint32_t Dsdt;

//     // field used in ACPI 1.0; no longer in use, for compatibility only
//     uint8_t  Reserved;

//     uint8_t  PreferredPowerManagementProfile;
//     uint16_t SCI_Interrupt;
//     uint32_t SMI_CommandPort;
//     uint8_t  AcpiEnable;
//     uint8_t  AcpiDisable;
//     uint8_t  S4BIOS_REQ;
//     uint8_t  PSTATE_Control;
//     uint32_t PM1aEventBlock;
//     uint32_t PM1bEventBlock;
//     uint32_t PM1aControlBlock;
//     uint32_t PM1bControlBlock;
//     uint32_t PM2ControlBlock;
//     uint32_t PMTimerBlock;
//     uint32_t GPE0Block;
//     uint32_t GPE1Block;
//     uint8_t  PM1EventLength;
//     uint8_t  PM1ControlLength;
//     uint8_t  PM2ControlLength;
//     uint8_t  PMTimerLength;
//     uint8_t  GPE0Length;
//     uint8_t  GPE1Length;
//     uint8_t  GPE1Base;
//     uint8_t  CStateControl;
//     uint16_t WorstC2Latency;
//     uint16_t WorstC3Latency;
//     uint16_t FlushSize;
//     uint16_t FlushStride;
//     uint8_t  DutyOffset;
//     uint8_t  DutyWidth;
//     uint8_t  DayAlarm;
//     uint8_t  MonthAlarm;
//     uint8_t  Century;

//     // reserved in ACPI 1.0; used since ACPI 2.0+
//     uint16_t BootArchitectureFlags;

//     uint8_t  Reserved2;
//     uint32_t Flags;

//     // 12 byte structure; see below for details
//     GenericAddressStructure ResetReg;

//     uint8_t  ResetValue;
//     uint8_t  Reserved3[3];

//     // 64bit pointers - Available on ACPI 2.0+
//     uint64_t                X_FirmwareControl;
//     uint64_t                X_Dsdt;

//     GenericAddressStructure X_PM1aEventBlock;
//     GenericAddressStructure X_PM1bEventBlock;
//     GenericAddressStructure X_PM1aControlBlock;
//     GenericAddressStructure X_PM1bControlBlock;
//     GenericAddressStructure X_PM2ControlBlock;
//     GenericAddressStructure X_PMTimerBlock;
//     GenericAddressStructure X_GPE0Block;
//     GenericAddressStructure X_GPE1Block;
// };

// // One entry for FADT, and one entry for DSDT
// #define XSDT_ENTRIES 2

// static uint8_t acpi_sdt_sum(struct ACPISDTHeader *hdr) {
//     uint8_t sum = 0;
//     for (int i = 0; i < hdr->Length; i++) {
//         sum += ((char *)(hdr))[i];
//     }

//     return sum;
// }

// static bool acpi_sdt_header_check(struct ACPISDTHeader *hdr) {
//     return acpi_sdt_sum(hdr) == 0;
// }

// static uint8_t acpi_sdt_header_checksum(struct ACPISDTHeader *hdr) {
//     return 0x100 - acpi_sdt_sum(hdr);
// }

// static void fadt_build(struct FADT *fadt, uint64_t xsdt_gpa) {
//     /* Despite the table being called 'FADT', this table was FACP in an earlier ACPI version,
//      * hence the inconsistency. */
//     memcpy(fadt->h.Signature, "FACP", 4);

//     fadt->h.Length = sizeof(struct FADT);
//     fadt->h.Revision = 6;

//     memcpy(fadt->h.OEMID, XSDT_OEMID, 6);
//     memcpy(fadt->h.OEMTableID, XSDT_OEMID, 6);
//     fadt->h.OEMRevision = 1;

//     fadt->h.CreatorID = 1;
//     fadt->h.CreatorRevision = 1;

//     /* Fill out data fields of FADT */

//     fadt->h.Checksum = acpi_sdt_header_checksum(&fadt->h);
//     assert(acpi_sdt_header_check(&fadt->h));
// }

// static size_t xsdt_build(void *xsdt_ram, uint64_t table_ptrs_gpa) {
//     struct ACPISDTHeader *xsdt = (struct ACPISDTHeader *)xsdt_ram;
//     memcpy(xsdt->Signature, "XSDT", 4);

//     /* Length is the size of the header and the memory footprint of pointers to other tables. */
//     uint32_t length = sizeof(struct ACPISDTHeader) + sizeof(uint64_t) * XSDT_ENTRIES;

//     xsdt->Length = length;
//     xsdt->Revision = XSDP_REVISION;

//     // TODO: not very elegant, maybe do something better.
//     memcpy(xsdt->OEMID, XSDT_OEMID, 6);
//     memcpy(xsdt->OEMTableID, XSDT_OEMID, 6);
//     xsdt->OEMRevision = 1;

//     xsdt->CreatorID = 1;
//     xsdt->CreatorRevision = 1;

//     uint64_t *table_ptrs = (uint64_t *)(xsdt_ram + sizeof(struct ACPISDTHeader));
//     /* Add FADT pointer */
//     table_ptrs[0] = table_ptrs_gpa;
//     table_ptrs[1] = table_ptrs_gpa - sizeof(struct FADT);

//     xsdt->Checksum = acpi_sdt_header_checksum(xsdt);
//     assert(acpi_sdt_header_check(xsdt));

//     return xsdt->Length;
// }

uint64_t acpi_rsdp_init(uintptr_t guest_ram_vaddr, uint64_t ram_top, uint64_t *acpi_start_gpa, uint64_t *acpi_end_gpa) {
    // Step 1: create the Root System Description Pointer structure.

    // We want to place everything at "ram_top", do that we can carve out a chunk
    // at the end and mark it as "ACPI" memory in the E820 table.
    uint64_t xsdp_gpa = ROUND_DOWN(ram_top - sizeof(struct xsdp), XSDP_ALIGN);
    struct xsdp *xsdp = (struct xsdp *) (guest_ram_vaddr + xsdp_gpa);
    memset(xsdp, 0, sizeof(struct xsdp));

    // memcpy as we do not want to null-termiante
    memcpy(xsdp->signature, XSDP_SIGNATURE, strlen(XSDP_SIGNATURE));
    memcpy(xsdp->oem_id, ACPI_OEMID, strlen(ACPI_OEMID));
    xsdp->revision = XSDP_REVISION;
    xsdp->length = sizeof(struct xsdp);

    // All the other tables "grow down" from the XSDP, here we pre-allocate the XSDT
    // so that we can compute the XSDP checksum.
    uint64_t apci_tables_gpa_watermark = xsdp_gpa;
    apci_tables_gpa_watermark -= sizeof(struct xsdt);
    xsdp->xsdt_gpa = apci_tables_gpa_watermark;

    xsdp->checksum = acpi_compute_checksum(xsdp, offsetof(struct xsdp, length));
    assert(acpi_checksum_ok(xsdp, offsetof(struct xsdp, length)));
    xsdp->ext_checksum = acpi_compute_checksum(xsdp, sizeof(struct xsdp));
    assert(acpi_checksum_ok(xsdp, sizeof(struct xsdp)));

    struct xdst *xsdt = (struct xdst *) (guest_ram_vaddr + xsdp->xsdt_gpa);
    // todo

    *acpi_start_gpa = ROUND_DOWN(apci_tables_gpa_watermark, PAGE_SIZE_4K);
    *acpi_end_gpa = ram_top;
    return xsdp_gpa;
}
