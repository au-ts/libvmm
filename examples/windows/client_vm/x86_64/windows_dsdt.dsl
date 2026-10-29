// Copyright 2026, UNSW
// SPDX-License-Identifier: BSD-2-Clause


DefinitionBlock ("", "DSDT", 2, "libvmm", "libvmm", 0x1)
{
    Scope (\_SB)
    {
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

                // Prefetchable MMIO window
                DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
                    0x0000000000000000, // Granularity
                    0xE0000000, // Min
                    0xEFFFFFFF, // Max
                    0x0000000000000000, // Translation
                    0x10000000  // Length
                )
            })
        }
    }

    Scope(\_SB.PCI0) {
        Device (GSIA) {
            Name (_HID, EisaId("PNP0C0F"))
            Name (_UID, 0)
            Name (_PRS, ResourceTemplate() {
                Interrupt (ResourceProducer, Level, ActiveLow, Shared) { 16 }
            })
            Name (_CRS, ResourceTemplate() {
                Interrupt (ResourceProducer, Level, ActiveLow, Shared) { 16 }
            })
            Method (_DIS, 0, NotSerialized) { }
            Method (_SRS, 1, NotSerialized) { }
        }

        Device (GSIB) {
            Name (_HID, EisaId("PNP0C0F"))
            Name (_UID, 1)
            Name (_PRS, ResourceTemplate() {
                Interrupt (ResourceProducer, Level, ActiveLow, Shared) { 17 }
            })
            Name (_CRS, ResourceTemplate() {
                Interrupt (ResourceProducer, Level, ActiveLow, Shared) { 17 }
            })
            Method (_DIS, 0, NotSerialized) { }
            Method (_SRS, 1, NotSerialized) { }
        }
    }

    Scope(\_SB.PCI0) {
        Method(_PRT, 0) {
            Return (Package() {
                Package() { 0x0004FFFF, 0, GSIA, 0 }, // net
                Package() { 0x0005FFFF, 0, GSIB, 0 }, // blk
            })
        }
    }

    Scope(\_SB.PCI0) {

        External(ISA, DeviceObj)

        Device(ISA) {
            Name(_ADR, 0x00010000)
        }
    }

    Scope(\_SB.PCI0.ISA) {
        Device (COM1) {
            Name (_HID, EisaId ("PNP0501"))
            Name (_UID, 1)
            Name (_CRS, ResourceTemplate ()
            {
                IO (Decode16, 0x03F8, 0x03F8, 0x00, 0x08)
                IRQNoFlags () { 4 }
            })
        }

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