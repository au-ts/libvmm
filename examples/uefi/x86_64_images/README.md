<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Guest images used

We use the mainline versions of all guest images.

Below are instructions for reproducing them.

## TianoCore EDK II Open Virtual Machine Firmware (OVMF)

### Details

* Git remote: https://github.com/tianocore/edk2.git
* Tag: edk2-stable202605 (commit hash: `b03a21a63e3bd001f52c527e5a57feddb53a690b`)

### Building

1. Install the build dependencies:
```
sudo apt install build-essential uuid-dev iasl git nasm python-is-python3
```

2. Get the sources:
```
git clone --depth 1 --branch edk2-stable202605 https://github.com/tianocore/edk2
cd edk2
git submodule update --init
```

3. Enable debug logging. By default the EDK II UEFI firmware will output
debug logs to a special I/O Port that QEMU handles, but we do not implement
that special port, so we need to tell the firmware to just use standard COM1.
Append this block to the end of `OvmfPkg/OvmfPkgX64.dsc`:
```
[PcdsFixedAtBuild]
gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase | 0x3F8
gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate     | 115200
gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio      | FALSE
```

4. Build:
```
make -C BaseTools
./OvmfPkg/build.sh -a X64 -b DEBUG -p OvmfPkg/OvmfPkgX64.dsc -D DEBUG_ON_SERIAL_PORT
```

The binary will be located at:
```
ls -hl Build/OvmfX64/DEBUG_GCC/FV/OVMF.fd
```

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
cp linux_config linux/.config
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
