<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Using multiple virtIO PCI devices with a Linux guest

This example shows off the virtIO support that libvmm provides using the
[seL4 Device Driver Framework (sDDF)](https://github.com/au-ts/sddf) to talk to
the actual hardware.

This example makes use of the following virtIO devices emulated by libvmm:

* console
* block
* network

All of the virtIO devices are emulated with their corresponding native drivers
from sDDF. The guest will interact with the virtIO devices on the virtual PCI bus.

On ARM, this is accomplished by declaring a PCI bus and ECAM window in the device
tree. Linux will detects the PCI bus, probes it, finds then drives our virtIO devices.

On x86, Linux will assume that a PCI bus exists at the legacy I/O Ports so we just
need to place the virtIO devices on the virtual bus.

The example currently works on the following platforms:

* QEMU virt AArch64
* Avnet MaaXBoard
* x86-64 (QEMU only)

### Metaprogram

Unlike the other examples, this one uses a metaprogram (`meta.py`) with
the [sdfgen](https://github.com/au-ts/microkit_sdf_gen) tooling to generate the
System Description File (SDF) and other necessary artefacts. Previously,
SDFs were written manually, along with C headers for sDDF-specific configurations,
but this approach was tedious and error-prone. With this tooling, we can describe
the system at a higher level, automating the generation of system-specific data.

## Dependencies

In addition to the dependencies outlined in the top-level README, the following
dependencies are needed:
* mkfs.fat
* gdisk
* sdfgen (for generating the System Description File with a metaprogram).

Ensure mkfs.fat is in your `$PATH`

### Linux

On apt based Linux distributions run the following commands:
```sh
sudo apt-get install dosfstools gdisk
pip3 install sdfgen==0.35.0
```

If you get error: `externally-managed-environment` when installing via pip, instead run:
```sh
pip3 install --break-system-packages sdfgen==0.35.0
```

This is sound because the `sdfgen` package does not have any dependencies.

### macOS

On macOS, you can install the dependencies via Homebrew:
```sh
brew install dosfstools
pip3 install sdfgen==0.35.0
```

If you get error: `externally-managed-environment` when installing via pip, instead run:
```sh
pip3 install --break-system-packages sdfgen==0.35.0
```

This is sound because the `sdfgen` package does not have any dependencies.

### Nix

There is a Nix flake available in the repository, so you can get a development shell via:
```sh
nix develop
```

Note that this will set the `MICROKIT_SDK` environment variable to the SDK path, you do not
need to download the Microkit SDK manually.

## Building

```sh
make MICROKIT_BOARD=<BOARD> MICROKIT_SDK=/path/to/sdk
```

Where `<BOARD>` is one of:

* `qemu_virt_aarch64`
* `maaxboard`
* `x86_64_generic_vtx`

Other configuration options can be passed to the Makefile such as `MICROKIT_CONFIG`
and `BUILD_DIR`, see the Makefile for details.

By default the build system fetches the Linux kernel and initrd images from
Trustworthy Systems' website. To use your own images, specify `LINUX` and/or
`INITRD`. For example:

```sh
make MICROKIT_BOARD=qemu_virt_aarch64 MICROKIT_SDK=/path/to/sdk LINUX=/path/to/linux INITRD=/path/to/initrd
```

If you would like to simulate the QEMU board you can run the following command:
```sh
make MICROKIT_BOARD=qemu_virt_aarch64 MICROKIT_SDK=/path/to/sdk qemu
```

This will build the example code as well as run the QEMU command to simulate a
system running the whole system.

## Running

The username to login is `root`. There is no password required.

### virtIO console

This example makes use of the virtIO console device so that the guest has access
to the serial device on the platform. The virtIO console support in libvmm talks to
the sDDF serial sub-system which contains a driver for input/output to the physical
serial device.

### virtIO block

The guest also doubles as a client in the block system that talks virtIO to a native
block device. The requests from the guest are multiplexed through the additional block
virtualiser component.

When you boot the example, the native block driver will boot first. When it is ready, the
client VM will boot. After the client VM boots, it will attempt to mount the
virtIO block device `/dev/vda` into `/mnt`.

The kernel logs from linux will show the virtIO drive initialising, for example:
```
[    5.381885] virtio_blk virtio1: [vda] 2040 512-byte logical blocks (1.04 MB/1020 KiB)
```

When you reboot the example, the client VM may display a warning indicating that the
FAT filesystem on the vda device was not cleanly unmounted, which could lead to potential
data corruption:
```
[   12.292600] FAT-fs (vda): Volume was not properly unmounted. Some data may be corrupt. Please run fsck.
```
To prevent this, always shut down the system properly by running `poweroff` after use,
instead of forcefully terminating the VM.

The system expects the storage device to contain an MBR partition table that contains
one partition. Each partition is allocated to a single client. Partitions must have a
starting block number that is a multiple of sDDF block's transfer size of 4096 bytes
divided by the disk's logical size. Partitions that do not follow this restriction
are unsupported.

By default on QEMU virt AArch64, we mount the first partition of the disk image,
on Avnet MaaXBoard we mount the third partition of the SD Card. You can change the partition mounted
by passing `PARTITION=n` when executing the Makefile.

### virtIO net

In addition to virtIO console and block, the guest can also talk with the native
sDDF network driver via virtIO for in-guest networking. Packets in and out of
the guest are multiplexed through the network virtualiser components.

When the guest starts, it will automatically bring up the network device
and obtain an IP address via DHCP. This is done with the `net_client_init`
script that is packaged into the root file system.

To test the guest network, you can try to ping Google DNS with:
```
# ping 8.8.8.8
PING 8.8.8.8 (8.8.8.8): 56 data bytes
64 bytes from 8.8.8.8: seq=0 ttl=255 time=18.560 ms
64 bytes from 8.8.8.8: seq=1 ttl=255 time=8.859 ms
64 bytes from 8.8.8.8: seq=2 ttl=255 time=5.361 ms
64 bytes from 8.8.8.8: seq=3 ttl=255 time=6.902 ms
64 bytes from 8.8.8.8: seq=4 ttl=255 time=9.198 ms
^C
--- 8.8.8.8 ping statistics ---
5 packets transmitted, 5 packets received, 0% packet loss
round-trip min/avg/max = 5.361/9.776/18.560 ms
```

The guest has a DNS resolver so you can also ping a URL.

## aarch64 Hardware Requirements

### QEMU

When running on QEMU, read and writes go to an emulated ramdisk instead of to your
local storage device. The ramdisk file supplied to QEMU is formatted during build
time to contain a FAT filesystem for both partitions.

### Hardware

When running on one of the supported hardware platforms, the system expects to
read and write from the SD card. You will need to format the SD card prior to
booting.

## x86-64 Hardware Requirements

This example only support QEMU target on x86-64.

QEMU's Tiny Code Generator (TCG) emulates ARM's virtualisation extensions, but
not x86's. To run this example you therefore need:

- an x86-64 Intel CPU with virtualisation (VT-x) enabled in your BIOS,
- a Linux host, and
- KVM enabled.

When running on QEMU, read and writes go to an emulated ramdisk instead of to your
local storage device. The ramdisk file supplied to QEMU is formatted during build
time to contain a FAT filesystem for both partitions.

### QEMU troubleshooting

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