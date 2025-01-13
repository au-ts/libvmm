#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
QEMU := qemu-system-aarch64

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
SYSTEM_DIR := $(EXAMPLE_DIR)/board/$(MICROKIT_BOARD)
SYSTEM_FILE := $(SYSTEM_DIR)/simple.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt
DTS_FILE := $(EXAMPLE_DIR)/build/vm.dts
LWIPDIR:=network/ipstacks/lwip/src
ECHO_SERVER:=${SDDF}/examples/echo_server

DRIV_DIR := imx
ETHERNET_DRIVER:=$(SDDF)/drivers/network/$(DRIV_DIR)
ETHERNET_CONFIG_INCLUDE:=${ECHO_SERVER}/include/ethernet_config
TIMER_DRIVER:=$(SDDF)/drivers/timer/$(DRIV_DIR)
CLK_DRIVER:=$(SDDF)/drivers/clk/imx

vpath %.c $(SDDF) $(LIBVMM) $(EXAMPLE_DIR) $(ECHO_SERVER)

IMAGES := vmm.elf uart_driver.elf serial_virt_tx.elf serial_virt_rx.elf lwip.elf \
				network_virt_tx.elf network_virt_rx.elf copy.elf timer_driver.elf clk_driver.elf

CFLAGS := \
	  -mstrict-align \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -DBOARD_$(MICROKIT_BOARD) \
	  -I$(BOARD_DIR)/include \
	  -I$(LIBVMM)/include \
	  -I$(LIBVMM)/tools/linux/include \
	  -I$(SDDF)/include \
		-I$(EXAMPLE_DIR)/include \
	  -I$(SDDF)/$(LWIPDIR)/include \
	  -I$(SDDF)/$(LWIPDIR)/include/ipv4 \
		-DBOARD_CLASS_imx \
	  -MD \
	  -MP \

CFLAGS_USERLEVEL := \
		-g3 \
		-O3 \
		-Wno-unused-command-line-argument \
		-Wall -Wno-unused-function \
		-D_GNU_SOURCE \
		-target aarch64-linux-gnu \
		-I$(BOARD_DIR)/include \
		-I$(SDDF)/include \
		-I${ETHERNET_CONFIG_INCLUDE}

LDFLAGS := -L$(BOARD_DIR)/lib -L${LIBC}
LIBS := --start-group -lmicrokit -Tmicrokit.ld -lc libvmm.a libsddf_util_debug.a --end-group

CHECK_FLAGS_BOARD_MD5:=.board_cflags-$(shell echo -- $(CFLAGS) $(BOARD) $(MICROKIT_CONFIG) | shasum | sed 's/ *-//')

$(CHECK_FLAGS_BOARD_MD5):
	-rm -f .board_cflags-*
	touch $@

include $(SDDF)/${LWIPDIR}/Filelists.mk

# NETIFFILES: Files implementing various generic network interface functions
# Override version in Filelists.mk
NETIFFILES:=$(LWIPDIR)/netif/ethernet.c

# LWIPFILES: All the above.
LWIPFILES=lwip.c $(COREFILES) $(CORE4FILES) $(NETIFFILES)
LWIP_OBJS := $(LWIPFILES:.c=.o) lwip.o utilization_socket.o \
	     udp_echo_socket.o tcp_echo_socket.o

OBJS := $(LWIP_OBJS)
DEPS := $(filter %.d,$(OBJS:.o=.d))

all: loader.img

${LWIP_OBJS}: ${CHECK_FLAGS_BOARD_MD5}
lwip.elf: $(LWIP_OBJS) libsddf_util.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

LWIPDIRS := $(addprefix ${LWIPDIR}/, core/ipv4 netif api)
${LWIP_OBJS}: |${BUILD_DIR}/${LWIPDIRS}
${BUILD_DIR}/${LWIPDIRS}:
	mkdir -p $@

vmm.elf: vmm.o images.o
	$(RANLIB) libvmm.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

all: loader.img

-include vmm.d

$(IMAGES): libvmm.a libsddf_util_debug.a

NET_DRIVER_VM_USERLEVEL := uio_net_driver
NET_DRIVER_VM_USERLEVEL_INIT := net_driver_init



$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

rootfs.cpio.gz: $(SYSTEM_DIR)/rootfs.cpio.gz $(NET_DRIVER_VM_USERLEVEL_INIT) $(NET_DRIVER_VM_USERLEVEL)
	$(LIBVMM)/tools/packrootfs ${SYSTEM_DIR}/rootfs.cpio.gz rootfs1 -o $@ \
		--startup $(NET_DRIVER_VM_USERLEVEL_INIT) \
		--home $(NET_DRIVER_VM_USERLEVEL)

vm.dts: $(SYSTEM_DIR)/linux.dts $(SYSTEM_DIR)/overlay.dts
	$(LIBVMM)/tools/dtscat $^ > $@

vm.dtb: vm.dts
	$(DTC) -q -I dts -O dtb $< > $@

vmm.o: $(EXAMPLE_DIR)/vmm.c $(CHECK_FLAGS_BOARD_MD5)
	$(CC) $(CFLAGS) -c -o $@ $<

images.o: $(LIBVMM)/tools/package_guest_images.S $(SYSTEM_DIR)/linux vm.dtb rootfs.cpio.gz
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(SYSTEM_DIR)/linux\" \
					-DGUEST_DTB_IMAGE_PATH=\"vm.dtb\" \
					-DGUEST_INITRD_IMAGE_PATH=\"rootfs.cpio.gz\" \
					$(LIBVMM)/tools/package_guest_images.S -o $@

include ${SDDF}/util/util.mk
include ${SDDF}/network/components/network_components.mk
include ${TIMER_DRIVER}/timer_driver.mk
include ${CLK_DRIVER}/clk_driver.mk
include ${SDDF}/drivers/serial/imx/uart_driver.mk
include $(SDDF)/serial/components/serial_components.mk
include $(LIBVMM)/vmm.mk
LIBVMM_TOOLS := $(LIBVMM)/tools/
include $(LIBVMM_TOOLS)/linux/uio/uio.mk
include $(LIBVMM_TOOLS)/linux/net/net_init.mk
include $(LIBVMM_TOOLS)/linux/uio_drivers/net/uio_net.mk

qemu: $(IMAGE_FILE)
	if ! command -v $(QEMU) > /dev/null 2>&1; then echo "Could not find dependency: qemu-system-aarch64"; exit 1; fi
	$(QEMU) -machine virt,virtualization=on,highmem=off,secure=off \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-m size=2G \
			-nographic

clean::
	$(RM) -f *.elf .depend* $
	find . -name \*.[do] |xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f $(IMAGE_FILE) $(REPORT_FILE)

-include $(DEPS)
