<!--
     Copyright 2025, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Linux kernel image

## Linux kernel

### Details
* Image name: `linux`
* Config name: `linux_config`
* Git remote: https://github.com/torvalds/linux.git
* Tag: v6.13 (commit hash: `ffd294d346d185b70e28b1a28abe367bbfe53c04`)
* Toolchain: `aarch64-none-elf`
    * Version: (Arm GNU Toolchain 12.2.Rel1 (Build arm-12.24)) 12.2.1 20221205

You can also get the Linux config used after booting by running the following
command in userspace: `zcat /proc/config.gz`. This is a kernel minimally configured with a
IPv4 network stack + DNS + DHCP and virtIO device drivers.

### Instructions for reproducing
```
git clone --depth 1 --branch v6.13 https://github.com/torvalds/linux.git
cp config linux/.config
make -C linux ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- all -j$(nproc)
```

The path to the image is: `linux/arch/arm64/boot/Image`.

## Buildroot RootFS image

### Details
* Image name: `rootfs.cpio.gz`
* Config name: `buildroot_config`
* Version: 2022.08-rc2

### Instructions for reproducing

```
wget https://buildroot.org/downloads/buildroot-2022.08-rc2.tar.xz
tar xvf buildroot-2022.08-rc2.tar.xz
cp buildroot_config buildroot-2022.08-rc2/.config
make -C buildroot-2022.08-rc2
```

The root filesystem will be located at: `buildroot-2022.08-rc2/output/images/rootfs.cpio.gz` along
with the other buildroot artefacts.