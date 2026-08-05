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

### QEMU

QEMU's Tiny Code Generator (TCG) emulates ARM's virtualisation extensions, but
not x86's. To run this example you therefore need:

- an x86_64 Intel CPU with virtualisation (VT-x) enabled in your BIOS,
- a Linux host, and
- KVM enabled.

## QEMU x86_64 troubleshooting

We use native sDDF VirtIO block and network drivers for storage and networking.
These drivers currently rely on hardcoded physical addresses for their device
registers, because sDDF does not yet have merged ACPI and PCI drivers to handle
dynamic mapping. As a result, the drivers crash if your QEMU instance places the
registers at addresses they do not expect. This limitation will disappear once
the ACPI and PCI drivers are merged into sDDF.

The crash looks like this:
```
MON|INFO: Microkit Monitor started!
TIMER DRIVER|ERROR: Invariant TSC not supported, expect performance degradation.
MON|INFO: PD 'timer_driver' is now passive!
BLK DRIVER|ERROR: driver does not support device capacity smaller than 0x1000 bytes (device has capacity of 0x0 bytes)
Failed assertion 'false' at /home/dreamliner7879/TS/libvmm_x86/dep/sddf/drivers/blk/virtio/pci/..//block.c:292 in function virtio_blk_init
MON|ERROR: received message 0x00000003  badge: 0x0000000000000006  tcb cap: 0x000000000000000f
MON|ERROR: faulting PD: blk_driver
Registers:
rip : 0x0000000000204607
rsp : 0x00007fffffffcf90
rflags : 0x0000000000010202
rax : 0x000000000000008b
rbx : 0x0000000000000000
rcx : 0xffffffffffffffff
rdx : 0x000000000000008b
rsi : 0x00007fffffffcf7f
rdi : 0x0000000000000000
rbp : 0x00007fffffffcf90
r8 : 0x0000000000000000
r9 : 0x0000000000000000
r10 : 0x00000000002030d8
r11 : 0x000000000000008b
r12 : 0x0000000000000000
r13 : 0x0000000000000000
r14 : 0x0000000000000000
r15 : 0x0000000000000000
fs_base : 0x0000000000000000
gs_base : 0x0000000000000000
MON|ERROR: UserException
<<seL4(CPU 0) [receiveIPC/153 T0xffffff8001132800 "tcb_monitor" @2001a4]: Reply object already has unexecuted reply!>>
```

If you hit this crash, retrieve the correct physical addresses from QEMU and
update your configuration by hand, as follows.

### 1. Open the QEMU monitor

While QEMU is running, press <kbd>Ctrl</kbd> + <kbd>a</kbd>, in the terminal,
release both keys, then press <kbd>c</kbd>.
You will be dropped at the QEMU monitor prompt:

```
QEMU 11.0.2 monitor - type 'help' for more information
(qemu)
```

### 2. Retrieve the PCI information

At the prompt, type `info pci` to list the connected devices and their memory
allocations. The output will look something like this:

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

### 3. Update `meta.py`

In the Ethernet controller and SCSI controller entries, find the line reading
`BAR4: 64 bit prefetchable memory at ...`. Replace `VIRTIO_NET_PCI_BAR_PADDR` and
`VIRTIO_BLK_PCI_BAR_PADDR` with the addresses your QEMU instance allocated. Check
that `VIRTIO_NET_PCI_IRQ` and `VIRTIO_BLK_PCI_IRQ` match the output too.

Then, in the display controller entry, find the lines reading
`BAR0: 32 bit prefetchable memory at ...` and `BAR2: 32 bit memory at ...`. BAR0
is the framebuffer and BAR2 is the video registers, so replace
`BOCHS_DISPLAY_FB_BAR_PADDR` with BAR0 and `BOCHS_DISPLAY_REGS_BAR_PADDR` with
BAR2.