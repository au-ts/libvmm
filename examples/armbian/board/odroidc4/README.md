# Odroid-C4 images

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
Using menuconfig `make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- menuconfig`,
enable booting with NFS root file system by enabling these drivers as built-in:
* DWMAC_MESON
* MDIO
* MDIO_BUS_MUX_MESON_G12A
* MESON_GXL_PHY ??
* R8169 ??
* DWMAC_GENERIC ??

Now build:
```
make ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- all -j$(nproc)
```

### Reproducing Linux dts
To obtain the dts, boot armbian bookworm natively and cat it's DTS.

Image obtained from: https://redirect.armbian.com/region/NA/odroidc4/Bookworm_current

```
dtc -I fs -O dts /proc/device-tree > linux.dts
```

dts will be stored in `linux.dts`