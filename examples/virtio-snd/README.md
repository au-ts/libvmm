<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# virtIO sound example

This example shows off the virtIO sound support that libvmm provides using the
[seL4 Device Driver Framework](https://github.com/au-ts/sddf) to virtualise
the hardware.

We do not have a native driver for sound, but instead leverage a Linux VM that
contains a driver and have a user-level program to convert sDDF requests/responses
into Linux calls into the ALSA library.

Currently this example is not integrated with the [virtIO example](../virtio)
due to issues with using multiple driver VMs in the same system. Therefore, temporarily,
we have a separate virtIO example for the emulating the sound device. You can find more
information [in this GitHub issue](https://github.com/au-ts/libvmm/issues/60).

The example currently works on the following platforms:
* QEMU virt AArch64
* HardKernel Odroid-C4

For details on the internals of the example and instructions on building and running it,
see [docs/SOUND.md](docs/SOUND.md).
