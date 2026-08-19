<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Revision history for libvmm

## Release 0.1.0 (26/Mar/2025)

This is the first proper release of libvmm, see the README and
manual for getting started and what the library offers. The library
has existed and been used internally by Trustworthy Systems for a
while now but is starting to get used by others so it is time to
start tagging and releasing versions.

For subsequent releases, release notes will be published as well.

## Release 0.2.0 (19/Aug/2026)

This release introduces support for x86-64 VMs, along with many
fixes to ARM virtualisation and the virtIO emulation layer.

### Features

#### ARM

- Support booting a default configured Linux kernel with an initrd image.
- Support guests with multiple cores (VCPUs) via PSCI.
- Hardware-accelerated GIC virtualisation.

#### x86

- ACPI implementation.
- Support booting a default configured Linux kernel with an initrd image.
- Support booting any EFI guest from block storage or PXE via TianoCore OVMF firmware.
- Only support guests with a single VCPU.
- Software virtualised APIC in xAPIC mode and single I/O APIC.

#### Guest-facilities

- VirtIO MMIO and PCI transports.
- Networking via virtIO Network, either through a NIC or Virtual Switch.
- Storage via virtIO Block.
- Console via serial passthrough, virtIO Console or SSH.
