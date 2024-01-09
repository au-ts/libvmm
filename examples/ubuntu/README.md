# VMM example running Armbian Bookworm

This example VMM supports Armbian Bookworm with only minimal CLI support on HardKernel's Odroidc4.

This works by running the initrd from armbian and using SD card passthrough containing the distro image for second stage booting. 

## Building with Make

```sh
make BOARD=odroidc4 MICROKIT_SDK=/path/to/sdk
```
Other configuration options can be passed to the Makefile such as `CONFIG`
and `BUILD_DIR`, see the Makefile for details.

## Set up

You'll need to flash an SD card with the armbiam bookworm image. Obtain image from: https://redirect.armbian.com/region/NA/odroidc4/Bookworm_current

Flashing with dd:
```sh
dd if=/path/to/armbian/image of=/dev/yourrawdevice bs=1M oflag=sync
```
You'll need to run above with sudo

You may also need to replace the ```root=/dev/mmcblk0p1``` param from bootargs located in ```board/odroidc4/dts/init.dts``` to the SD card partition that has the image flashed.

