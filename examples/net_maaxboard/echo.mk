#
# Copyright 2022, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

QEMU := qemu-system-aarch64

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
SYSTEM_DIR:=$(ROOT_DIR)/board/$(MICROKIT_BOARD)

SYSTEM_FILE := $(SYSTEM_DIR)/echo_server.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt

vpath %.c ${SDDF} ${LIBVMM} ${ECHO_SERVER}

IMAGES := vmm_ethernet.elf

CFLAGS := -mcpu=$(CPU) \
	  -mstrict-align \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
		-DBOARD_$(MICROKIT_BOARD) \
	  -I$(BOARD_DIR)/include \
	  -I$(SDDF)/include \
		-I$(LIBVMM)/include \
	  -I$(LIBVMM)/tools/linux/include \
	  -MD \
	  -MP

CFLAGS_USERLEVEL := \
	-g3 \
	-O3 \
	-Wno-unused-command-line-argument \
	-Wall -Wno-unused-function \
	-D_GNU_SOURCE \
	-target aarch64-linux-gnu \
	-I$(BOARD_DIR)/include \
	-I$(SDDF)/include \

LDFLAGS := -L$(BOARD_DIR)/lib -L${LIBC}
LIBS := --start-group -lmicrokit -Tmicrokit.ld -lc libvmm.a --end-group

CHECK_FLAGS_BOARD_MD5:=.board_cflags-$(shell echo -- ${CFLAGS} ${BOARD} ${MICROKIT_CONFIG} | shasum | sed 's/ *-//')

${CHECK_FLAGS_BOARD_MD5}:
	-rm -f .board_cflags-*
	touch $@

%.elf: %.o
	$(LD) $(LDFLAGS) $< $(LIBS) -o $@

all: loader.img

# Need to build libsddf_util_debug.a because it's included in LIBS
# for the unimplemented libc dependencies
${IMAGES}: libvmm.a

# Build the VMM for ethernet
ETHERNET_INITRD_FINAL := ethernet_linux_rootfs.cpio.gz

eth_vm/vm.dts: $(SYSTEM_DIR)/maaxboard.dts $(SYSTEM_DIR)/overlay.dts |eth_vm
	$(LIBVMM)/tools/dtscat $^ > $@

eth_vm/vm.dtb: eth_vm/vm.dts |eth_vm
	$(DTC) -q -I dts -O dtb $< > $@

eth_vm/rootfs.cpio.gz: $(SYSTEM_DIR)/rootfs.cpio.gz |eth_vm
	cp $(SYSTEM_DIR)/rootfs.cpio.gz $(BUILD_DIR)/$@

eth_vm/images.o: $(LIBVMM)/tools/package_guest_images.S $(SYSTEM_DIR)/linux \
		eth_vm/rootfs.cpio.gz eth_vm/vm.dtb |eth_vm
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(SYSTEM_DIR)/linux\" \
					-DGUEST_DTB_IMAGE_PATH=\"eth_vm/vm.dtb\" \
					-DGUEST_INITRD_IMAGE_PATH=\"eth_vm/rootfs.cpio.gz\" \
					$(LIBVMM)/tools/package_guest_images.S -o $@

eth_vmm.o: ${SYSTEM_DIR}/vmm_ethernet.c
	$(CC) $(CFLAGS) -c $^ -o $@

eth_vm:
	mkdir -p $@

vmm_ethernet.elf: eth_vmm.o eth_vm/images.o
	mkdir -p eth_vm
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

${IMAGE_FILE} $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

# include ${SDDF}/util/util.mk
# include ${SDDF}/network/components/network_components.mk
# include ${BENCHMARK}/benchmark.mk
# include ${TIMER_DRIVER}/timer_driver.mk
# include ${UART_DRIVER}/uart_driver.mk
# include ${SERIAL_COMPONENTS}/serial_components.mk
include ${LIBVMM}/vmm.mk
# LIBVMM_TOOLS := $(LIBVMM)/tools/
# include $(LIBVMM_TOOLS)/linux/uio/uio.mk
# include $(LIBVMM_TOOLS)/linux/net/net_init.mk
# include $(LIBVMM_TOOLS)/linux/uio_drivers/net/uio_net.mk

clean::
	${RM} -f *.elf .depend* $
	find . -name \*.[do] |xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f ${IMAGE_FILE} ${REPORT_FILE}

-include $(DEPS)
