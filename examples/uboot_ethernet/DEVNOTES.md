# Outline for setting up UBoot variant of pass-through ethernet example
This contains notes for internal use on how to get UBoot to boot a Linux kernel
over ethernet using a tftp server. This was put together on Ubuntu 22.04 LTS.

Note that like the other examples this requires having the Microkit setup beforehand.

## Modified code
The code is similar to that of the standard ethernet example. The primary change to vmm.c is the 
use of arch/aarch64/uboot.h (and its variant functions) in place of arch/aarch64/linux.h. 

Some minor changes are made in the makefile as well to reference the UBoot binary instead of the Linux image.

## UBoot setup
### Downloading UBoot
The source for UBoot is downloaded from: https://github.com/u-boot/u-boot (latest version should be fine)

### Compilation tools for UBoot
We're building for an aarch64 system so we need to make sure we have the correct compilation tools. For 
GCC on a Debian based distribution we can use the following to install the correct compiler:
```
sudo apt-get install gcc gcc-aarch64-linux-gnu
```
UBoot documentation has detailed instructions here - https://docs.u-boot.org/en/latest/build/gcc.html

### Configuring UBoot
Before we build UBoot we first need to configure it with a few steps.

Detailed documentation for the qemu_arm_virt board is given here: https://docs.u-boot.org/en/latest/board/emulation/qemu-arm.html

First, in the root of the UBoot source code (i.e. pathtoUBoot/u-boot/) generate a generic aarch64 config file
using the following command
```
make qemu_arm64_defconfig
```

Next we need to modify this to work correctly with the Microkit. To easily modify the config file we may use
```
make menuconfig
```
In the interactive menu navigate to the following config and turn it off and then save the file.
Environment
    --> Environment is in flash memory

We need to turn this off as otherwise we'll get a memory fault as UBoot tries to access memory from a flash device we don't support.

### Building UBoot
Now we've configured UBoot we need to build and copy it. Running the following will use all your processors and attempt a clean build
using the compiler we installed before;
```
CROSS_COMPILE=aarch64-linux-gnu- make -j$(nproc) -B
```
We then copy the binary file (u-boot.bin) over to pathtoUBootexample/uboot_ethernet/board/qemu_arm_virt/
```
cp u-boot.bin pathtoUBootexample/uboot_ethernet/board/qemu_arm_virt/
```
After this UBoot should be setup correctly.

## Setting up a tftp server and linux boot files
Now we need to make sure we have a tftp server setup with the correct files so UBoot can access it and download the correct files.

Setting up a tftp server is a bit finicky. I used tftpd-hpa but there are alternatives so whatever works for you.

Once the tftp server is running we need to provide the correct files to be able to download them from UBoot.

Copy both the Linux image and the rootfs.cpio.gz from from pathtoUBootexample/uboot_ethernet/board/qemu_arm_virt/ over to your 
/tftpboot (or whatever path you've put it at).

Now we need to reformat the rootfs.cpio.gz to a format that UBoot can interpret. To do this we need to use the uboot utility 'mkimage'.
Run the following in the /tftpboot folder (where rootfs.cpio.gz should now be located).
```
mkimage -A arm -O linux -T ramdisk -d rootfs.cpio.gz uRamdisk
```
This will produce a file called uRamdisk which UBoot will happily read. Now we're ready to build and run our project.

## Build and run the project
From pathtoUBootexample/uboot_ethernet/ we can run the following to build and start the project.
```
make BOARD=qemu_arm_virt MICROKIT_SDK=/path/to/sdk qemu
```
This will build and start the project. The microkit should run UBoot which should then run till it reaches its terminal.

From here we want to setup the server ip for our tftp server and then download the correct files before we boot Linux. Run the following in
the UBoot terminal.

First, we set the server ip:
```
setenv serverip 'yourtftpserverip'
```
Next, we download the Linux distribution and rootfs (your addresses may be different just make sure the components don't overlap).
```
tftp 0x42000000 linux
```
This will download the file named linux from our /tftpboot folder (this should be the Linux image we put there earlier).
```
tftp 0x45000000 uRamdisk
```
This will download the formatted rootfs from our /tftpboot folder. We place this far enough away that it doesn't overlap the linux image.

Finally we can boot our linux image using the following booti command in UBoot:
```
booti 0x42000000 0x45000000 0x40000000
```
The format here is
```
booti $linux_image $rootfs $linux.dtb
```
Note the device tree blob is the same we load in and use for UBoot so we don't need to load it via tftp we can just reeuse it.

After this our Linux distribution should boot!
