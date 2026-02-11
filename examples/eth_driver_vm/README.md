<!--
     Copyright 2025, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Ethernet Driver VM Example

This example demonstrates an Ethernet driver VM running with sDDF's echo server on the Odroid C4. It is
a shallow copy that only contains files that needs to be modified to accomodate the Ethernet driver
VM.

The only files that we copied from the echo server is `echo.mk` and `meta.py` to accomodate the VM.

The idea is we have a userspace program in Linux that converts sDDF's network TX/RX into
corresponding syscalls in a Linux raw socket.
- The sDDF network control, data regions and notification from virtualisers are mapped into Linux
  userspace via UIO.
- A promiscuous socket is opened to sniff all incoming frames. When a frame comes through from the
  socket, we enqueue the data into the appropriate sDDF queues and fault on a pre-determined address
  so the VMM can notify the RX virtualiser.
- When a client wants to transmit data, we will get TX virt notifications via UIO, then we simply
  write the data out into the socket.

Even though we already have a native Ethernet driver for Odroid C4, this example is still useful as
it could be used to provide Ethernet driver VM on new boards or further developed into a
WiFi/Cellular/Bluetooth driver VM.

The example currently works on the following platforms:
* HardKernel Odroid-C4

Things to note when using the network UIO driver:
- You need to map in the control queues as uncached in the guest and native virtualisers. This is
  because Linux treats UIO regions as device memory thus map them uncached to userspace.

## Compilation
To compile this example, run:
```
make MICROKIT_SDK=<absolute path to SDK> MICROKIT_BOARD=odroidc4 MICROKIT_CONFIG=release
```

### Boot
On a successful boot, your console will look like this:
```
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: eth0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc mq qlen 1000
    link/ether 00:1e:06:4a:35:e4 brd ff:ff:ff:ff:ff:ff
[    3.716293] meson8b-dwmac ff3f0000.ethernet eth0: Link is Up - 1Gbps/Full - flow control off
[    5.753497] random: crng init done
UIO(NET): *** Starting up
UIO(NET): *** Network interface: eth0
UIO(NET): *** Setting up raw promiscuous socket
[   10.669821] meson8b-dwmac ff3f0000.ethernet eth0: entered promiscuous mode
UIO(NET): *** Creating epoll
UIO(NET): *** Binding socket to epoll
UIO(NET): *** Setting up data passing page via UIO
UIO(NET): -> mapped device 'uio0' with size 0x1000 @ 0xffff8454d000
UIO(NET): -> rx data paddr = 0x5feff000
UIO(NET): -> tx client #0 data paddr = 0x5fdff000
UIO(NET): -> tx client #1 data paddr = 0x5fcff000
UIO(NET): *** Setting up in sDDF net control queues
UIO(NET): -> mapped device 'uio1' with size 0x3000 @ 0xffff8454a000
UIO(NET): -> mapped device 'uio3' with size 0x3000 @ 0xffff84547000
UIO(NET): -> mapped device 'uio2' with size 0x5000 @ 0xffff84542000
UIO(NET): -> mapped device 'uio4' with size 0x5000 @ 0xffff8453d000
UIO(NET): *** Setting up TX/RX fault mechanism for guest -> VMM notifications
UIO(NET): -> mapped device 'uio5' with size 0x1000 @ 0xffff8453c000
UIO(NET): -> mapped device 'uio6' with size 0x1000 @ 0xffff8453b000
UIO(NET): *** Setting up TX/RX UIO IRQs for VMM -> guest notifications
UIO(NET): *** Binding UIO TX and RX incoming interrupts to epoll
UIO(NET): *** Setting up in sDDF net data queues
UIO(NET): -> mapped device 'uio9' with size 0x100000 @ 0xffff8443b000
UIO(NET): -> mapped device 'uio10' with size 0x200000 @ 0xffff8423b000
UIO(NET): *** All initialisation successful, now sending all pending TX active before we block on events
UIO(NET): *** All pending TX active have been sent thru the raw sock, entering event loop now.
UIO(NET): *** You won't see any output from UIO Net anymore. Unless there is a warning or error.
DHCP request finished, IP address for netif client0 is: 172.16.1.91
DHCP request finished, IP address for netif client1 is: 172.16.1.90
```

Though the MAC, IP and memory addreses would be different of course. There will be delay in the tens
of seconds between the Linux userspace network driver initialisation and lwIP DHCP completion. This
is due to all the components being able to initialise first before the guest can get to userspace.
So lwIP will send many DHCP requests without success, with increasing timeout for each request.

## Future work
- Support more platforms.
- Fix the avalance of:
`<<seL4(CPU 0) [handleInterruptEntry/56 T0x8003849400 "net_driver_vm" @20c0fc]: Spurious interrupt!>>`
on debug config.
- Further minimise the Linux kernel. I've reduced the kernel size to only 12.5MiB by disabling most
  drivers. Though there are room to push it further by disabling more features that we don't need.
- Move the UIO driver into a kernel module or use iouring to reduce the context switch cost every
  time we send or receive a frame.
- (Tying into previous) Move away from UIO, which allow us to use SIMD instructions for faster
  memory operations in the driver. I mentioned this as the network queue entry size isn't a power of
  2, so clang always generate vector loads and stores to speed things up. Which has to be disabled
  to make UIO happy.
- Investigate mapping the queue and data regions as cached.
- Properly implement WFI/WFE in libvmm so we correctly measure CPU utilisation.
