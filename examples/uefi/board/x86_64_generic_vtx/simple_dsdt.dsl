// Copyright 2025, UNSW
// SPDX-License-Identifier: BSD-2-Clause

// Compile with `iasl -tc simple_dsdt.dsl`
// This is used to build the ACPI DSDT. Which is needed for the FADT, and
// the FADT is needed for Linux's ACPI core to initialise and serial IRQ to be discovered.

DefinitionBlock ("", "DSDT", 2, "libvmm", "libvmm", 0x1)
{
    Scope (\_SB)
    {
        // https://github.com/pebble/qemu/blob/master/hw/i386/acpi-dsdt-hpet.dsl
        Device(HPET) {
            Name(_HID, EISAID("PNP0103"))
            Name(_UID, 0)
            OperationRegion(HPTM, SystemMemory, 0xFED00000, 0x400)
            Field(HPTM, DWordAcc, Lock, Preserve) {
                VEND, 32,
                PRD, 32,
            }
            Method(_STA, 0, NotSerialized) {
                Store(VEND, Local0)
                Store(PRD, Local1)
                ShiftRight(Local0, 16, Local0)
                If (LOr(LEqual(Local0, 0), LEqual(Local0, 0xffff))) {
                    Return (0x0)
                }
                If (LOr(LEqual(Local1, 0), LGreater(Local1, 100000000))) {
                    Return (0x0)
                }
                Return (0x0F)
            }
            Name(_CRS, ResourceTemplate() {
                Memory32Fixed(ReadOnly,
                    0xFED00000,         // Address Base
                    0x00000400,         // Address Length
                    )
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

            Name (_CRS, ResourceTemplate ()
            {
                // Bus numbers this root bridge owns
                WordBusNumber (ResourceProducer, MinFixed, MaxFixed, PosDecode,
                    0x0000,         // Granularity
                    0x0000,         // Min
                    0x0000,         // Max
                    0x0000,         // Translation
                    0x0001          // Length
                )

                // I/O port window(s) forwarded to PCI below this root bridge.
                // Precisely host bridge actually decodes.
                WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
                    0x0000,         // Granularity
                    0x0000,         // Min
                    0x0CF7,         // Max
                    0x0000,         // Translation
                    0x0CF8          // Length
                )

                WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
                    0x0000,         // Granularity
                    0x0D00,         // Min
                    0xFFFF,         // Max
                    0x0000,         // Translation
                    0xF300          // Length  (0xFFFF - 0x0D00 + 1)
                )

                // Prefetchable MMIO window
                QWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, Cacheable, ReadWrite,
                    0x0000000000000000, // Granularity
                    0xd0000000, // Min
                    0xd05fffff, // Max
                    0x0000000000000000, // Translation
                    0x600000  // Length
                )

                // Prefetchable MMIO window
                QWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, Cacheable, ReadWrite,
                    0x0000000000000000, // Granularity
                    0xe0000000, // Min
                    0xe01fffff, // Max
                    0x0000000000000000, // Translation
                    0x200000  // Length
                )
            })
        }
    }

    Scope(\_SB) {
        Scope(PCI0) {
            // PCI Routing Table
            Method(_PRT, 0) {
                Return (Package() {
                    // Virtio console:
                    // Device 0x3, function 0, INTA -> GSI 15
                    Package() { 0x00030000, 0, 0, 15 },
                    // Virtio net:
                    // Device 0x4, function 0, INTA -> GSI 14
                    Package() { 0x00040000, 0, 0, 14 },
                    // Virtio blk:
                    // Device 0x5, function 0, INTA -> GSI 13
                    Package() { 0x00050000, 0, 0, 13 },
                })
            }
        }
    }

    Scope(\_SB.PCI0) {

        External(ISA, DeviceObj)

        Device(ISA) {
            Name(_ADR, 0x00010000)


        }
    }

    Scope(\_SB.PCI0.ISA) {
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

        // https://github.com/pebble/qemu/blob/master/hw/i386/acpi-dsdt-isa.dsl
        Device(KBD) {
            Name(_HID, EisaId("PNP0303"))
            Method(_STA, 0, NotSerialized) {
                Return (0x0f)
            }
            Name(_CRS, ResourceTemplate() {
                IO(Decode16, 0x0060, 0x0060, 0x01, 0x01)
                IO(Decode16, 0x0064, 0x0064, 0x01, 0x01)
                IRQNoFlags() { 1 }
            })
        }

        Device(MOU) {
            Name(_HID, EisaId("PNP0F13"))
            Method(_STA, 0, NotSerialized) {
                Return (0x0f)
            }
            Name(_CRS, ResourceTemplate() {
                IRQNoFlags() { 12 }
            })
        }
    }
}
