<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Using multiple virtIO devices with a Linux guest

This example shows off the virtIO support that libvmm provides using the
[seL4 Device Driver Framework](https://github.com/au-ts/sddf) to talk to the
actual hardware.

This example makes use of the following virtIO devices emulated by libvmm:
* console
* block
* network

All of the virtIO devices are emulated with their corresponding native drivers from sDDF.

The example currently works on the following platforms:
* QEMU virt AArch64
* Avnet MaaXBoard

## Dependencies

In addition to the dependencies outlined in the top-level README, the following dependencies
are needed:
* Zig (for cross-compiling for Linux user-space),
* mkfs.fat, and
* sdfgen (for generating the System Description File with a metaprogram).

On all platforms, `sdfgen` can be installed with:
```shell
pip3 install sdfgen
```
Or alternatively with a virtual environment:
```shell
python3 -m venv my_venv
source ./my_venv/bin/activate
pip3 install sdfgen
```

### Linux

Zig can be installed manually or via a package manager, see
[here](https://github.com/ziglang/zig/wiki/Install-Zig-from-a-Package-Manager).

You may already have `mkfs.fat`, if not and you use `apt` try the following command:
```sh
sudo apt-get install dosfstools
```

### macOS

Zig can be installed manually or via a package manager, see
[here](https://github.com/ziglang/zig/wiki/Install-Zig-from-a-Package-Manager).

To install `mkfs.fat` run:
```sh
brew install dosfstools
```

### Nix

The top-level `shell.nix` has everything necessary:
```sh
nix-shell ../../shell.nix
```

## Building

```sh
make MICROKIT_BOARD=<BOARD> MICROKIT_SDK=/path/to/sdk
```

Where `<BOARD>` is one of:
* `qemu_virt_aarch64`
* `maaxboard`

Other configuration options can be passed to the Makefile such as `CONFIG`
and `BUILD_DIR`, see the Makefile for details.

If you would like to simulate the QEMU board you can run the following command:
```sh
make MICROKIT_BOARD=qemu_virt_aarch64 MICROKIT_SDK=/path/to/sdk qemu
```

This will build the example code as well as run the QEMU command to simulate a
system running the whole system.

### Metaprogram
Unlike the other examples, this one uses a metaprogram (meta.py) with microkit_sdf_gen 
to generate the System Description File (SDF) and other necessary artefacts. Previously, 
SDFs were written manually, along with C headers for sDDF-specific configurations, but 
this approach was tedious and error-prone. By leveraging microkit_sdf_gen, we can describe 
the system at a higher level, automating the generation of system-specific data for driver 
subsystems and reducing the complexity of assembling more advanced systems.

## Running

### virtIO console

This example makes use of the virtIO console device so that the guest has access
to any serial device on the platform. The virtIO console support in libvmm talks to
a serial multiplexor which then talks to a driver for input/output to the physical
serial device.

When you boot the example, you will see different coloured output for the guest.
Initially all input is defaulted to guest 1 in red.

### virtIO block

The guest also doubles as a client in the block system that talks virtIO to a native
block device. The requests from the guest are multiplexed through the additional block
virtualiser component.

When you boot the example, the native block driver will boot first. When it is ready, the
client VM will boot. After the client VM boot, they will attempt to mount the
virtIO block device `/dev/vda` into `/mnt`. The kernel logs from linux will show the
virtIO drive initialising.
```
[    5.381885] virtio_blk virtio1: [vda] 2040 512-byte logical blocks (1.04 MB/1020 KiB)
```

When you reboot the example, the client VM may display a warning indicating that the
FAT filesystem on the vda device was not cleanly unmounted, which could lead to potential
data corruption:
```
[   12.292600] FAT-fs (vda): Volume was not properly unmounted. Some data may be corrupt. Please run fsck.
```
To prevent this, always shut down the system properly by running poweroff after use,
instead of forcefully terminating the VM.

The system expects the storage device to contain an MBR partition table that contains
one partition. Each partition is allocated to a single client. Partitions must have a
starting block number that is a multiple of sDDF block's transfer size of 4096 bytes
divided by the disk's logical size. Partitions that do not follow this restriction
are unsupported.

### virtIO net
In addition to virtIO console and block, the guest can also talk with the native sDDF network
driver via virtIO for in-guest networking. Packets in and out of the guest are multiplexed through
the network virtualiser components.

When the guest boots up, you must bring up the network device. First check the name of the network device,
it should be called `eth0`:
```
# ip link show
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
2: eth0: <BROADCAST,MULTICAST> mtu 1500 qdisc noop qlen 1000
    link/ether 52:54:01:00:00:fd brd ff:ff:ff:ff:ff:ff
```

Then bring up the device:
```
ip link set eth0 up
```

Before you can talk on the network, you need an IP address. To obtain an IP address, initiate DHCP with:
```
# udhcpc
```

Now the guest network, you can try to ping Google DNS with:
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

The minimal kernel also comes with a DNS resolver so you can ping a URL too.


### QEMU set up
When running on QEMU, read and writes go to an emulated ramdisk instead of to your
local storage device. The ramdisk file supplied to QEMU is formatted during build
time to contain a FAT filesystem for both partitions.

### Hardware set up

When running on one of the supported hardware platforms, the system expects to read
and write from the SD card. You will need to format the SD card prior to booting.

