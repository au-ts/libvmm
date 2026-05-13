/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <libvmm/guest_ram.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/memory_space.h>
#include <libvmm/arch/x86_64/guest_time.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>

/* Internal structures for the tables builder function. */

struct acpi_builder {
    uint64_t size;
    uint64_t start_gpa;
};

struct acpi_table_alloc {
    void *vaddr;
    uint64_t gpa;
};

/* Documents referenced:
 * [1] Advanced Configuration and Power Interface (ACPI) Specification Release 6.5 UEFI Forum, Inc. Aug 29, 2022
 *     [1a] "5.2.5.3 Root System Description Pointer (RSDP) Structure"
 *     [1b] "5.2.6 System Description Table Header"
 *     [1c] "5.2.8 Extended System Description Table (XSDT)"
 * [2] IA-PC HPET (High Precision Event Timers) Specification Revision: 1.0a Date: October 2004
 *     https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf
 *     [2a] "3.2.4 The ACPI 2.0 HPET Description Table (HPET)"
 */

#define LOG_ACPI_INFO(...) do{ printf("%s|ACPI INFO: ", microkit_name); printf(__VA_ARGS__); }while(0)
#define LOG_ACPI_ERR(...) do{ printf("%s|ACPI ERR: ", microkit_name); printf(__VA_ARGS__); }while(0)

/* This isn't enforced by any specifications, but we will align all the tables to prevent
 * UBSAN from tripping. */
#define ACPI_TABLES_ALIGNMENT 8

/* The FACS is an exception to the above. [1] "5.2.10 Firmware ACPI Control Structure (FACS)":
 * "The platform boot firmware aligns the FACS on a 64-byte boundary anywhere within the system’s memory address space." */
#define FACS_ALIGNMENT 64

#define PAGE_SIZE_4K 0x1000
#define ACPI_OEMID "libvmm"

static uint8_t acpi_table_sum(char *table, int size)
{
    uint8_t sum = 0;
    for (int i = 0; i < size; i++) {
        sum += (unsigned char)table[i];
    }
    return sum;
}

static bool acpi_checksum_ok(char *table, int size)
{
    return acpi_table_sum(table, size) == 0;
}

static uint8_t acpi_compute_checksum(char *table, int size)
{
    return 0x100 - acpi_table_sum(table, size);
}

size_t madt_build(struct madt *madt)
{
    memcpy(madt->h.signature, "APIC", 4);
    madt->h.revision = MADT_REVISION;
    memcpy(madt->h.oem_id, ACPI_OEMID, 6);
    memcpy(madt->h.oem_table_id, ACPI_OEMID, 6);
    madt->h.oem_revision = 1;

    madt->h.length = sizeof(struct madt);

    madt->h.creator_id = 1;
    madt->h.creator_revision = 1;

    madt->lapic_entry = (struct madt_lapic) {
        .entry = {
            .type = MADT_ENTRY_TYPE_LAPIC,
            .length = MADT_ENTRY_LAPIC_LENGTH,
        },
        .acpi_processor_id = 0,
        .apic_id = 0,
        .flags = MADT_LAPIC_FLAGS,
    };

    madt->ioapic_0_entry = (struct madt_ioapic) {
        .entry = {
            .type = MADT_ENTRY_TYPE_IOAPIC,
            .length = MADT_ENTRY_IOAPIC_LENGTH,
        },
        .id = 0,
        .res = 0,
        .address = IOAPIC_GPA,
        .global_system_irq_base = 0,
    };

    // Connect ISA IRQ 0 from the legacy Programmable Interval Timer (PIT)
    // to I/O APIC pin 2, which is where the HPET in legacy replacement code is connected.
    madt->hpet_source_override_entry = (struct madt_ioapic_source_override) {
        .entry = { .type = 2, .length = 10 },
        .bus = 0,
        .source = 0,
        .gsi = 2,
        .flags = 0,
    };

    madt->apic_addr = LAPIC_GPA;
    madt->flags = MADT_FLAGS;

    // Finished building, now do the checksum.
    madt->h.checksum = acpi_compute_checksum((char *)madt, madt->h.length);
    assert(acpi_checksum_ok((char *)madt, madt->h.length));

    return madt->h.length;
}

size_t hpet_build(struct hpet *hpet)
{
    memcpy(hpet->h.signature, HPET_SIGNATURE, 4);
    hpet->h.length = sizeof(struct hpet);
    hpet->h.revision = HPET_REVISION;
    memcpy(hpet->h.oem_id, ACPI_OEMID, 6);
    memcpy(hpet->h.oem_table_id, ACPI_OEMID, 6);
    hpet->h.creator_id = 1;
    hpet->h.creator_revision = 1;

    hpet->evt_timer_block_id = HPET_LEGACY_IRQ_CAPABLE | (HPET_NUM_COMPARATORS_VALUE << HPET_NUM_COMPARATORS_SHIFT)
                             | HPET_64B_COUNTER | HPET_VENDOR_MASK;

    hpet->address_desc.address_space_id = 0;
    hpet->address_desc.register_bit_width = 32;
    hpet->address_desc.address = HPET_GPA;

    hpet->hpet_number = 0;
    hpet->minimum_clk_tick = 1;
    hpet->page_protection = 0;

    hpet->h.checksum = acpi_compute_checksum((char *)hpet, hpet->h.length);
    assert(acpi_checksum_ok((char *)hpet, hpet->h.length));

    return sizeof(struct hpet);
}

size_t mcfg_build(struct mcfg *mcfg)
{
    memcpy(mcfg->h.signature, MCFG_SIGNATURE, 4);
    mcfg->h.length = sizeof(struct mcfg);
    mcfg->h.revision = MCFG_REVISION;
    memcpy(mcfg->h.oem_id, ACPI_OEMID, 6);
    memcpy(mcfg->h.oem_table_id, ACPI_OEMID, 6);
    mcfg->h.creator_id = 1;
    mcfg->h.creator_revision = 1;

    mcfg->config_spaces[0].base_address = ECAM_GPA;
    mcfg->config_spaces[0].pci_segment_group = 0;
    mcfg->config_spaces[0].start_bus = 0;
    mcfg->config_spaces[0].end_bus = 0;

    mcfg->h.checksum = acpi_compute_checksum((char *)mcfg, mcfg->h.length);
    assert(acpi_checksum_ok((char *)mcfg, mcfg->h.length));

    return sizeof(struct mcfg);
}

static uint16_t pm1_enable_reg = 0;
static uint16_t pm1_control_reg = 0;
static uint16_t pm1_status_reg = 0;

#define TMR_EN BIT(0) /* Timer Enable */
#define GBL_EN BIT(5) /* Global Enable */
bool acpi_pm_timer_can_irq(void)
{
    return (pm1_enable_reg & BIT(0)) && (pm1_enable_reg & BIT(5));
}

bool pm1a_cnt_pio_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification, seL4_VCPUContext *vctx,
                               void *cookie)
{
    assert(!pio_fault_is_string_op(qualification));
    int access_width_bytes = pio_fault_to_access_width_bytes(qualification);
    assert(access_width_bytes == 2);

    if (port_offset == 0) {
        if (pio_fault_is_read(qualification)) {
            vctx->eax = pm1_control_reg;
        } else {
            pm1_control_reg = vctx->eax & 0xffff;
        }

    } else {
        return false;
    }

    return true;
}

bool pm1a_evt_pio_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification, seL4_VCPUContext *vctx,
                               void *cookie)
{
    assert(!pio_fault_is_string_op(qualification));
    int access_width_bytes = pio_fault_to_access_width_bytes(qualification);
    assert(access_width_bytes == 2);

    if (port_offset == 2) {
        if (pio_fault_is_read(qualification)) {
            vctx->eax = pm1_enable_reg;
        } else {
            pm1_enable_reg = vctx->eax;
            if (acpi_pm_timer_can_irq()) {
                LOG_ACPI_INFO("ACPI PM Timer Overflow IRQ ON!\n");
                assert(false);
            }
        }
    } else if (port_offset == 0) {
        if (pio_fault_is_read(qualification)) {
            vctx->eax = pm1_status_reg;
        } else {
            // @billn sus implement proper set to clear
            pm1_status_reg = vctx->eax;
        }
    } else {
        return false;
    }

    return true;
}

bool pm_timer_pio_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification, seL4_VCPUContext *vctx,
                               void *cookie)
{
    assert(!pio_fault_is_string_op(qualification));
    int access_width_bytes = pio_fault_to_access_width_bytes(qualification);
    assert(access_width_bytes == 4);
    assert(pio_fault_is_read(qualification));

    vctx->eax = convert_ticks_by_frequency(guest_time_tsc_now(), guest_time_tsc_hz(), ACPI_PMT_FREQ_HZ);

    return true;
}

bool smi_cmd_pio_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification, seL4_VCPUContext *vctx,
                              void *cookie)
{
    assert(!pio_fault_is_string_op(qualification));
    int access_width_bytes = pio_fault_to_access_width_bytes(qualification);
    assert(access_width_bytes == 1);
    assert(pio_fault_is_write(qualification));

    uint8_t cmd = vctx->eax & 0xff;
    if (cmd == ACPI_ENABLE) {
        pm1_control_reg |= BIT(0);
    } else if (cmd == ACPI_DISABLE) {
        pm1_control_reg |= ~BIT(0);
    }

    return true;
}

size_t facs_build(struct facs *facs)
{
    // @billn figure out if wake vector is important

    memset(facs, 0, sizeof(struct facs));
    memcpy(&facs->signature, FACS_SIGNATURE, 4);

    facs->length = sizeof(struct facs);

    return sizeof(struct facs);
}

size_t fadt_build(struct fadt *fadt, uint64_t dsdt_gpa, uint64_t facs_gpa)
{
    /* Despite the table being called 'FADT', this table was FACP in an earlier ACPI version,
     * hence the inconsistency. */
    memset(fadt, 0, sizeof(struct fadt));

    memcpy(fadt->h.signature, "FACP", 4);

    fadt->h.length = sizeof(struct fadt);
    fadt->h.revision = 6;

    memcpy(fadt->h.oem_id, ACPI_OEMID, 6);
    memcpy(fadt->h.oem_table_id, ACPI_OEMID, 6);
    fadt->h.oem_revision = 1;

    fadt->h.creator_id = 1;
    fadt->h.creator_revision = 1;

    /* Fill out data fields of FADT */

    assert(dsdt_gpa < (1ull << 32));
    assert(facs_gpa < (1ull << 32));

    fadt->Dsdt = 0;
    fadt->X_Dsdt = dsdt_gpa;
    fadt->FirmwareCtrl = 0;
    fadt->X_FirmwareControl = facs_gpa;
    fadt->Flags = 0; // Not hardware reduced, ACPI PM timer 24-bit wide

    fadt->PreferredPowerManagementProfile = 0; /* Unspecified Power Profile */

    fadt->SCI_Interrupt = ACPI_SCI_IRQ_PIN;

    fadt->BootArchitectureFlags =
        BIT(1) /* Support PS/2 KB+M */ | BIT(3) /* MSI not supported */ | BIT(2) /* No ISA VGA */;

    fadt->SMI_CommandPort = SMI_CMD_PIO_ADDR;
    fadt->AcpiEnable = ACPI_ENABLE;
    fadt->AcpiDisable = ACPI_DISABLE;
    {
        bool success = fault_register_pio_exception_handler(SMI_CMD_PIO_ADDR, 1, smi_cmd_pio_fault_handle, NULL);
        assert(success);
    }

    fadt->PM1aEventBlock = PM1A_EVT_BLK_PIO_ADDR;
    fadt->PM1EventLength = PM1A_EVT_BLK_PIO_LEN;
    fadt->X_PM1aEventBlock.address_space_id = 1;
    fadt->X_PM1aEventBlock.register_bit_width = 32;
    fadt->X_PM1aEventBlock.register_bit_offset = 0;
    fadt->X_PM1aEventBlock.access_size = 2;
    fadt->X_PM1aEventBlock.address = PM1A_EVT_BLK_PIO_ADDR;
    {
        bool success = fault_register_pio_exception_handler(PM1A_EVT_BLK_PIO_ADDR, PM1A_EVT_BLK_PIO_LEN,
                                                            pm1a_evt_pio_fault_handle, NULL);
        assert(success);
    }

    fadt->PM1aControlBlock = PM1A_CNT_BLK_PIO_ADDR;
    fadt->PM1ControlLength = PM1A_CNT_BLK_PIO_LEN;
    fadt->X_PM1aControlBlock.address_space_id = 1;
    fadt->X_PM1aControlBlock.register_bit_width = 16;
    fadt->X_PM1aControlBlock.register_bit_offset = 0;
    fadt->X_PM1aControlBlock.access_size = 2;
    fadt->X_PM1aControlBlock.address = PM1A_CNT_BLK_PIO_ADDR;
    {
        bool success = fault_register_pio_exception_handler(PM1A_CNT_BLK_PIO_ADDR, PM1A_CNT_BLK_PIO_LEN,
                                                            pm1a_cnt_pio_fault_handle, NULL);
        assert(success);
    }

    fadt->PMTimerBlock = PM_TMR_BLK_PIO_ADDR;
    fadt->PMTimerLength = PM_TMR_BLK_PIO_LEN;
    fadt->X_PMTimerBlock.address_space_id = 1;
    fadt->X_PMTimerBlock.register_bit_width = 32;
    fadt->X_PMTimerBlock.register_bit_offset = 0;
    fadt->X_PMTimerBlock.access_size = 3;
    fadt->X_PMTimerBlock.address = PM_TMR_BLK_PIO_ADDR;
    {
        bool success = fault_register_pio_exception_handler(PM_TMR_BLK_PIO_ADDR, PM_TMR_BLK_PIO_LEN,
                                                            pm_timer_pio_fault_handle, NULL);
        assert(success);
    }
    // @billn sus, OVMF always think that its running on Xen, which places the ACPI PM timer is at 0xb008
    // Not sure if this is it's quirk or our fault somewhwere
    {
        bool success = fault_register_pio_exception_handler(0xb008, PM_TMR_BLK_PIO_LEN, pm_timer_pio_fault_handle,
                                                            NULL);
        assert(success);
    }

    fadt->h.checksum = acpi_compute_checksum((char *)fadt, fadt->h.length);
    assert(acpi_checksum_ok((char *)fadt, fadt->h.length));

    return fadt->h.length;
}

size_t xsdt_build(struct xsdt *xsdt, uint64_t *table_ptrs, size_t num_table_ptrs)
{
    memcpy(xsdt->h.signature, "XSDT", 4);

    /* length is the size of the header and the memory footprint of pointers to other tables. */
    uint32_t length = sizeof(struct xsdt);

    xsdt->h.length = length;
    xsdt->h.revision = XSDP_REVISION;

    memcpy(xsdt->h.oem_id, ACPI_OEMID, 6);
    memcpy(xsdt->h.oem_table_id, ACPI_OEMID, 6);
    xsdt->h.oem_revision = 1;

    xsdt->h.creator_id = 1;
    xsdt->h.creator_revision = 1;

    // TODO: remove limitation
    assert(num_table_ptrs == XSDT_ENTRIES);
    for (int i = 0; i < num_table_ptrs; i++) {
        xsdt->tables[i] = table_ptrs[i];
    }

    xsdt->h.checksum = acpi_compute_checksum((char *)xsdt, xsdt->h.length);
    assert(acpi_checksum_ok((char *)xsdt, xsdt->h.length));

    return xsdt->h.length;
}

size_t xsdp_build(struct xsdp *xsdp, uint64_t xsdt_gpa)
{
    memset(xsdp, 0, sizeof(struct xsdp));
    memcpy(xsdp->signature, XSDP_SIGNATURE, strlen(XSDP_SIGNATURE));
    memcpy(xsdp->oem_id, ACPI_OEMID, strlen(ACPI_OEMID));
    xsdp->revision = XSDP_REVISION;
    xsdp->length = sizeof(struct xsdp);

    xsdp->xsdt_gpa = xsdt_gpa;

    xsdp->checksum = acpi_compute_checksum((char *)xsdp, offsetof(struct xsdp, length));
    assert(acpi_checksum_ok((char *)xsdp, offsetof(struct xsdp, length)));
    xsdp->ext_checksum = acpi_compute_checksum((char *)xsdp, sizeof(struct xsdp));
    assert(acpi_checksum_ok((char *)xsdp, sizeof(struct xsdp)));

    return sizeof(struct xsdp);
}

static struct acpi_table_alloc acpi_alloc(struct acpi_builder *builder, uint64_t size, const char *name,
                                          uint64_t alignment)
{
    uint64_t bytes_remaining = 0;
    uint64_t gpa = builder->start_gpa + builder->size;
    void *table_vaddr = gpa_to_vaddr(gpa, &bytes_remaining);
    if (!table_vaddr || bytes_remaining < size) {
        LOG_VMM_ERR("acpi_build_all(): out of memory for %s, available %lu < needed %lu\n", name, bytes_remaining,
                    size);
        return (struct acpi_table_alloc) {
            .vaddr = NULL,
            .gpa = 0,
        };
    }
    builder->size += ROUND_UP(size, alignment);

    return (struct acpi_table_alloc) {
        .vaddr = table_vaddr,
        .gpa = gpa,
    };
}

uint64_t acpi_build_all(uint64_t acpi_gpa_start, void *dsdt_blob, uint64_t dsdt_blob_size, uint64_t *num_bytes_used)
{
    struct acpi_builder acpi_builder;
    memset(&acpi_builder, 0, sizeof(struct acpi_builder));
    acpi_builder.start_gpa = acpi_gpa_start;

    /* Firstly create the Root System Description Pointer structure. */
    struct acpi_table_alloc xsdp = acpi_alloc(&acpi_builder, sizeof(struct xsdp), "XSDP", ACPI_TABLES_ALIGNMENT);
    if (!xsdp.vaddr) {
        return 0;
    }

    /* All the other tables "grow down" from the XSDP, here we pre-allocate the XSDT
     * so that we can compute the XSDP checksum. */
    struct acpi_table_alloc xsdt = acpi_alloc(&acpi_builder, sizeof(struct xsdt), "XSDT", ACPI_TABLES_ALIGNMENT);
    if (!xsdt.vaddr) {
        return 0;
    }
    xsdp_build(xsdp.vaddr, xsdt.gpa);

    struct acpi_table_alloc madt = acpi_alloc(&acpi_builder, sizeof(struct madt), "MADT", ACPI_TABLES_ALIGNMENT);
    if (!madt.vaddr) {
        return 0;
    }
    madt_build(madt.vaddr);

    struct acpi_table_alloc hpet = acpi_alloc(&acpi_builder, sizeof(struct hpet), "HPET", ACPI_TABLES_ALIGNMENT);
    if (!hpet.vaddr) {
        return 0;
    }
    hpet_build(hpet.vaddr);

    /* Differentiated System Description Table
     * Used for things that cannot be described by other tables or probed.
     * An example is legacy I/O Port serial IRQ pin.
     * It is in a binary format from the ASL compiler, which just needs to be copied to guest RAM.
     */
    struct acpi_table_alloc dsdt = acpi_alloc(&acpi_builder, dsdt_blob_size, "DSDT", ACPI_TABLES_ALIGNMENT);
    if (!dsdt.vaddr) {
        return 0;
    }
    memcpy(dsdt.vaddr, dsdt_blob, dsdt_blob_size);

    struct acpi_table_alloc facs = acpi_alloc(&acpi_builder, sizeof(struct facs), "FACS", FACS_ALIGNMENT);
    if (!facs.vaddr) {
        return 0;
    }
    facs_build(facs.vaddr);

    struct acpi_table_alloc fadt = acpi_alloc(&acpi_builder, sizeof(struct fadt), "FADT", ACPI_TABLES_ALIGNMENT);
    if (!fadt.vaddr) {
        return 0;
    }
    fadt_build(fadt.vaddr, dsdt.gpa, facs.gpa);

    memset(xsdt.vaddr, 0, sizeof(struct xsdt));
    uint64_t xsdt_table_ptrs[XSDT_ENTRIES] = { madt.gpa, hpet.gpa, fadt.gpa };
    xsdt_build(xsdt.vaddr, xsdt_table_ptrs, XSDT_ENTRIES);

    *num_bytes_used = acpi_builder.size;
    return xsdp.gpa;
}
