<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Odroid-C4 images

## Linux kernel

### Details
* Image name: `linux`
* Config name: `linux_config`
* Git remote: https://github.com/torvalds/linux.git
* Tag: v6.17 (commit hash: `e5f0a698b34ed76002dc5cff3804a61c80233a7a`)
* Toolchain: `aarch64-none-elf`
    * Version: (Arm GNU Toolchain 12.2.Rel1 (Build arm-12.24)) 12.2.1 20221205

You can also get the Linux config used after booting by running the following
command in userspace: `zcat /proc/config.gz`.

### Instructions for reproducing

```
git clone --depth 1 --branch v6.17 https://github.com/torvalds/linux.git
cp linux_config linux/.config
make -C linux ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- linux_config all -j$(nproc)
```

The path to the image will be: `linux/arm64/boot/Image`.

## Buildroot RootFS image

Note that buildoot currently does not list a configuration for OdroidC4 so we just
use the OdroidC2 configuration with `ethtool` enabled.
