<!--
     Copyright 2023, UNSW (ABN 57 195 873 179)

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# CAmkES to seL4CP Transition Guide

## Intro

This is a document that aims to provide advice for those who are migrating applications from CAmkES to seL4CP. A basic understanding of CAmkES and seL4CP might be needed before reading this documentation. Most of the examples in this guide are taken from the [CAmkES Tutorial][camkes tut] and the following example systems:

- [sDDF(seL4 Device Driver Framework)][sDDF]
    * [similar CAmkES version][camkes sDDF](no longer maintained)

- [VirtIO Demo][VMM]
    * [similar CAmkES version][camkes VMM]

In general, CAmkES is a higher-level tool than seL4CP. It provides features that seL4CP does not intend to provide, such as build systems and connectors. seL4CP is simple, it intentionally doesn't prescribe a boiled system but provides an SDK that is designed to integrate with a build system of your choice. This guide will demonstrate the available 1-to-1 mappings between CAmkES concepts and seL4CP concepts, as well as potential ways to achieve similar results on seL4CP where seL4CP does not provide a feature.

To avoid confusion between CAmkES and seL4CP, when referring to seL4CP terminologies, we always emphasize them with *italics*.

## Overview

Given a simple 2-component CAmkES system:
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

At the system level, there is a roughly 1-to-1 match between ``assembly``/``component`` in CAmkES and *``system``*/*``protection domain``* (*PD*) in seL4CP, but the way CAmkES and seL4CP implement inter-component/*PD* communication and shared memory are different.

## Concepts

### `assembly`, `composition` and *`system`*
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

### `component` and *`protection domain`*
The CAmkES way of adding a component:
```c
component Foo {
    control;
    // cool features
}

assembly {
    composition {
        component Foo foo;
        // your other stuff
    }
}
```
A CAmkES component are typically multithreaded, the keyword `control` means it will contain a main function and have an active thread of control. Each `interface` (see [CAmkES Interfaces](#iface)) the component interacts with induces another thread within the component.

seL4CP *PDs* are single-threaded and event-driven to keep the programming model and implementations simple. A *PD* provides one single thread of control.

The seL4CP way of adding a *protection domain*:
```xml
<system>
    <protection_domain name="ping" priority="42">
        <!-- cool features -->
    </protection_domain>

<!-- also your other stuff -->
</system>
```

Unlike components, it is mandatory to specify the priority of the *PD*. You may also want to specify other thread attributes, including *`passive`*. A *PD* of a passive server may look like this:

```xml {#pd_config}
<protection_domain name="eth" priority="101" budget="160" period="300" passive="true" pp="true">
    <!-- cool features -->
</protection_domain>
```

budget and period are thread attributes for the scheduler of the MCS kernel. On camkes, you should be able to configure them as attributes (see [thread attributes](#thread_attr)).

*`passive`* means this is an event-driven *protection domain*, that has no continuously running thread of its own. After it has initialised itself, its scheduling context is revoked, and it runs on the scheduling contexts of its invokers or a notification.

*`pp`* means this component has a *protected procedure call* entry point.

Note: The ability to configure the CPU affinity of a *PD* will be added in the future.

The configuration of hardware components however, will be covered in section [Hardware](#hardware).

### `attributes`, `configuration` {#attributes}
CAmkES allows the programmer to configure custom attributes of components/connectors, such as the details of the device's MMIO and IRQ for a driver component. Those attributes are accessible as global variables in the application's code.
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

seL4CP doesn't provide such a mechanism, the alternatives and current workaround is discussed in [Build](#build).

Thread attributes (e.g., priority, budget, period CPU affinity) are also considered a type of attribute in CAmkES:
```c {#thread_attr}
assembly {
    composition {
        component Mycomponent c;
    }
    configuration {
        c.priority = 42;
        c.affinity = 1;
    }
}
```
On seL4CP, thread attributes are part of the *PD* attributes (see [this example](#pd_config)).

### `connectors`, `connections` and *`memory regions`*, *`channels`*

CAmkES provides various `connectors` for communication (e.g., `seL4VirtQueues`) and shared memory (e.g., `seL4RPCDataport`) for different purposes. There are also connectors for specific types of servers (e.g., `seL4Ethdriver`). Some connectors (e.g., `seL4VMDTBPassthrough`), are even used as ways to hack the build system. In contrast, seL4CP provides strictly minimum supports for memory mapping with *`memory regions`* and IPCs/notifications with *`channels`*. With seL4CP, it is up to you to implement the mechanisms you need.

With the help of *`memory regions`*, *`channels`* and potentially libraries that run on top of seL4CP (currently we only have a ready-to-merge [`libsharedringbuffer`][Lucy ShRingBuf] available), you might be able to implement similar features on seL4CP for most of the [standard `connectors`][std conn] and some of the [global `connectors`][glob conn].

An example of a global `connector` is `seL4RPCDataportSignal`, which combines a `dataport` and a seL4 notification channel. The following example is a common use case of this connector:
```c
procedure DriverInterface {
    // some cool APIs
};

component TheDriver {
    provides DriverInterface client;
}

component TheClient {
    control;
    uses DriverInterface driver;
}

assembly {
    composition {
        component TheDriver mydriver;
        component TheClient myclient;
    }
    configuration {
        connection seL4RPCDataportSignal(from myclient.driver, to mydriver.client);
    }
}
```

On seL4CP, a similar functionality can be implemented thus:
```xml
<?xml version="1.0" encoding="UTF-8"?>

<system>
    <memory_region name="sharedbuf" size="0x1_000" />

    <protection_domain name="mydriver" priority="43">
        <program_image path="mydriver.elf" />
        <map mr="sharedbuf" vaddr="0x3000000" perms="r" cached="false" setvar_vaddr="clientbuf" />
    </protection_domain>

    <protection_domain name="myclient" priority="42">
        <program_image path="myclient.elf" />
        <map mr="sharedbuf" vaddr="0x3000000" perms="w" cached="false" setvar_vaddr="driverbuf" />
    </protection_domain>

    <channel>
        <end pd="mydriver" id="0" />
        <end pd="myclient" id="0" />
    </channel>
</system>
```
`sharedbuf` is a contiguous range of physical memory that's mapped into both *PDs* with the specified virtual address, caching attributes and permissions. The symbols `clientbuf` and `driverbuf` will be bound to the value `0x3000000` and are accessible in the application's code.

You can also set the physical address and the page size for a *`memory region`*:
```xml
<!-- physical memory for UART -->
<memory_region name="serial" size="0x1_000" phys_addr="0x9000000" />

<!-- RAM for a VM guest -->
<memory_region name="guest_ram" size="0x10_000_000" page_size="0x200_000" />
```
These are particularly useful for VMM and driver development.

Hardware connectors will be covered in [Hardware](#hardware).

### CAmkES Interfaces and *`channels`* {#iface}
CAmkES provides interfaces as abstract exposed interaction points of a component. Examples of interfaces are `dataports` (port interface), `emits`/`uses` (event interface) and `Ethdriver` (procedure interface). CAmkES sDDF makes use of the `Ethdriver` interface:
```c
// the definition of `Ethdriver` in Ethdriver.idl4
procedure Ethdriver {
    int tx(in int len);
    int rx(out int len);
    void mac(out uint8_t b1, out uint8_t b2, out uint8_t b3, out uint8_t b4, out uint8_t b5, out uint8_t b6);
};
```

```c
// code segments from CAmkES sDDF
assembly {
    composition {
        // some unrelated stuff

        // the connection of two interfaces
        connection seL4Ethdriver eth_driver_conn(from lwipserver.ethdriver, to ethdriver.client);
    }
}
```

This gives component `lwipserver` the ability to transmit data to component `ethdriver` by calling `ethdriver_tx`. On seL4CP, however, there are no built-in mechanisms for Ethdrivers (or any other kind of drivers). What you might do instead, is to register *channels* for all methods you need:
```xml
<system>
    <!-- some cool configs for shared buffers -->

    <protection_domain name="eth" priority="101" budget="160" period="300" pp="true">
        <!-- some cool configs for shared buffer mappings -->
    </protection_domain>

    <protection_domain name="client" priority="100" budget="20000">
        <!-- some cool configs for shared buffer mappings -->
    </protection_domain>

    <!-- RX -->
    <channel>
        <end pd="eth" id="0" />
        <end pd="client" id="0" />
    </channel>
    <!-- TX -->
    <channel>
        <end pd="client" id="1" />
        <end pd="eth" id="1" />
    </channel>
    <!-- get MAC -->
    <channel>
        <end pd="client" id="2" />
        <end pd="eth" id="2" />
    </channel>
</system>
```

And have a handler that handles the notifications/calls for the receiver:
```c
// in eth driver's PD
void notified(sel4cp_channel channel_id)
{
    switch(channel_id) {
        case 1:
            // handles TX
            return;
        case 2:
            // handles get mac
            return;
        default:
            // complains
            break;
    }
}

// in client's PD
void notified(sel4cp_channel channel_id)
{
    switch(channel_id) {
        case 0:
            // handles RX
            return;
        default:
            // complains
            break;
    }
}
```
Port interfaces and event interfaces can also be implemented in similar ways.

## Hardware {#hardware}

A hardware component represents an interface to hardware in the form of a component. A hardware component may look like this:
```c
component Device {
  hardware;

  provides IOPort io_port; // IA32
  emits Interrupt irq;
  dataport Buf mem;
}
```

seL4CP doesn't have an abstraction for hardware. To allow a *PD* to access a particular device, the physical memory of the device needs to be mapped into the *PD*. You may also need to register the IRQ for it.
```xml
<system>
    <memory_region name="device_mem" size="0x10_000" phys_addr="0x30890000" />
    <memory_region name="device_ioport" size="0x200" phys_addr="0x308a0000" />
    <!-- other cool stuff -->

    <protection_domain name="driver" priority="42">
        <map mr="device_mem" vaddr="0x30890000" perms="rw" cached="false" setvar_vaddr="device_base" />
        <map mr="device_ioport" vaddr="0x308a0000" perms="rw" cached="false" setvar_vaddr="device_ioport" />

        <irq irq="142" id="7" />
    </protection_domain>
    <!-- other cool stuff -->
</system>
```

To handle the IRQ, add an entry in the handler for *channels*.
```c
void notified(sel4cp_channel channel_id)
{
    switch(channel_id) {
        case 7:
            // handle interrupt
            return;

        // other channels

        default:
            // complains
            break;
    }
}
```

## Build {#build}

seL4CP is build-system-agnostic, so setting up build flags as it is done on CAmkES using CMake is up to the build system you choose for your project.

As we mentioned in [`attributes`, `configuration`](#attributes), CAmkES uses `attributes` that result in symbols being defined and available to the source of various components.  For example, in CAmkES VMM development, `attributes` are used heavily for describing the layout of the whole system and the features of each VMM:
```c
configuration {
    // telling the vswitch backend about the connections that VM0 likes to have
   vm0.vswitch_layout = [{"mac_addr": "02:00:00:00:AA:02", "recv_id": 0, "send_id":1},
                        {"mac_addr": "02:00:00:00:AA:03", "recv_id": 2, "send_id":3}];

   // telling the VMM what init functions should be call during the initialisation
   vm0.init_cons = [
   {"init":"make_virtio_blk"},
   {"init":"make_virtio_con_driver_dummy", "badge":"recv1_notification_badge()", "irq":"handle_serial_console"},
   {"init":"make_virtio_con"},
   ];
}
```
seL4CP does not have a standard build system. In existing seL4CP example systems, most of these variables and structures are hard-coded. In addition, for parameters in the `.system` file, such as *channel* IDs and IRQs, it is up to you to ensure that they match those specified in the application's code. It would be possible to have external build tools as a solution in the future, but more use cases are needed to determine what seL4CP actually needs to provide.

On CAmkES, you are also able to import components from other files or include header files for your `.camkes` file, which may not be necessary for seL4CP systems.

## Libraries

### Standard C library

Standard C library functionality is available on CAmkES, but seL4CP does not enforce any particular C library, or even the use of C as an implementation language.

### Synchronization Primitives

CAmkES provides primitives for intra-component synchronization, e.g.:
```c
/* Lock mutex m */
int m_lock(void);

/* Unlock mutex m */
int m_unlock(void);
```
which is not needed on seL4CP as *PDs* are single-threaded.

There are no built-in inter-component synchronisation primitives for either CAmkES or seL4CP as seL4 system calls are sufficient.

### Allocator

CAmkES provides a [DMA allocator][dma alloc] to allocate and manage DMA buffers from a DMA pool, which is configured with the `dma_pool` attribute in CAmkES components. It also provides a [seL4 capability object allocator][obj alloc] that can be used to allocate seL4 capability objects from a managed pool. These tools are not available on seL4CP.

Dynamic memory allocation in CAmkES is done by standard C library functions, which manipulate a static array that has been set up by CAmkES. The size of the array can be configured with the `heap_size` attribute in CAmkES components. On seL4CP, you can configure a *`memory region`* for this purpose, but you will also need to implement any allocators you need (at least for now).

### Inter-component Transport Mechanisms {#sharedring}

The inter-component transport mechanisms on CAmkES are a virtqueue-like [ring buffer library][camkes vq] and the corresponding connectors `VirtQueueInit` and `seL4VirtQueues`. The mechanisms are commonly used in client-server systems:
```c
component Comp {
    uses VirtQueueDev recv;
    uses VirtQueueDrv send;
}

assembly {
    composition {
        component Comp mydriver;
        component Comp myclient;

        component VirtQueueInit vqinit0;
        component VirtQueueInit vqinit1;

        connection seL4VirtQueues virtq_conn0(to vqinit0.init, from mydriver.send, from myclient.recv);
        connection seL4VirtQueues virtq_conn1(to vqinit1.init, from mydriver.recv, from myclient.send);
    }

    configuration {
        // some configuration for the vq size etc.
    }
}
```
There is an external library [ring buffer library][Lucy ShRingBuf] available for seL4CP that achieves the same goal. An example configuration for the ring buffers looks like this:
```xml
<system>
    <!-- shared memory for ring buffer mechanism -->
    <memory_region name="avail-1" size="0x200_000" page_size="0x200_000"/>
    <memory_region name="used-1" size="0x200_000" page_size="0x200_000"/>
    <memory_region name="avail-2" size="0x200_000" page_size="0x200_000"/>
    <memory_region name="used-2" size="0x200_000" page_size="0x200_000"/>

    <memory_region name="shared_dma-1" size="0x200_000" page_size="0x200_000" />
    <memory_region name="shared_dma-2" size="0x200_000" page_size="0x200_000" />

    <protection_domain name="mydriver" priority="43">
        <program_image path="mydriver.elf" />

        <map mr="avail-1" vaddr="0x50000000" perms="rw" cached="true" setvar_vaddr="rx_avail" />
        <map mr="used-1" vaddr="0x5200000" perms="rw" cached="true" setvar_vaddr="rx_used" />
        <map mr="avail-2" vaddr="0x5400000" perms="rw" cached="true" setvar_vaddr="tx_avail" />
        <map mr="used-2" vaddr="0x5600000" perms="rw" cached="true" setvar_vaddr="tx_used" />

        <map mr="shared_dma-1" vaddr="0x5800000" perms="rw" cached="true" setvar_vaddr="rx_shared_dma_vaddr" />
        <map mr="shared_dma-2" vaddr="0x5a00000" perms="rw" cached="true" setvar_vaddr="tx_shared_dma_vaddr" />
    </protection_domain>

    <protection_domain name="mycient" priority="42">
        <program_image path="mycient.elf" />

        <map mr="avail-2" vaddr="0x50000000" perms="rw" cached="true" setvar_vaddr="rx_avail" />
        <map mr="used-2" vaddr="0x5200000" perms="rw" cached="true" setvar_vaddr="rx_used" />
        <map mr="avail-1" vaddr="0x5400000" perms="rw" cached="true" setvar_vaddr="tx_avail" />
        <map mr="used-1" vaddr="0x5600000" perms="rw" cached="true" setvar_vaddr="tx_used" />

        <map mr="shared_dma-2" vaddr="0x5a00000" perms="rw" cached="true" setvar_vaddr="rx_shared_dma_vaddr" />
        <map mr="shared_dma-1" vaddr="0x5800000" perms="rw" cached="true" setvar_vaddr="tx_shared_dma_vaddr" />
    </protection_domain>

    <channel>
        <end pd="mydriver" id="0" />
        <end pd="mycient" id="0" />
    </channel>
</system>
```
A good example of how to set up and use the ring buffer is [sDDF][sDDF].

## Reference
1. [seL4CP manual](https://github.com/BreakawayConsulting/sel4cp/blob/main/docs/manual.md)
2. [CAmkES manual](https://docs.sel4.systems/projects/camkes/manual.html)
3. [libsel4camkes manual](https://github.com/seL4/camkes-tool/pull/82) (the best version I can find)

[camkes tut]: https://docs.sel4.systems/projects/camkes/manual.html
[sDDF]: https://github.com/lucypa/sDDF/blob/main/echo_server/eth.system
[camkes sDDF]: https://github.com/seL4/camkes/pull/25
[VMM]: https://github.com/Ivan-Velickovic/sel4cp_vmm/blob/jade/virtio_mmio/board/qemu_arm_virt_hyp/systems/virtio_demo.system
[camkes VMM]: https://github.com/seL4/camkes-vm-examples/blob/master/apps/Arm/vm_multi
[Lucy ShRingBuf]: https://github.com/lucypa/sDDF/tree/main/echo_server/libsharedringbuffer
[std conn]: https://github.com/seL4/camkes-tool/blob/master/include/builtin/std_connector.camkes
[glob conn]: https://github.com/seL4/global-components/blob/master/interfaces/global-connectors.camkes
[dma alloc]: https://github.com/seL4/camkes-tool/blob/master/libsel4camkes/include/camkes/dma.h
[obj alloc]: https://github.com/seL4/camkes-tool/blob/master/libsel4camkes/include/camkes/allocator.h
[camkes vq]: https://github.com/seL4/camkes-tool/blob/master/libsel4camkes/include/camkes/virtqueue.h
