<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Using multiple virtIO (PCI) devices with a Linux guest

This example is the exact same as the virtIO example except
that instead of using MMIO as the backend, we use PCI.

For instructions for building and using, see the virtIO
example's [README](../virtio/README.md).

The main differences between this example and the virtIO example
is the PCI node in the guest Device Tree and the use of different
APIs to setup virtIO PCI devices in the client VMM.
