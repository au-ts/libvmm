<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Guest images used

We use the mainline Linux kernel.

Below are instructions for reproducing them.

## Linux kernel

### Details

* Config name: `linux_config`, equivalent to `x86_64_defconfig`
* Git remote: https://github.com/torvalds/linux.git
* Tag: v6.19 (commit hash: `05f7e89ab9731565d8a62e3b5d1ec206485eeb0b`)
* Toolchain: `gcc`
    * Version: gcc (Debian 15.2.0-15) 15.2.0

You can also get the Linux config used after booting by running the following
command in userspace: `zcat /proc/config.gz`.

### Building

```sh
git clone --depth 1 --branch v6.19 https://github.com/torvalds/linux.git
cp x86_64_defconfig linux/.config
make -C linux ARCH=x86_64 all -j$(nproc)
```

The path to the image is: `linux/arch/x86_64/boot/bzImage`.

## Buildroot RootFS image

### Details

* Config name: `buildroot_config`
* Version: 2025.11-rc2

### Building

```sh
wget https://buildroot.org/downloads/buildroot-2025.11-rc2.tar.xz
tar xvf buildroot-2025.11-rc2.tar.xz
cp buildroot_config buildroot-2025.11-rc2/.config
make -C buildroot-2025.11-rc2
```

The root filesystem will be located at: `buildroot-2025.11-rc2/output/images/rootfs.cpio.gz` along
with the other buildroot artefacts.
