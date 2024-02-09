# LibUIO Documentation Draft (@jade: undraft this)
## overview
TODO

## design decisions
### Why do we have no PPC?
### Others

## how to write your system description for UIO
```
<memory_region name="uio-irq" size="0x1000"/>

<!-- shared memory for ring buffer mechanism for networking -->
<memory_region name="net_free_rx" size="0x200_000" page_size="0x200_000"/>
<memory_region name="net_used_rx" size="0x200_000" page_size="0x200_000"/>
<memory_region name="net_free_tx" size="0x200_000" page_size="0x200_000"/>
<memory_region name="net_used_tx" size="0x200_000" page_size="0x200_000"/>

<memory_region name="net_shared_dma" size="0x200_000" page_size="0x200_000" />
```
## how to write your makefile so that linux userlevel knows about sddf
Includes the right directories in your C flags, e.g. (works for sDDF Net):
```
CFLAGS_SDDF := -Isddf/include \
		  		-Isddf/util/include \
		  		-Isddf

CFLAGS_USERLEVEL :=	-g3 \
		  			-O3 \
					-Wno-unused-command-line-argument \
		  			-Wall -Wno-unused-function -Werror \
		  			-Isddf/include
```
## how to write your dts
### Overlay
### UIO entry
```
uio {
    compatible = "generic-uio\0uio";
    reg = <0x00 0x30d00000 0x00 0x1000 0x00 0x30e00000 0x00 0x200000 0x00 0x31000000 0x00 0x200000 0x00 0x31200000 0x00 0x200000 0x00 0x31400000 0x00 0x200000 0x00 0x31600000 0x00 0x200000>;
    interrupts = <0x00 41 0x04>;
};
```
TODO(@jade): does the shared buffer region needs to be in the UIO entry?

## example userlevel driver

## future work
By using poll(), we are able to handle multiple connection (i.e. multiple UIO devices). But should we do it in LibUIO?

## useful resource
Documentation:
- https://www.kernel.org/doc/html/v4.13/driver-api/uio-howto.html
- https://man7.org/linux/man-pages/man2/poll.2.html

Ancient implementation (for reference when stuck):
- CAmkES VMM based guest SATA driver: https://github.com/seL4/camkes-vm-linux/blob/3de585eba8a42d351afe0422ccbab47a2f7ddab8/camkes-linux-artifacts/camkes-linux-apps/camkes-connector-apps/pkgs/dataport/sata_backend.c
- Peter's guest Ethernet driver draft: https://github.com/au-ts/libvmm/blob/7689f616d68be8d09e8181541a3ec094aa68a659/tools/linux/ethernet.c

Human:
- Jade, Peter, Eric
