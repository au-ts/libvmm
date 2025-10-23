#include <string.h>
#include <stdbool.h>
#include <libvmm/util/util.h>

#define XSDT_OEMID "libvmm"

#define XSDP_OEMID "x-oem-id"
#define XSDP_REVISION 2

#define XSDT_REVISION 1

/* https://wiki.osdev.org/RSDP */
struct XSDP {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;      // deprecated since version 2.0

 uint32_t Length;
 uint64_t XsdtAddress;
 uint8_t ExtendedChecksum;
 uint8_t reserved[3];
} __attribute__ ((packed));

struct ACPISDTHeader {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
};

typedef struct {
  uint8_t AddressSpace;
  uint8_t BitWidth;
  uint8_t BitOffset;
  uint8_t AccessSize;
  uint64_t Address;
} GenericAddressStructure;

struct FADT {
    struct   ACPISDTHeader h;
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

    // 12 byte structure; see below for details
    GenericAddressStructure ResetReg;

    uint8_t  ResetValue;
    uint8_t  Reserved3[3];

    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                X_FirmwareControl;
    uint64_t                X_Dsdt;

    GenericAddressStructure X_PM1aEventBlock;
    GenericAddressStructure X_PM1bEventBlock;
    GenericAddressStructure X_PM1aControlBlock;
    GenericAddressStructure X_PM1bControlBlock;
    GenericAddressStructure X_PM2ControlBlock;
    GenericAddressStructure X_PMTimerBlock;
    GenericAddressStructure X_GPE0Block;
    GenericAddressStructure X_GPE1Block;
};

// One entry for FADT, and one entry for DSDT
#define XSDT_ENTRIES 2

static uint8_t acpi_sdt_sum(struct ACPISDTHeader *xsdt) {
    uint8_t sum = 0;
    for (int i = 0; i < xsdt->Length; i++) {
        sum += ((char *)(xsdt))[i];
    }

    return sum;
}

static bool acpi_sdt_header_check(struct ACPISDTHeader *xsdt) {
    return acpi_sdt_sum(xsdt) == 0;
}

static uint8_t acpi_sdt_header_checksum(struct ACPISDTHeader *xsdt) {
    return 0x100 - acpi_sdt_sum(xsdt);
}

static void xsdt_build(void *xsdt_ram, uint64_t table_ptrs_gpa) {
    struct ACPISDTHeader *xsdt = (struct ACPISDTHeader *)xsdt_ram;
    memcpy(xsdt->Signature, "XSDT", 4);

    /* Length is the size of the header and the memory footprint of pointers to other tables. */
    uint32_t length = sizeof(struct ACPISDTHeader) + sizeof(uint64_t) * XSDT_ENTRIES;

    xsdt->Length = length;
    xsdt->Revision = XSDP_REVISION;

    // TODO: not very elegant, maybe do something better.
    memcpy(xsdt->OEMID, XSDT_OEMID, 6);
    memcpy(xsdt->OEMTableID, XSDT_OEMID, 6);
    xsdt->OEMRevision = 1;

    xsdt->CreatorID = 1;
    xsdt->CreatorRevision = 1;

    uint64_t *table_ptrs = (uint64_t *)(xsdt_ram + sizeof(struct ACPISDTHeader));
    /* Add FADT pointer */
    table_ptrs[0] = table_ptrs_gpa;
    table_ptrs[1] = table_ptrs_gpa - sizeof(struct FADT);

    xsdt->Checksum = acpi_sdt_header_checksum(xsdt);
    assert(acpi_sdt_header_check(xsdt));
}

void acpi_rsdp_init(uint64_t guest_ram_start, struct XSDP *xsdp) {
    // memcpy as we do not want to null-termiante
    memcpy(xsdp->Signature, "RSD PTR ", 8);

    // TODO: checksum

    memcpy(xsdp->OEMID, XSDP_OEMID, strlen(XSDP_OEMID));

    xsdp->Revision = XSDP_REVISION;
}
