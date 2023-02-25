<!--
     Copyright 2023, UNSW (ABN 57 195 873 179)

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Intro

This is a simple documentation about the structure of the seL4cp VMM VirtIO Backend.

TODO: say more things

# Overview

- `virtio_mmio.[hs]`: generic virtio mmio emul layer that handles MMIO Device Register read/write and non device-specific I/O operations
- `virtio_<device name>_emul.[hs]`: device-specific virtio mmio emul layer that:
     * handles device-specific I/O operations
     * manipulates the virtqueues
     * talk to the backend layer
- device driver/vswitch/vsock: the backend layer that actually does works

# Reference

https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1

# Issues

TODO