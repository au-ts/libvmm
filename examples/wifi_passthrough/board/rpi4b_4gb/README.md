<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Guest images used

We use the Raspberry Pi fork for the kernel, and mainline for the Buildroot initrd.

Below are instructions for reproducing them.

## Linux kernel

### Details

* Config name: `linux_config`, derived from `bcm2711_defconfig`.
* Git remote: https://github.com/raspberrypi/linux.git
* Tag: rpi-6.18.y (commit hash: `57083d8cf8f0c5f2e9f97ed79d5fe2382a20d3d4`)
* Toolchain: `aarch64-linux-gnu`
    * Version: aarch64-linux-gnu-gcc (GCC) 15.2.0

See [rpi_linux_wifi.diff](rpi_linux_wifi.diff) for the list of changes applied
to `bcm2711_defconfig`.

### Building

```sh
git clone --depth 1 --branch rpi-6.18.y https://github.com/raspberrypi/linux.git
cp linux_config linux/.config
mmake ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs -j$(nproc)
```

The path to the image is: `linux/arch/arm64/boot/Image`.

## Buildroot RootFS image

### Details

* Config name: `buildroot_config`, derived from `raspberrypi4_64_defconfig`.
* Git remote: https://github.com/buildroot/buildroot.git
* Tag: 2026.05 (commit hash: `313414b92c2501a2bc123ffa1b6383dca464de05`)

See [rpi_buildroot_wifi.diff](rpi_buildroot_wifi.diff) for the list of changes applied
to `raspberrypi4_64_defconfig`.

### Building

```sh
git clone --depth 1 --branch 2026.05 https://github.com/buildroot/buildroot.git
cp buildroot_config buildroot/.config
make
```

The root filesystem will be located at: `buildroot/output/images/rootfs.cpio.gz` along
with the other buildroot artefacts.
