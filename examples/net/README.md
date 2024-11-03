<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# UIO Networking example

This example show case an Ethernet driver VM running with sDDF's echo server on the Odroid C4. It is a shallow copy that only contains files that needs to be modified to accomodate the Ethernet driver VM. This is a summary of modified files, relative to `/dep/sddf/examples/echo_server/`:
- `board/odroidc4/echo_server.system`: swapped out native Ethernet driver for VM.
- `include/serial_config/serial_config.h`: Hooked VM to serial TX virt.
- `Makefile`: specified `zig cc` for Linux userspace drivers compilation and added libvmm related paths.
- `echo.mk`: swapped out native Ethernet driver build step for VM build steps.

The basic idea is we have a userspace program in Linux that converts sDDF's network TX/RX into corresponding syscalls in a Linux socket.
- The sDDF network control, data regions and notification from virtualisers are mapped into Linux userspace via UIO.
- A promiscuous socket is opened to sniff all incoming frames. When a frame comes through from the socket, we enqueue the data into the appropriate sDDF queues and fault on a pre-determined address so the VMM can notify the RX virtualiser.
- When a client wants to transmit data, we will get TX virt notifications via UIO, then we simply write the data out into the socket.

Even though we already have a native Ethernet driver for Odroid C4, this example is still useful as it could be used to provide Ethernet driver VM on new boards or further developed into a WiFi/Cellular/Bluetooth driver VM.

The example currently works on the following platforms:
* HardKernel Odroid-C4

Things to note when using the network UIO driver:
- You need to map in the control queues as uncached in the guest and native virtualisers. This is because Linux treats UIO regions as device memory thus map them uncached to userspace.