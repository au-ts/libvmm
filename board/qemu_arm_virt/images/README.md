# QEMU ARM virt images

## Linux kernel

### Info
* Image name: linux
* Git remote: https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
* Tag: v5.18 (commit hash: 4b0986a3613c92f4ec1bdc7f60ec66fea135991f)
* Config name: linux_config
* Toolchain: aarch64-none-elf-

You can also get the Linux config used after booting by running the following
command in userspace: `zcat /proc/config.gz`.

### Instructions for reproducing
After you've acquired the Linux source and checkout out the right revision:
```
make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- /path/to/linux_config menuconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- all [-j$(nproc)]
```

## Buildroot RootFS image

### Info
* Name: rootfs.cpio.gz
* Version: 2022.08-rc2
* Config: buildroot_config

### Instructions for reproducing

```
wget https://buildroot.org/downloads/buildroot-2022.08-rc2.tar.xz
tar xvf buildroot-2022.08-rc2.tar.xz
cd buildroot-2022.08-rc2
```
