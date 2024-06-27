# VMM example running Armbian

This example VMM supports Armbian with minimal desktop on HardKernel's Odroidc4.

This works by running the initrd from armbian and passing through a block device containing the distro image for second stage booting.

## Building with Make

```sh
make BOARD=odroidc4 MICROKIT_SDK=/path/to/sdk
```
Other configuration options can be passed to the Makefile such as `CONFIG`
and `BUILD_DIR`, see the Makefile for details.


## OdroidC4 build instructions

### Getting Armbian

You'll need to flash an SD card with the armbiam image.

Image obtained from https://www.armbian.com/odroid-c4/. Or get the [armbian image](https://github.com/armbian/community/releases/download/24.8.0-trunk.205/Armbian_community_24.8.0-trunk.205_Odroidc4_bookworm_current_6.6.35_minimal.img.xz) directly.

Flashing with dd:
```sh
dd if=/path/to/armbian/image of=/dev/yourrawdevice bs=1M oflag=sync
```
You'll need to run above with sudo

You may also need to replace the ```root=/dev/mmcblk0p1``` param from bootargs located in ```board/odroidc4/armbian_vm/dts/init.dts``` to the SD card partition that has the image flashed.

#### Booting with NFS

Alternatively, you may also boot with NFS by storing the armbian root-fs in a NFS server, then replacing `root=` with `root=/dev/nfs nfsroot=[<server-ip>:]/path/to/rootfs` and removing `rootwait`.

## QEMU build instructions

### Getting Armbian

Image obtained from https://www.armbian.com/qemu-uboot-arm64/. Or get the [armbian image](https://github.com/armbian/os/releases/download/24.8.0-trunk.201/Armbian_24.8.0-trunk.201_Qemu-uboot-arm64_bookworm_current_6.6.35_minimal.img.qcow2.xz) directly.

Decompress the image and pass it in as an argument `ARMBIAN` to the Makefile.

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
```
git clone --depth 1 --branch v6.1 https://github.com/torvalds/linux.git
cp linux_config linux/.config
make -C linux ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- linux_config all -j$(nproc)
```

The path to the image will be: `linux/arm64/boot/Image`.

### Reproducing Linux configuration from scratch
Begin with default linux configuration
```
make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- defconfig
```
For odroidc4, we also need to enable the phy and ethernet drivers as built-in.
Using menuconfig `make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- menuconfig` enable:
* DWMAC_MESON
* MDIO_BUS_MUX_MESON_G12A

Now build:
```
make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- all -j$(nproc)
```

### Reproducing Linux dts
To obtain the dts for odroidc4, boot armbian natively and cat it's DTS.

```
dtc -I fs -O dts /proc/device-tree > linux.dts
```

dts will be stored in `linux.dts`


To obtain dts for QEMU, add the flag `-machine dumpdtb=qemu.dtb` to the qemu command in Makefile. You can then run:

```
dtc -I dtb -O dts qemu.dtb > qemu.dts
```

dts will be stored in qemu.dts