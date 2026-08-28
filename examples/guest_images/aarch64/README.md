<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Guest images used in examples

We use the same images (Linux kernel and initrd/rootfs) for all ARM examples and platforms.

Below are instructions for reproducing them.

## Linux kernel

### Details

* Git remote: https://github.com/torvalds/linux.git
* Tag: v7.1 (commit hash: `8cd9520d35a6c38db6567e97dd93b1f11f185dc6`)
* Toolchain: `aarch64-linux-gnu`
    * Version: aarch64-linux-gnu-gcc (GCC) 16.1.0

The config is based on the arm64 defconfig. You can also get
the Linux config used after booting by running the following
command in userspace: `zcat /proc/config.gz`.

### Building

```sh
git clone --depth 1 --branch v7.1 https://github.com/torvalds/linux.git
cd linux
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- all -j$(nproc)
```

The path to the image is: `arch/arm64/boot/Image`.

## Buildroot RootFS image

### Details

* Git remote: https://gitlab.com/buildroot.org/buildroot.git
* Tag: 2026.05.1  (commit hash: `cb857ba4c87a93e5265a9e4a3f32071abf39e14a`)

### Building

```sh
git clone --depth 1 --branch 2026.05.1 https://gitlab.com/buildroot.org/buildroot.git
cd buildroot
make qemu_aarch64_virt_defconfig
```

Once the source is cloned and the default config set up, we need to make a few changes to Buildroot settings:
1. Open the config TUI: `make menuconfig`.
2. Disable automatic DHCP, so the system doesn't hang on a non-existent network interface.
   Go to `System configuration` -> `Network interface to configure through DHCP`, clear the `eth0` string.
3. Disable kernel building. Go to `Kernel` -> `Linux Kernel`, press `n` to clear.
4. Make a CPIO image for use as initrd. Go to `Filesystem images` -> `cpio the root filesystem`,
   press `y` to enable. Ensure that `cpio type` is `cpio the whole root filesystem`, and `compression method` is `gzip`.
5. Disable `ext2/3/4 root filesystem` in the same menu.
6. Optionally enable any userspace packages you need in `Target packages`.
   In the image we ship, we enabled these packages in addition to the default:
    - `Hardware handling` -> `hwdata` (lspci pretty printing)
    - `Hardware handling` -> `memtester`
    - `Hardware handling` -> `memtool` (/dev/mem helper)
    - `Hardware handling` -> `parted` (GNU partition resizing program)
    - `Hardware handling` -> `pciutils`
    - `Networking applications` -> `ethtool`
    - `Networking applications` -> `iperf3`
    - `Networking applications` -> `openssh`
    - `Libraries` -> `Crypto` -> `openssl binary`
    - `Libraries` -> `Crypto` -> `CA Certificates`
    - `Libraries` -> `Networking` -> `libcurl` & `curl binary`
    - `Debugging, profiling and benchmark` -> `blktrace`
    - `Debugging, profiling and benchmark` -> `fio`
    - `Debugging, profiling and benchmark` -> `rt-tests`
    - `Debugging, profiling and benchmark` -> `stress-ng`
7. Save and exit.

Then change the following Busybox settings:
1. Open the config TUI: `make busybox-menuconfig`.
2. Enable HTTPS for wget: `Networking Utilities` -> `wget` -> `Try to connect to HTTPS using openssl`
3. Enable verbose logging for `mount`: `Linux System Utilities` -> `mount` -> `Support -v`
4. Disable `klogd`, so the boot log doesn't get replayed: `System Logging Utilities` -> `klogd`, press `n`.
4. Build `ntpd`: `Networking Utilities` -> `ntpd`, press `y`.
5. Save and exit.

Finally start the build process:
```
make
```

The root filesystem will be located at: `output/images/rootfs.cpio.gz` along
with the other buildroot artefacts.
