<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# MaaXBoard images

## Linux kernel

### Details
* Image name: `linux`
* Config name: `linux_config`
* Git remote: https://github.com/torvalds/linux.git
* Tag: v6.1 (commit hash: `830b3c68c1fb1e9176028d02ef86f3cf76aa2476`)
* Toolchain: `aarch64-none-elf`
    * Version: GNU Toolchain for the A-profile Architecture 10.2-2020.11 (arm-10.16)) 10.2.1 20201103

You can also get the Linux config used after booting by running the following
command in userspace: `zcat /proc/config.gz`.

### Instructions for reproducing

#### Step 1: clone the git repo

```
git clone --depth 1 --branch v6.1 https://github.com/torvalds/linux.git
```

If a Linux repo already exists, run `make clean` first.

#### Step 2: configure the kernel

Option 1: use the configuration file `linux_config`

```
cp linux_config linux/.config
make -C linux ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- linux_config all -j$(nproc)
```

Option 2: manually configure dependencies

```

```

#### Step 3: 

```
make all ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- -j$(nproc)
```

The path to the image will be: `linux/arm64/boot/Image`.

## Buildroot RootFS image

### Details
* Image name: `rootfs.cpio.gz`
* Config name: `buildroot_config`
* Git remote: https://github.com/buildroot/buildroot
* Toolchain: `aarch64-none-elf`
    * Version: GNU Toolchain for the A-profile Architecture 10.2-2020.11 (arm-10.16)) 10.2.1 20201103
    
### Instructions for reproducing

#### Step 1: clone the git repo
```
git clone --depth 1 https://github.com/buildroot/buildroot
```

If a Buildroot repo already exists, run `make clean` first.

#### Step 2: configure the dependencies

Option 1: use the configuration file `buildroot_config`.

```
cp buildroot_config buildroot/.config
```

Option 2: manually configure dependencies.

```
make menuconfig ARCH=arm64 CROSS_COMPILE=aarch64-none-elf-

Enable cpio filesystem: Filesystem images -> cpio the root filesystem -> Compression method (gzip)
DHCP: Target packages -> Networking applications -> dhcpcd
```

#### Step 3: compile the image

```
make all ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- -j$(nproc)
```

The path to the image will be: `buildroot/output/images/rootfs.cpio.gz`.
