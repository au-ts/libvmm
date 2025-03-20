# Copyright 2022, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

QEMU := qemu-system-aarch64
DTC := dtc
PYTHON ?= python3

LIBVMM_DOWNLOADS := https://trustworthy.systems/Downloads/libvmm/images/

METAPROGRAM := $(TOP)/meta.py

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
ECHO_SERVER:=${SDDF}/examples/echo_server
LWIPDIR:=network/ipstacks/lwip/src
BENCHMARK:=$(SDDF)/benchmark
UTIL:=$(SDDF)/util
ETHERNET_DRIVER:=$(SDDF)/drivers/network/$(ETH_DRV_DIR)
SERIAL_COMPONENTS := $(SDDF)/serial/components
SERIAL_DRIVER := $(SDDF)/drivers/serial/$(SERIAL_DRIV_DIR)
TIMER_DRIVER:=$(SDDF)/drivers/timer/$(TIMER_DRV_DIR)
NETWORK_COMPONENTS:=$(SDDF)/network/components

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
SYSTEM_DIR := $(EXAMPLE_DIR)/board/$(MICROKIT_BOARD)
IMAGE_FILE := loader.img
REPORT_FILE := report.txt
SYSTEM_FILE := echo_server.system
DTS := $(SDDF)/dts/$(MICROKIT_BOARD).dts
DTB := $(MICROKIT_BOARD).dtb
CLIENT_DTB := client_vm/vm.dtb
METAPROGRAM := $(TOP)/meta.py
CLIENT_VM_USERLEVEL := echoit
CLIENT_VM_USERLEVEL_INIT := echoserver_init

vpath %.c ${SDDF} $(LIBVMM) $(EXAMPLE_DIR) $(NETWORK_COMPONENTS) ${ECHO_SERVER}

IMAGES := eth_driver.elf client_vmm0.elf client_vmm1.elf benchmark.elf idle.elf network_virt_rx.elf\
	  network_virt_tx.elf network_copy.elf timer_driver.elf serial_driver.elf serial_virt_tx.elf serial_virt_rx.elf

CFLAGS := -mcpu=$(CPU) \
	  -mstrict-align \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -I$(BOARD_DIR)/include \
	  -I$(SDDF)/include \
		-I$(LIBVMM)/include \
	  -I${ECHO_INCLUDE}/lwip \
	  -I${SDDF}/$(LWIPDIR)/include \
	  -I${SDDF}/$(LWIPDIR)/include/ipv4 \
	  -MD \
	  -MP \
		-target $(TARGET) \
		-I/Users/terrybai/ts/arm-gnu-toolchain-12.2.rel1-darwin-arm64-aarch64-none-elf/aarch64-none-elf/include

CFLAGS_USERLEVEL := \
	-g3 \
	-O3 \
	-Wno-unused-command-line-argument \
	-Wall -Wno-unused-function \
	-D_GNU_SOURCE \
	-target aarch64-linux-gnu \
	-I$(BOARD_DIR)/include \
	-I$(SDDF)/include

LDFLAGS := -L$(BOARD_DIR)/lib -L/Users/terrybai/ts/arm-gnu-toolchain-12.2.rel1-darwin-arm64-aarch64-none-elf/aarch64-none-elf/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld -lc libsddf_util_debug.a libvmm.a --end-group

CHECK_FLAGS_BOARD_MD5:=.board_cflags-$(shell echo -- ${CFLAGS} ${BOARD} ${MICROKIT_CONFIG} | shasum | sed 's/ *-//')

${CHECK_FLAGS_BOARD_MD5}:
	-rm -f .board_cflags-*
	touch $@

%.elf: %.o
	$(LD) $(LDFLAGS) $< $(LIBS) -o $@

# Client VMs
client_vm:
	mkdir -p $@

${LINUX}:
	curl -L ${LIBVMM_DOWNLOADS}/$(LINUX).tar.gz -o $(LINUX).tar.gz
	mkdir -p linux_download_dir
	tar -xf $@.tar.gz -C linux_download_dir
	cp linux_download_dir/${LINUX}/linux ${LINUX}

echoit.o: $(SDDF_BENCHMARK)/linux/echoit.c
	$(CC_USERLEVEL) $(CFLAGS_USERLEVEL) -o $@ -c $<

$(CLIENT_VM_USERLEVEL): echoit.o
	$(CC_USERLEVEL) -static $(CFLAGS_USERLEVEL) $^ -o $@

$(CLIENT_VM_USERLEVEL_INIT): $(EXAMPLE_DIR)/echoserver_init
	cp $^ $@

client_vm/rootfs.cpio.gz: $(SYSTEM_DIR)/rootfs.cpio.gz \
	$(CLIENT_VM_USERLEVEL) $(CLIENT_VM_USERLEVEL_INIT) |client_vm
	$(LIBVMM)/tools/packrootfs $(SYSTEM_DIR)/rootfs.cpio.gz \
		client_vm/rootfs -o $@ \
		--startup $(CLIENT_VM_USERLEVEL_INIT) \
		--home $(CLIENT_VM_USERLEVEL)

client_vm/vm.dts: $(SYSTEM_DIR)/linux.dts $(SYSTEM_DIR)/$(GIC_DT_OVERLAY) \
	$(CHECK_FLAGS_BOARD_MD5) |client_vm
	$(LIBVMM)/tools/dtscat $^ > $@

client_vm/vm.dtb: client_vm/vm.dts |client_vm
	$(DTC) -q -I dts -O dtb $< > $@

client_vm/vmm.o: $(EXAMPLE_DIR)/client_vmm.c $(CHECK_FLAGS_BOARD_MD5) |client_vm
	$(CC) $(CFLAGS) -c -o $@ $<

client_vm/images.o: $(LIBVMM)/tools/package_guest_images.S $(CHECK_FLAGS_BOARD_MD5) \
	${LINUX} client_vm/vm.dtb client_vm/rootfs.cpio.gz
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"${LINUX}\" \
					-DGUEST_DTB_IMAGE_PATH=\"client_vm/vm.dtb\" \
					-DGUEST_INITRD_IMAGE_PATH=\"client_vm/rootfs.cpio.gz\" \
					-target $(TARGET) \
					$(LIBVMM)/tools/package_guest_images.S -o $@

client_vmm%.elf: client_vm/vmm.o client_vm/images.o libsddf_util.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# Need to build libsddf_util_debug.a because it's included in LIBS
# for the unimplemented libc dependencies
${IMAGES}: libsddf_util_debug.a libvmm.a

$(DTB): $(DTS)
	dtc -q -I dts -O dtb $(DTS) > $(DTB)

$(SYSTEM_FILE): $(METAPROGRAM) $(IMAGES) $(DTB) $(CLIENT_DTB)
	$(PYTHON) $(METAPROGRAM) --sddf $(SDDF) --board $(MICROKIT_BOARD) --dtb $(DTB) --client_dtb $(CLIENT_DTB) --output . --sdf $(SYSTEM_FILE)
	$(OBJCOPY) --update-section .device_resources=uart_driver_device_resources.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_driver_config=serial_driver_config.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_virt_tx_config=serial_virt_tx.data serial_virt_tx.elf
	$(OBJCOPY) --update-section .serial_virt_rx_config=serial_virt_rx.data serial_virt_rx.elf
	$(OBJCOPY) --update-section .device_resources=ethernet_driver_device_resources.data eth_driver.elf
	$(OBJCOPY) --update-section .net_driver_config=net_driver.data eth_driver.elf
	$(OBJCOPY) --update-section .net_virt_rx_config=net_virt_rx.data network_virt_rx.elf
	$(OBJCOPY) --update-section .net_virt_tx_config=net_virt_tx.data network_virt_tx.elf
	$(OBJCOPY) --update-section .net_copy_config=net_copy_client0_net_copier.data network_copy.elf network_copy0.elf
	# $(OBJCOPY) --update-section .net_copy_config=net_copy_client1_net_copier.data network_copy.elf network_copy1.elf
	$(OBJCOPY) --update-section .device_resources=timer_driver_device_resources.data timer_driver.elf
	$(OBJCOPY) --update-section .timer_client_config=timer_client_client0.data client_vmm0.elf
	$(OBJCOPY) --update-section .net_client_config=net_client_client0.data client_vmm0.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_client0.data client_vmm0.elf
	# $(OBJCOPY) --update-section .timer_client_config=timer_client_client1.data client_vmm1.elf
	# $(OBJCOPY) --update-section .net_client_config=net_client_client1.data client_vmm1.elf
	# $(OBJCOPY) --update-section .serial_client_config=serial_client_client1.data client_vmm1.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_bench.data benchmark.elf
	$(OBJCOPY) --update-section .vmm_config=vmm_client0.data client_vmm0.elf
	# $(OBJCOPY) --update-section .vmm_config=vmm_client1.data client_vmm1.elf
	$(OBJCOPY) --update-section .benchmark_config=benchmark_config.data benchmark.elf
	$(OBJCOPY) --update-section .benchmark_client_config=benchmark_client_config.data client_vmm0.elf
	$(OBJCOPY) --update-section .benchmark_config=benchmark_idle_config.data idle.elf

${IMAGE_FILE} $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)


include ${SDDF}/util/util.mk
include ${SDDF}/network/components/network_components.mk
include ${ETHERNET_DRIVER}/eth_driver.mk
include ${BENCHMARK}/benchmark.mk
include ${TIMER_DRIVER}/timer_driver.mk
include ${SERIAL_DRIVER}/serial_driver.mk
include ${SERIAL_COMPONENTS}/serial_components.mk
include $(LIBVMM)/vmm.mk

qemu: $(IMAGE_FILE)
	$(QEMU) -machine virt,virtualization=on \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-m size=2G \
			-nographic \
			-device virtio-net-device,netdev=netdev0 \
			-netdev user,id=netdev0,hostfwd=tcp::1236-:1236,hostfwd=tcp::1237-:1237,hostfwd=udp::1235-:1235 \
			-global virtio-mmio.force-legacy=false \
			-d guest_errors

clean::
	${RM} -f *.elf .depend* $
	find . -name \*.[do] |xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f ${IMAGE_FILE} ${REPORT_FILE}

-include $(DEPS)
