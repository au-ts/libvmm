<!--
     Copyright 2025, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Using multiple virtIO devices with Linux guests

This example shows off the multiple guests and virtIO support that
libvmm provides using the
[seL4 Device Driver Framework (sDDF)](https://github.com/au-ts/sddf)
to talk to the actual hardware.

This example makes use of the following virtIO devices emulated by libvmm:
* console
* block
* network
* socket (also known as 'vsock' and does not interact with any real hardware)

Note that while we demo all virtIO devices together in one unified example system.
THeir implementation are independant of each others and thus can be deployed
independantly.

All of the virtIO devices (except socket) are emulated with their corresponding
native drivers from sDDF.

The example currently works on the following platforms:
* QEMU virt AArch64
* Avnet MaaXBoard

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
* sdfgen (for generating the System Description File with a metaprogram).

### Linux
<!-- TODO bump sdfgen ver once vsock PR in sdfgen is merged -->
On apt based Linux distributions run the following commands:
```sh
sudo apt-get install dosfstools
pip3 install sdfgen==0.23.1
```

If you get error: `externally-managed-environment` when installing via pip, instead run:
```sh
pip3 install --break-system-packages sdfgen==0.23.1
```

### macOS

On macOS, you can install the dependencies via Homebrew:
```sh
brew install dosfstools
pip3 install sdfgen==0.23.1
```

If you get error: `externally-managed-environment` when installing via pip, instead run:
```sh
pip3 install --break-system-packages sdfgen==0.23.1
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

### virtIO console

This example makes use of the virtIO console device so that guests has access
to the serial device on the platform. The virtIO console support in libvmm talks to
the sDDF serial sub-system which contains a driver for input/output to the physical
serial device.

Since only 1 guest can read from the console at any given time, by default console
reading is granted to the first guest (guest #0). You can switch the console input
by pressing `Ctrl` + `\`, then the guest's index (`0` or `1` in this example).

### virtIO block

The guests also doubles as clients in the block system that talks virtIO to a native
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

By default on QEMU virt AArch64, we mount the first and second partitions of the disk image to
guest #0 and #1 respectively. On Avnet MaaXBoard we mount the second and third partitions
of the SD Card. You can change the partition mounted by passing `PARTITION=n` when executing the Makefile.

### virtIO net

In addition to virtIO console and block, the guests can also talk with the native
sDDF network driver via virtIO for in-guest networking. Packets in and out of
the guests are multiplexed through the network virtualiser components.

When the guests boots up, you must bring up the network device. First check the
name of the network device, it should be called `eth0`:
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

Before you can talk on the network, you need an IP address.
To obtain an IP address, initiate DHCP with:
```
udhcpc
```

Now the guest can talk on the network, you can try to ping Google DNS with:
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

### virtIO socket
The virtIO socket device allow inter-guests communication without Ethernet
or IP protocols. In this example, the two guests act as a sender as receiver
with corresponding user-level programs packaged into the ramdisk. Guest #0
and #1 are allocated CID 3 and 4 respectively.

```
--------- VM ---------            --------- VM ---------
Userland:                         Userland:
    vsock_recv                        vsock_send
    ^                                 |
    |                                 v
Kernel:                           Kernel:
    virtio MMIO vsock driver          virtio MMIO vsock driver
    ^                                 |
    |                                 v
--------- VMM --------            --------- VMM --------
virtio vsock device           /-- virtio vsock device
    ^                        /
    |                       /
rx buffer <----------------/      rx buffer
```

In sender VM, `vsock_send` will send 32k worth of data through virtIO socket
to `vsock_recv` in the receiver VM. Then, the receiver VM will verify that the
data are all correct and both programs will quit.

Each virtIO socket device have a small receive buffer that the peer can write to
to send data. The buffer size is advertised to both the sender and receiver via the
`buf_alloc` field of the packet header for the guest driver to split up packets
as necessary.

Here is a demo of this process happening. Red (-) is guest #0 while
green (+) is guest #1:
```diff
+ Welcome to Buildroot
+ buildroot login: 
- Welcome to Buildroot
- buildroot login: root
- login[262]: root login on 'console'
- # ./vsock_recv 3
- VSOCK RECV|INFO: starting
- VSOCK RECV|INFO: creating socket to wait on CID 3
VIRT_RX|LOG: switching to client 1

+ Welcome to Buildroot
+ buildroot login: root
+ login[260]: root login on 'console'
+ # ./vsock_send 3
+ VSOCK SEND|INFO: starting
+ VSOCK SEND|INFO: creating socket to send on CID 3
- VSOCK RECV|INFO: peer connected
+ VSOCK SEND|INFO: connected, preparing payload
+ VSOCK SEND|INFO: now sending 32768 bytes!
- VSOCK RECV|INFO: Accumulatively received 4050 bytes
- VSOCK RECV|INFO: Accumulatively received 8100 bytes
- VSOCK RECV|INFO: Accumulatively received 12150 bytes
- VSOCK RECV|INFO: Accumulatively received 16200 bytes
- VSOCK RECV|INFO: Accumulatively received 20250 bytes
- VSOCK RECV|INFO: Accumulatively received 24300 bytes
- VSOCK RECV|INFO: Accumulatively received 28350 bytes
- VSOCK RECV|INFO: Accumulatively received 32400 bytes
- VSOCK RECV|INFO: Total bytes received 32768, verifying data...
- VSOCK RECV|INFO: All is well in the universe
```

### QEMU set up

When running on QEMU, read and writes go to an emulated ramdisk instead of to your
local storage device. The ramdisk file supplied to QEMU is formatted during build
time to contain a FAT filesystem for both partitions.

### Hardware set up

When running on one of the supported hardware platforms, the system expects to
read and write from the SD card. You will need to format the SD card prior to
kbooting.
