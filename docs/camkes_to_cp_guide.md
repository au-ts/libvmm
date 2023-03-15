<!--
     Copyright 2023, UNSW (ABN 57 195 873 179)

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# CAmkES to seL4CP Transition Guide

## Intro
<!-- @jade: write more stuff -->
This is a guide that aims to provide leads for those who are migrating applications from CAmkES to seL4CP. The guide will be taking two seL4CP applications as examples, and comparing them with similar examples on CAmkES:

- [sDDF(seL4 Device Driver Framework)][sDDF]
    * [similar CAmkES version][camkes sDDF](no longer maintained)

- [VirtIO Demo][VMM]
    * [similar CAmkES version][camkes VMM]

A basic understanding of CAmkES and seL4CP might be needed before reading this documentation.

## Comparison

In general, CAmkES is a more high-level tool compared to seL4CP, it provides mechanisms that seL4CP does not, and should not. On seL4CP, it's up to the user to choose the build systems and the ways to do communication between protection domains. In this section, we will demonstrate the available 1-to-1 mappings between CAmkES and seL4CP, as well as the potential ways to achieve similar features on seL4CP when there are not enough machenisms provided.

To avoid confusion between CAmkES and seL4CP, when referring to seL4CP terminologies, we always emphasize them with italics.

### Overview

Given a simple 2-component CAmkES system (copied from [CAmkES Tutorial][camkes tut] with some modification):
```c {#camkes-overview}
component Ping {
    control;
    dataport Buf(0x1000) sharedbuf_1;
    dataport Buf(0x1000) sharedbuf_2;
    emits Done n1;
    consumes Ready n2;
}

component Pong {
    control;
    dataport Buf(0x1000) sharedbuf_1;
    dataport Buf(0x1000) sharedbuf_2;
    consumes Done n1;
    emits Ready n2;
}

assembly {
    composition {
        component Ping ping;
        component Pong pong;

        connection seL4SharedData channel1(from ping.sharedbuf_1, to pong.sharedbuf_1);
        connection seL4Notification ntfn1(from ping.n1, to pong.n1);

        connection seL4SharedData channel2(from ping.sharedbuf_2, to pong.sharedbuf_2);
        connection seL4Notification ntfn2(from ping.n2, to pong.n2);
    }
}
```

A mostly equivalent seL4CP system of this CAmkES system may look like this:
```xml {#cp-overview}
<?xml version="1.0" encoding="UTF-8"?>

<system>
    <memory_region name="sharedbuf_1" size="0x1_000" />
    <memory_region name="sharedbuf_2" size="0x1_000" />

    <protection_domain name="ping" priority="42">
        <program_image path="ping.elf" />
        <map mr="sharedbuf_1" vaddr="0x3000000" perms="rw" cached="false" setvar_vaddr="send_buf" />
        <map mr="sharedbuf_2" vaddr="0x3001000" perms="r" cached="false" setvar_vaddr="recv_buf" />
    </protection_domain>

    <protection_domain name="pong" priority="42">
        <program_image path="pong.elf" />
        <map mr="sharedbuf_2" vaddr="0x3000000" perms="rw" cached="false" setvar_vaddr="send_buf" />
        <map mr="sharedbuf_1" vaddr="0x3001000" perms="r" cached="false" setvar_vaddr="recv_buf" />
    </protection_domain>

    <channel>
        <end pd="ping" id="0" />
        <end pd="pong" id="0" />
    </channel>

    <channel>
        <end pd="ping" id="1" />
        <end pd="pong" id="1" />
    </channel>
</system>
```

At the system level, there is a roughly 1-to-1 match between ``assembly``/``component`` in CAmkES and *``system``*/*``protection domain``* (*PD*) in seL4CP, but the way CAmkES and seL4CP implement inter-component/*PD* communication and shared memory have diverged.

### Similar Concepts
There are concepts that are mostly equivalent in CAmkES and seL4CP.

#### `assembly`, `composition` and *`system`*
The CAmkES way of defining a system:
```c
assembly {
    composition {
        // your stuff
    }
}
```

and the seL4CP way of defining a system:
```xml
<system>
<!-- also your stuff -->
</system>
```

#### `component` and *`protection domain`*
The CAmkES way of adding a component:
```c
component Foo {
    // cool features
}

assembly {
    composition {
        component Foo foo;
        // your other stuff
    }
}
```

and the seL4CP way of adding a *protection domain*:
```xml
<system>
    <protection_domain name="ping" priority="42">
        <!-- cool features -->
    </protection_domain>

<!-- also your other stuff -->
</system>
```
Unlike components, is mandatory to specify the priority of the PD. You may also want to specify the budget, period etc. here:
```xml {#pd_config}
<protection_domain name="eth" priority="101" budget="160" period="300" pp="true">
    <!-- cool features -->
</protection_domain>
```
Note: Configurating CPU affinity of a *PD* will be added in the future.

The configuration of hardware components however, are quite different between CAmkES and seL4CP, and will be covered in section [Hardwares](#hardwares).

### Distinct Concepts

#### `attributes`, `configuration`
CAmkES allows the programmer to configure custom attributes of components/connectors, such as the details of the device's MMIO and IRQ for a driver component:
```c
component Ethdriver {
    // some cool stuff

    attribute int mmio_paddr = 0xf7cc0000;
    attribute int mmio_size = 0x20000;
    attribute string irq_irq_type = "pci";
    attribute int irq_irq_ioapic = 0;
    attribute int irq_irq_ioapic_pin = 16;
    attribute int irq_irq_vector = 16;
}
```
These attributes are accessible in the application's code.

seL4CP doesn't provide such mechanisms, we will be talking about the alternatives and current walkaround in [Build Systems](#build).
Thread attributes are also considered a type of attributes in CAmkES, e.g. priority, CPU affinity:
```c
assembly {
    composition {
        component Mycomponent c;
    }
    configuration {
        // Run all threads in "c" on CPU 1
        c.priority = 42;
        c.affinity = 1;
    }
}
```
On seL4CP, thread attributes are part of the *PD* attributes (see [this example](#pd_config)).

#### `connector`, `connection`
CAmkES provides various `connectors` for communication(e.g. `seL4VirtQueues`) and shared memory for different purposes. Some connectors (e.g. `seL4VMDTBPassthrough`), are also used as ways to hack the build system. while seL4CP provides strictly minimum support for memory mapping and IPCs/notifications. With seL4CP, it is up to you (at least for now) to implement the mechanisms you need.

With the help of *`memory regions`*, *`channels`* and potentially seL4CP libraries (currently we only have a ready-to-merge [`libsharedringbuffer`][Lucy ShRingBuf] available), you might be able to implement similar features on seL4CP for most of the [standard `connectors`][std conn] and some of the [global `connectors`][glob conn]:

1. `seL4VirtQueues` (more details in section [Inter-component Transporting Mechanisms](#sharedring))
2. `seL4RPCCallSignal`
3. `seL4RPCDataport`
4. `seL4RPCDataportSignal`
5. `seL4RPCOverMultiSharedData`
6. `seL4GlobalAsynch`
7. `seL4GlobalAsynchCallback`
8. `seL4SharedDataWithCaps`
9. `seL4MessageQueue`

A typical `connector` is `seL4RPCDataportSignal`, which combines a `dataport` and a seL4 notification channel. The following example is a common use case of this connector:
```c
procedure DriverInterface {
    // some cool APIs
};

component TheDriver {
    provides DriverInterface client;
}

component TheClient {
    uses DriverInterface driver;
}

assembly {
    composition {
        component TheDriver mydriver;
        component TheClient mycient;
    }
    configuration {
        connection seL4RPCDataportSignal(from myclient.driver, to mydriver.client);
    }
}
```

On seL4CP, the similar functionalities can be implemented like this:
```xml
<?xml version="1.0" encoding="UTF-8"?>

<system>
    <memory_region name="sharedbuf" size="0x1_000" />

    <protection_domain name="mydriver" priority="43">
        <program_image path="mydriver.elf" />
        <map mr="sharedbuf" vaddr="0x3000000" perms="r" cached="false" setvar_vaddr="clientbuf" />
    </protection_domain>

    <protection_domain name="mycient" priority="42">
        <program_image path="mycient.elf" />
        <map mr="sharedbuf" vaddr="0x3000000" perms="w" cached="false" setvar_vaddr="driverbuf" />
    </protection_domain>

    <channel>
        <end pd="mydriver" id="0" />
        <end pd="mycient" id="0" />
    </channel>
</system>
```


<!-- TODO(@jade): last written here -->

Hardware connectors will be covered in section [Hardwares](#hardwares).

#### CAmkES Interface

### Hardwares {#hardwares}
<!-- TODO(@jade): Hardware Components -->
#### Hardware connectors

### Build systems {#build}
CAmkES uses auto-generated code to introduce or parameterise variables for the system (e.g. struct vswitch_mapping, VM_COMPOSITION_DEF, recv1_shmem_size), this is not the case in sel4cp VMM. We don't yet have a proper way to configure this on sel4cp VMM, so many of the variables and structures are hard-coded, this includes:

<!-- TODO(@jade): expand the bullet points -->
<!-- - vswitch layouts (connections, MAC address, sharedringbuffer etc.) -->
- VM dtb, currently all VMM shares the same dtb file, which makes it harder to do device passthrough and interrupt handling. To bypass this, you might need to hack the Makefile
<!-- - the ability to switch VirtIO backend on and off. -->
- make
- import? include?
- memory attributes/allocator

We have not yet addressed this issue as we need more examples to determine what sel4cp actually needs.

### libraries?
1. Runtime API?
2. Synchronization Primitives?

### Inter-component Transporting Mechanisms {#sharedring}
The inter-component transporting mechanisms on CAmkES is a virtqueue-like ring buffer library, which requires a corresponding connector(s). sel4cp uses shared ring buffers as well as channel
to achieve a similar functionality but with a way simpler implementation.
    <!-- TODO(@jade): gives examples (sddf?) -->

### main control loop?
<!-- what is the thread model for sel4cp? -->

### dataports vs. memory mapping?

### Virtualization
1. dtb?
2. passthrough devices?

## Reference
1. [seL4CP manual](https://github.com/BreakawayConsulting/sel4cp/blob/main/docs/manual.md)
2. [CAmkES manual](https://docs.sel4.systems/projects/camkes/manual.html)
3. [libsel4camkes manual](https://github.com/seL4/camkes-tool/pull/82) (the best version I can find)

[sDDF]: https://github.com/lucypa/sDDF/blob/main/echo_server/eth.system
[camkes sDDF]: https://github.com/seL4/camkes/pull/25
[VMM]: https://github.com/Ivan-Velickovic/sel4cp_vmm/blob/jade/virtio_mmio/board/qemu_arm_virt_hyp/systems/virtio_demo.system
[camkes VMM]: https://github.com/seL4/camkes-vm-examples/blob/master/apps/Arm/vm_multi
[camkes tut]: https://docs.sel4.systems/projects/camkes/manual.html#an-example-of-dataports
[Lucy ShRingBuf]: https://github.com/lucypa/sDDF/tree/main/echo_server/libsharedringbuffer
[std conn]: https://github.com/seL4/camkes-tool/blob/master/include/builtin/std_connector.camkes
[glob conn]: https://github.com/seL4/global-components/blob/master/interfaces/global-connectors.camkes