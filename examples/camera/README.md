<!--
     Copyright 2025, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Camera VMM Example for Raspberry Pi 4B 1GB

This example is a VMM that supports running a Linux guest with camera functionality on the Raspberry Pi 4B 1GB board. It integrates with the camera driver and client for capturing and processing frames.

The example currently works on the following platform:
* Raspberry Pi 4B 1GB

## Building

To build the example, run:

```sh
make MICROKIT_BOARD=<BOARD> MICROKIT_SDK=/path/to/sdk
```

Where `<MICROKIT_BOARD>` is one of:
* `rpi4b_1gb'

Other configuration options can be passed to the Makefile such as `MICROKIT_CONFIG`
and `BUILD_DIR`, see the Makefile for details.

This will compile the VMM, camera driver, and client components for the Raspberry Pi 4B 1GB board.

## Running

After building, this should produce a loader.img file in the /build directory.

