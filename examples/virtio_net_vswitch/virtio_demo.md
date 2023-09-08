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

2. Due to the current limitation of the build system, some hacks are used in this system to bypass the configuration, see [more information](https://github.com/au-ts/camkes_to_sel4cp_guide/blob/main/guide.md)

# Usage

Build and run the example:
```
make -B BUILD_DIR=<your build dir> SEL4CP_SDK=<your sdk dir> CONFIG=debug BOARD=qemu_arm_virt qemu
```

# Testing virtIO vswitch

ping VM2 on VM1:
```shell
$ ifconfig eth0 192.168.1.2 & ping 192.168.1.1
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
2. add virtio net
3. support odroidc4

# Reference
1. sel4cp manual: https://github.com/BreakawayConsulting/sel4cp/blob/main/docs/manual.md
