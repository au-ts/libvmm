// Copyright 2025, UNSW
// SPDX-License-Identifier: BSD-2-Clause

// Compile with `iasl -tc simple_dsdt.dsl`
// This is used to build the ACPI DSDT. Which is needed for the FADT, and
// the FADT is needed for Linux's ACPI core to initialise and serial IRQ to be discovered.

DefinitionBlock ("", "DSDT", 2, "libvmm", "libvmm", 0x1)
{
    Scope (\_SB)
    {
        Device (COM1)
        {
            Name (_HID, EisaId ("PNP0501"))
            Name (_UID, One)
            Name (_CRS, ResourceTemplate ()
            {
                IO (Decode16, 0x03F8, 0x03F8, 0x00, 0x08)
                IRQNoFlags () { 4 }
            })
        }

        Device (PCI0)
        {
            // PCI root bridge
            Name (_HID, EisaId ("PNP0A03"))
            // Compatible with PCIe root
            Name (_CID, EisaId ("PNP0A08"))
            Name (_UID, One)
            // PCI segment and base bus number 0
            Name (_SEG, Zero)
            Name (_BBN, Zero)

            // @billn add I/O Port ranges that the host bridge decodes,
            // so that we can get rid of "pci=nocrs" in cmdline
        }
    }
}