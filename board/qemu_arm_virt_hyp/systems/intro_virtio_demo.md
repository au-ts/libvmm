<!--
     Copyright 2023, UNSW (ABN 57 195 873 179)

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Intro

Documentation about the VirtIO demo systems. The system contains two VMMs that are connected by a vswitch. Both VMs have a passthrough serial.

This example system is to provide a comparison between VM applications on top of sel4cp VMM and CAmkES VMM, the corresponding example on CAmkES can be found in here:

https://github.com/seL4/camkes-vm-examples/blob/master/apps/Arm/vm_multi

A basic understanding of CAmkES and sel4cp might be needed before reading this documentation.

# Comparison

1. CAmkES uses auto-generated code to introduce or parameterise variables for the system (e.g. struct vswitch_mapping, VM_COMPOSITION_DEF, recv1_shmem_size), this is not the case in sel4cp VMM. We don't yet have a proper way to configure this on sel4cp VMM, so many of the variables and structures are hard-coded, this includes:

    - vswitch layouts (connections, MAC address, sharedringbuffer etc.)
    - VM dtb, currently all VMM shares the same dtb file, which makes it harder to do device passthrough and interrupt handling. To bypass this, you might need to hack the Makefile
    - the ability to switch VirtIO backend on and off.
    - other variable names.

    We have not yet addressed this issue as we need more examples to determine what sel4cp actually needs.

2. CAmkES uses different kinds of connectors, such as seL4VirtQueues, to enable communication between components, which also add auto-generated codes into the systems. Some connectors, e.g. seL4VMDTBPassthrough, are unfortunately used as ways to hack the system. In sel4cp it uses channels to enable two protection domains to interact using either protected procedures or notifications.

3. inter-component transporting mechanisms on CAmkES is a virtqueue-like ring buffer library, which requires a corresponding connector(s). sel4cp uses shared ring buffers as well as channel
to achieve a similar functionality but with a way simpler implementation.

4. build systems: TBA

5. This example only cares about the behaviours of VMMs, and does not contain a real device driver (yet). In case you're interested in drivers, there are echo system examples on both [camkes](https://github.com/seL4/camkes/pull/25)(no longer maintained) and [sel4cp](https://github.com/lucypa/sDDF) that might give you and understanding of their differences.

# Issue

We don't yet have a serial multiplexor on sel4cp VMM, so only one VM gets to have the passthrough serial device and be able to do serial I/O.

# Usage

```
make BUILD_DIR=<your build dir> SEL4CP_SDK=<your sdk dir> SYSTEM=virtio_demo.system SEL4CP_CONFIG=debug SEL4CP_BOARD=qemu_arm_virt_hyp run
```

# Testing virtIO vswitch

command:
```
# on vm1
ifconfig eth0 192.168.1.2
ping 192.168.1.1
```

sample output:
```
# ping 192.168.1.1
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
2. CAmkES manual: https://github.com/seL4/camkes-tool/pull/82 (the best I can find)
