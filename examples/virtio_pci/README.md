<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Using multiple virtIO (PCI) devices with a Linux guest

This example is the exact same as the virtIO example except
that instead of using MMIO as the backend, we use PCI.

For instructions for building and using, see the virtIO
example's [README](../virtio/README.md).

The main differences between this example and the virtIO example
is the PCI node in the guest Device Tree and the use of different
APIs to setup virtIO PCI devices in the client VMM. In addition,
it supports QEMU x86_64 target (use `x86_64_generic_vtx` board).

## x86_64 Hardware Requirements

QEMU's Tiny Code Generator (TCG) does not emulate Intel's
virtualisation extension on x86_64. Whereas for ARM it does.
Hence to run this example on QEMU, you will need an x86_64 Intel CPU
with virtualisation (VT-x) turned on in your BIOS, a Linux install
and KVM enabled.

## QEMU x86_64 troubleshooting

We use native sDDF virtIO Block and Network drivers for storage
and networking. However, these drivers temporarily rely on
hardcoded physical addresses for their device registers. Because
sDDF does not yet have merged ACPI and PCI drivers to handle
dynamic mapping, the drivers will crash if your QEMU instance
places the registers at unexpected addresses.

(Note: This limitation will be resolved once the ACPI and PCI
drivers are merged into sDDF.)

If you encounter this crash, you must manually retrieve the correct
physical addresses from QEMU and update your configuration.

1. Access the QEMU Monitor
While your QEMU instance is running, press Ctrl + A, release
both keys, and then press c. You will be dropped into the QEMU
monitor prompt:

```
QEMU 11.0.2 monitor - type 'help' for more information
(qemu)
```

2. Retrieve PCI Information
At the prompt, type info pci to list the connected devices
and their memory allocations. You will see an output similar
to this:

```
(qemu) info pci
  Bus  0, device   0, function 0:
    Host bridge: PCI device 8086:1237
      PCI subsystem 1af4:1100
      id ""
  Bus  0, device   1, function 0:
    ISA bridge: PCI device 8086:7000
      PCI subsystem 1af4:1100
      id ""
  Bus  0, device   1, function 1:
    IDE controller: PCI device 8086:7010
      PCI subsystem 1af4:1100
      BAR4: I/O at 0xc0a0 [0xc0af]
      id ""
  Bus  0, device   1, function 3:
    Bridge: PCI device 8086:7113
      PCI subsystem 1af4:1100
      IRQ 9, pin A
      id ""
  Bus  0, device   2, function 0:
    Ethernet controller: PCI device 1af4:1000
      PCI subsystem 1af4:0001
      IRQ 10, pin A
      BAR0: I/O at 0xc080 [0xc09f]
      BAR1: 32 bit memory at 0xfebc0000 [0xfebc0fff]
      BAR4: 64 bit prefetchable memory at 0xfebf8000 [0xfebfbfff]
      BAR6: 32 bit memory (not mapped)
      id ""
  Bus  0, device   3, function 0:
    SCSI controller: PCI device 1af4:1001
      PCI subsystem 1af4:0002
      IRQ 11, pin A
      BAR0: I/O at 0xc000 [0xc07f]
      BAR1: 32 bit memory at 0xfebc1000 [0xfebc1fff]
      BAR4: 64 bit prefetchable memory at 0xfebfc000 [0xfebfffff]
      id ""
(qemu)
```

3. Update meta.py
Locate the Ethernet controller and SCSI controller sections in the
output. For both devices, find the line that reads BAR4: 64 bit
prefetchable memory at....

Take these memory addresses, replace the existing values at the
top of meta.py, and everything will function normally.