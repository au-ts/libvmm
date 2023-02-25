<!--
   Copyright 2022, UNSW
   SPDX-License-Identifier: CC-BY-SA-4.0
-->

libsharedringbuffer
-------------------

This directory contains a library implementation of shared ring
buffers for the transportation of data. This is intended to be used as a
communication mechanism between system components for bulk data transfer,
and was originally created as a data plane between an ethernet driver and
network stack for the sDDF. This library doesn't contain any code that
interfaces with seL4. It is expected that the user will provide shared
memory regions and notification/signalling handlers to this library.

To use this library in a project you can link `shared_ringbuffer` in your
target applications CMake configuration.

This libary is intended to be used by both a producer and a consumer. For
example, an ethernet driver produces data for a network stack to consume.
2 separate shared memory regions are required for each ring handle; one
to store available buffers and one to store used buffers. Each ring buffer
contains a separate read index and write index. The reader only ever
increments the read index, and the writer the write index. As read and
writes of a small integer are atomic, we can keep memory consistent
without locks.
The size of the ring buffers can be set with the cmake config option,
`LIB_SHARED_RINGBUFFER_DESC_COUNT` but defaults to 512. The user must
ensure that the shared memory regions handed to the library are of
appropriate size to match this.

Use case
---------

This library is intended to be used with a separate shared memory region,
usually allocated for DMA for a driver. The ring buffers can then contain
pointers into this shared memory, indicating which buffers are in use or
available to be used by either component.
Typically, 2 shared ring buffers are required, with separate structures
required on the recieve path and transmit path. Thus there are 4 regions
of shared memory required: 1 storing pointers to available RX buffers,
1 storing pointers to used RX buffers, 1 storing pointers to TX 
buffers, and another storing pointers to available TX buffers.

On initialisation, both the producer and consumer should allocate their
own ring handles (`struct ring_handle`). These data structures simply
store pointers to the actual shared memory regions and are used to
interface with this library. The ring handle should then be passed into
`ring_init` along with 2 shared memory regions, an optional function
pointer to signal the other component, and either 1 or 0 to indicate
whether the read/write indices need to be initialised (note that only one
side of the shared memory regions needs to do this).

After initialisation, a typical use case would look like:
The driver wants to add some buffer that will be read by another component
(for example, a network stack processing incoming packets).

    1. The driver dequeues a pointer to an available buffer from the
    available ring.
    2. Once data is inserted into the buffer (eg. via DMA), the driver
    then enqueues the pointer to it into the used ring.
    3. The driver can then notify the reciever.
    4. Similarly, the reciever dequeues the pointer from the used ring,
    processes the data, and once finished, can enqueue it back into
    the available ring to be used once more by the driver.
