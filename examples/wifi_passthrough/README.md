<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# WiFi passthrough

This example demonstrate WiFi hardware passthrough for Linux guest.

The example currently works on the following platforms:

* Raspberry Pi 4B
     * I had it working for the 4GB variant, but won't be too bad to add support for the other variants.

## Building

```sh
make MICROKIT_BOARD=<BOARD> MICROKIT_SDK=/path/to/sdk
```

Where `<MICROKIT_BOARD>` is one of:

* `rpi4b_4gb`

Other configuration options can be passed to the Makefile such as `MICROKIT_CONFIG`
and `BUILD_DIR`, see the Makefile for details.

<!-- @billn sort out -->
<!-- By default the build system fetches the Linux kernel and initrd images from
Trustworthy Systems' website on-demand. To override this anduse your own images,
specify `LINUX` and/or `INITRD`. For example:

```sh
make MICROKIT_BOARD=qemu_virt_aarch64 MICROKIT_SDK=/path/to/sdk LINUX=/path/to/linux INITRD=/path/to/initrd qemu
``` -->

## Running

You can deploy the built Microkit image in `$BUILD_DIR/loader.img` with U-Boot or another
bootloader of your choice. Please see the Microkit Manual that is bundled with the distribution
for more details.

Here is a demo of the example in action:
```
Welcome to Buildroot
buildroot login: root
# ip route
default via 172.XXX.XXX.XXX dev eth0
172.XXX.XXX.XXX/16 dev eth0 scope link  src 172.XXX.XXX.XXX
# dmesg | grep mac
[    0.318036] bcm2835-dma fe007000.dma-controller: DMA legacy API manager, dmachans=0x1
[    1.952363] unimac-mdio unimac-mdio.-19: Broadcom UniMAC MDIO bus
[    1.971007] usbcore: registered new interface driver brcmfmac
[    2.410958] brcmfmac: F1 signature read @0x18000000=0xXXXXXXXX
[    2.435902] brcmfmac: brcmf_fw_alloc_request: using brcm/brcmfmac43455-sdio for chip BCM4345/6
[    2.728098] brcmfmac: brcmf_c_process_txcap_blob: no txcap_blob available (err=-2)
[    2.740800] brcmfmac: brcmf_c_preinit_dcmds: Firmware: BCM4345/6 wl0: Aug 29 2023 01:47:08 version 7.45.265 (28bca26 CY) FWID 01-b677b91b
# ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq qlen 1000
    link/ether XX:XX:XX:XX:XX:XX brd ff:ff:ff:ff:ff:ff
    inet 172.XXX.XXX.XXX/16 brd 172.XXX.XXX.XXX scope global eth0
       valid_lft forever preferred_lft forever
3: wlan0: <BROADCAST,MULTICAST> mtu 1500 qdisc noop qlen 1000
    link/ether XX:XX:XX:XX:XX:XX brd ff:ff:ff:ff:ff:ff
# ip link set eth0 down
[   71.327752] bcmgenet fd580000.ethernet eth0: Link is Down
# ip link set wlan0 up
[   73.585800] brcmfmac: brcmf_cfg80211_set_power_mgmt: power save enabled
# iw dev wlan0 scan | grep SSID
        SSID: UNSW-IoT
        SSID: UNSW-IoT
        SSID: eduroam
        SSID: UNSW Guest
        SSID: eduroam
        SSID: eduroam
        SSID: UNSW Guest
        SSID: UNSW-IoT
        SSID: keg
        SSID: keg
# wpa_passphrase "XXXXXXXX" "XXXXXXXX" > /etc/wpa_supplicant.conf
# wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant.conf
Successfully initialized wpa_supplicant
# udhcpc -i wlan0
udhcpc: started, v1.38.0
udhcpc: broadcasting discover
udhcpc: broadcasting discover
udhcpc: broadcasting discover
udhcpc: broadcasting select for 10.XXX.XXX.XXX, server 10.XXX.XXX.XXX
udhcpc: lease of 10.XXX.XXX.XXX obtained from 10.XXX.XXX.XXX, lease time 3600
deleting routers
adding dns 10.XXX.XXX.XXX
adding dns 10.XXX.XXX.XXX
# ip route
default via 10.XXX.XXX.XXX dev wlan0
10.XXX.XXX.XXX/23 dev wlan0 scope link  src 10.XXX.XXX.XXX
# wget google.com
Connecting to google.com (142.251.222.238:80)
Connecting to www.google.com (142.251.152.119:80)
saving to 'index.html'
index.html           100% |********************************| 79768  0:00:00 ETA
'index.html' saved
# head index.html
<!doctype html><html itemscope="" itemtype="http://schema.org/WebPage" lang="en-AU">...
```

## Pitfalls

These are notes on problems I ran into while trying to get this working on the Raspberry Pi 4B. Though it
will apply generally to other platforms as well. The provided Linux and Buildroot initrd configs and images
already have these problems ironed out. But these notes will be relevant if you want to add support
for other platforms or would like to better understand the picture.

### In-kernel drivers and modules
The Wi-Fi stack and Broadcom drivers must be built into the kernel text, rather than a kernel modules.
The [Pi kernel tree](https://github.com/raspberrypi/linux)'s default config `bcm2711_defconfig` built
them as kernel modules so that won't work.

This can be worked around if you package the kernel modules with your initial ramdisk and dynamically
load them. But I find building them into the kernel more convenient.

See `board/rpi4b_4gb/rpi_linux_wifi.diff` for the changes I have to make to the config file from
`bcm2711_defconfig` for Wi-Fi hardware passthrough to work.

### Buildroot packages
Similar problem apply to the buildroot initial ramdisk. We must enable and package the
Wi-Fi utilities and Broadcom firmware.

See `board/rpi4b_4gb/rpi_buildroot_wifi.diff` for the changes I have to make to the config file from
`raspberrypi4_64_defconfig` for Wi-Fi hardware passthrough to work.

### One to one HPA-GPA mapping for DMA
Since the Wi-Fi card and other devices related to its function (e.g. mailbox) are DMA capable.
The Host Physical Address (HPA) of guest RAM must be equal to the Guest Physical Address (GPA)
of RAM start that we declares in the DTS for DMA to work.

### SoC DMA window
On some SoC, the DMA engine can only reach a certain range of physical RAM. On the Raspberry Pi 4,
this range is `[0x0..0x3C000000)`. Since guest RAM was configured as 1GB, it will occupy HPA
`[0x10000000..0x50000000)`. Thus, if Linux sends a DMA request with GPA > `0x3C000000`, the request
will fail.

To fix this, you must tell Linux what is the valid DMA-capable address range. In this example, we've
done that via the DTS overlay:
```
    soc {
        /delete-property/ dma-ranges;
        dma-ranges = <0xd0000000 0x10000000 0x2c000000>;
```

### IRQ passthrough
Similar to the previous pitfall. You must pass through IRQ of all the devices required for Wi-Fi to
function to the guest. Currently it seems like I have missed some, as removing `irqpoll` from the DTS
overlay will break the demo. Will fix.