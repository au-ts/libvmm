<!--
     Copyright 2023, UNSW (ABN 57 195 873 179)

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# VirtIO Demo Application

## Intro

<!-- @jade: update the description -->
Documentation about the VirtIO demo systems. The system contains two VMMs that are connected by a vswitch. Both VMs have a passthrough serial. It currently only support QEMU.

This example system is to provide a comparison between VM applications on top of sel4cp VMM and CAmkES VMM, the corresponding example on CAmkES can be found in here:

https://github.com/seL4/camkes-vm-examples/blob/master/apps/Arm/vm_multi

# Issue

1. We don't yet have a serial multiplexor on sel4cp VMM, so only one VM gets to have the passthrough serial device and be able to do serial I/O.

2. Due to the current limitation of the build system, some hacks are used in this system to bypass the configuration, see [more information](../../../docs/camkes_to_cp_guide.md)

# Usage

Build and run the example:
```
make BUILD_DIR=<your build dir> SEL4CP_SDK=<your sdk dir> SYSTEM=virtio_demo.system SEL4CP_CONFIG=debug SEL4CP_BOARD=qemu_arm_virt_hyp run
```

# Testing passthrough block device

creating an image for the passthrough blk device on you own machine:
```shell
$ dd if=/dev/zero of=sel4cp-vmm.img bs=1M count=1000
$ # you might need to do `sudo modprobe loop`
$ losetup -P /dev/loop0 sel4cp-vmm.img
$ fdisk -l /dev/loop0 # and create a partition
$ mkfs -t ext2 /dev/loop0p1
$ losetup -D
```

The additional QEMU configuration for the passthrough blk devide, in case you're interested:
```
-drive id=disk,file=../misc/sel4cp-vmm.img,if=none -device virtio-blk-device,drive=disk
```

mount the disk on VM1:
```shell
$ mount /dev/vda1 /mnt/
$ # do stuff
$ umount /mnt/
```

# Testing virtIO vswitch

ping VM2 on VM1:
```shell
$ ifconfig eth0 192.168.1.2
$ ping 192.168.1.1
```

sample output:
```shell
$ ping 192.168.1.1
PING 192.168.1.1 (192.168.1.1): 56 data bytes
64 bytes from 192.168.1.1: seq=0 ttl=64 time=17.897 ms
64 bytes from 192.168.1.1: seq=1 ttl=64 time=2.995 ms
64 bytes from 192.168.1.1: seq=2 ttl=64 time=3.347 ms
64 bytes from 192.168.1.1: seq=3 ttl=64 time=2.258 ms
64 bytes from 192.168.1.1: seq=4 ttl=64 time=2.982 ms
64 bytes from 192.168.1.1: seq=5 ttl=64 time=2.735 ms
^C
--- 192.168.1.1 ping statistics ---
6 packets transmitted, 6 packets received, 0% packet loss
round-trip min/avg/max = 2.258/5.369/17.897 ms
```

# further work
1. add vhost
2. add virtio net and virtio block
3. convert it to a 3-VMM system

# Reference
1. sel4cp manual: https://github.com/BreakawayConsulting/sel4cp/blob/main/docs/manual.md
