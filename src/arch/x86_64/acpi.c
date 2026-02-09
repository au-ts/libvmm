/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/fault.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>

#define LOG_ACPI_INFO(...) do{ printf("%s|ACPI INFO: ", microkit_name); printf(__VA_ARGS__); }while(0)
#define LOG_ACPI_ERR(...) do{ printf("%s|ACPI ERR: ", microkit_name); printf(__VA_ARGS__); }while(0)

#define PAGE_SIZE_4K 0x1000
#define ACPI_OEMID "libvmm"

/* Documents referenced:
 * [1] Advanced Configuration and Power Interface (ACPI) Specification Release 6.5 UEFI Forum, Inc. Aug 29, 2022
 *     [1a] "5.2.5.3 Root System Description Pointer (RSDP) Structure"
 *     [1b] "5.2.6 System Description Table Header"
 *     [1c] "5.2.8 Extended System Description Table (XSDT)"
 * [2] IA-PC HPET (High Precision Event Timers) Specification Revision: 1.0a Date: October 2004
 *     https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf
 *     [2a] "3.2.4 The ACPI 2.0 HPET Description Table (HPET)"
 */

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

// TODO: when creating MADT, get the user to pass the address of APIC that is definitely
// outside of RAM, or determine it ourselves.

// static void madt_add_entry(struct madt *madt, uintptr_t dest, void *entry)
// {
//     struct madt_irq_controller *madt_entry = (struct madt_irq_controller *)entry;

//     madt->h.length += madt_entry->length;

//     memcpy((void *)dest, entry, madt_entry->length);
// }

size_t madt_build(struct madt *madt)
{
    memcpy(madt->h.signature, "APIC", 4);
    madt->h.revision = MADT_REVISION;
    // TODO: not very elegant, maybe do something better.
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
        .address = IOAPIC_ADDRESS,
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

    madt->apic_addr = MADT_LOCAL_APIC_ADDR;
    madt->flags = MADT_FLAGS;

    // uintptr_t watermark = (uintptr_t)madt + sizeof(struct madt);
    // madt_add_entry(madt, watermark, &lapic);
    // watermark += sizeof(struct madt_lapic);
    // madt_add_entry(madt, watermark, &ioapic);
    // watermark += sizeof(struct madt_ioapic);
    // madt_add_entry(madt, watermark, &ioapic_hpet_override);
    // watermark += sizeof(struct madt_ioapic_source_override);
    // madt_add_entry(madt, watermark, &ioapic_com1_override);

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
    hpet->minimum_clk_tick = 1; // @billn ??? revisit
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
static uint16_t pm1_status_reg = 0;

#define TMR_EN BIT(0) /* Timer Enable */
#define GBL_EN BIT(5) /* Global Enable */
bool acpi_pm_timer_can_irq(void)
{
    return (pm1_enable_reg & BIT(0)) && (pm1_enable_reg & BIT(5));
}

bool pm1a_evt_pio_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification, seL4_VCPUContext *vctx,
                               void *cookie)
{
    uint64_t is_read = qualification & BIT(3);
    uint64_t is_string = qualification & BIT(4);
    uint16_t port_addr = (qualification >> 16) & 0xffff;
    ioport_access_width_t access_width = (ioport_access_width_t)(qualification & 0x7);
    int access_width_bytes = ioports_access_width_to_bytes(access_width);
    assert(!is_string);
    assert(access_width_bytes == 2);

    if (port_offset == 2) {
        if (is_read) {
            vctx->eax = pm1_enable_reg;
        } else {
            pm1_enable_reg = vctx->eax;
            if (acpi_pm_timer_can_irq()) {
                LOG_ACPI_INFO("ACPI PM Timer Overflow IRQ ON!\n");
                assert(false);
            }
        }
    } else if (port_offset == 0) {
        if (is_read) {
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
    uint64_t is_read = qualification & BIT(3);
    uint64_t is_string = qualification & BIT(4);
    uint16_t port_addr = (qualification >> 16) & 0xffff;
    ioport_access_width_t access_width = (ioport_access_width_t)(qualification & 0x7);
    int access_width_bytes = ioports_access_width_to_bytes(access_width);
    assert(!is_string);
    assert(access_width_bytes == 4);
    assert(port_offset == 0);
    assert(is_read);

    uint64_t timer_ns = sddf_timer_time_now(TIMER_DRV_CH_FOR_LAPIC);
    vctx->eax = (uint64_t)(((double)timer_ns / (double)NS_IN_S) * ACPI_PMT_FREQ_HZ);

    return true;
}

size_t fadt_build(struct FADT *fadt, uint64_t dsdt_gpa)
{
    /* Despite the table being called 'FADT', this table was FACP in an earlier ACPI version,
     * hence the inconsistency. */
    memset(fadt, 0, sizeof(struct FADT));

    memcpy(fadt->h.signature, "FACP", 4);

    fadt->h.length = sizeof(struct FADT);
    fadt->h.revision = 6;

    memcpy(fadt->h.oem_id, ACPI_OEMID, 6);
    memcpy(fadt->h.oem_table_id, ACPI_OEMID, 6);
    fadt->h.oem_revision = 1;

    fadt->h.creator_id = 1;
    fadt->h.creator_revision = 1;

    /* Fill out data fields of FADT */
    fadt->Dsdt = (uint32_t)dsdt_gpa;
    fadt->X_Dsdt = dsdt_gpa;
    fadt->Flags = 0; // Not hardware reduced, ACPI PM timer 24-bit wide

    fadt->PreferredPowerManagementProfile = 0; /* Unspecified Power Profile */

    fadt->SCI_Interrupt = ACPI_SCI_IRQ_PIN;

    fadt->BootArchitectureFlags =
        BIT(1) /* Support PS/2 KB+M */ | BIT(3) /* MSI not supported */ | BIT(2) /* No ISA VGA */;

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

    // TODO: not very elegant, maybe do something better.
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

    // memcpy as we do not want to null-termiante
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

uint64_t acpi_top;

uint64_t acpi_allocate_gpa(size_t length)
{
    assert(length);
    acpi_top -= length;

    return acpi_top;
}

uint64_t acpi_build_all(uintptr_t guest_ram_vaddr, void *dsdt_blob, uint64_t dsdt_blob_size, uint64_t ram_top,
                        uint64_t *acpi_start_gpa, uint64_t *acpi_end_gpa)
{
    acpi_top = ram_top;
    // Step 1: create the Root System Description Pointer structure.

    // We want to place everything at "ram_top", do that we can carve out a chunk
    // at the end and mark it as "ACPI" memory in the E820 table.
    uint64_t xsdp_gpa = acpi_allocate_gpa(sizeof(struct xsdp));
    struct xsdp *xsdp = (struct xsdp *)(guest_ram_vaddr + xsdp_gpa);

    // All the other tables "grow down" from the XSDP, here we pre-allocate the XSDT
    // so that we can compute the XSDP checksum.
    uint64_t xsdt_gpa = acpi_allocate_gpa(sizeof(struct xsdt));
    xsdp_build(xsdp, xsdt_gpa);

    // TODO: hack
    uint64_t madt_gpa = acpi_allocate_gpa(sizeof(struct madt));
    struct madt *madt = (struct madt *)(guest_ram_vaddr + madt_gpa);
    madt_build(madt);
    assert(madt->h.length <= 0x1000);

    // @billn todo really need some sort of memory range allocator
    uint64_t hpet_gpa = acpi_allocate_gpa(sizeof(struct hpet));
    struct hpet *hpet = (struct hpet *)(guest_ram_vaddr + hpet_gpa);
    hpet_build(hpet);
    assert(hpet->h.length <= 0x1000);

    // Differentiated System Description Table
    // Used for things that cannot be described by other tables or probed.
    // An example is legacy I/O Port serial IRQ pin.
    // It is in a binary format from the ASL compiler, which just needs to be copied to guest RAM.
    assert(dsdt_blob_size < 0x1000);
    uint64_t dsdt_gpa = acpi_allocate_gpa(0x1000);
    memcpy((void *)(guest_ram_vaddr + dsdt_gpa), dsdt_blob, dsdt_blob_size);

    uint64_t fadt_gpa = acpi_allocate_gpa(sizeof(struct FADT));
    struct FADT *fadt = (struct FADT *)(guest_ram_vaddr + fadt_gpa);
    fadt_build(fadt, dsdt_gpa);
    assert(fadt->h.length <= 0x1000);

    uint64_t mcfg_gpa = acpi_allocate_gpa(sizeof(struct mcfg));
    struct mcfg *mcfg = (struct mcfg *)(guest_ram_vaddr + mcfg_gpa);
    mcfg_build(mcfg);
    assert(mcfg->h.length <= 0x1000);

    struct xsdt *xsdt = (struct xsdt *)(guest_ram_vaddr + xsdp->xsdt_gpa);

    memset(xsdt, 0, sizeof(struct xsdt));
    uint64_t xsdt_table_ptrs[XSDT_ENTRIES] = { madt_gpa, hpet_gpa, fadt_gpa, mcfg_gpa };
    xsdt_build(xsdt, xsdt_table_ptrs, XSDT_ENTRIES);

    *acpi_start_gpa = ROUND_DOWN(acpi_top, PAGE_SIZE_4K);
    *acpi_end_gpa = ram_top;

    return xsdp_gpa;
}
