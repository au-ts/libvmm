#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
PYTHON ?= python3

LIBVMM_DOWNLOADS := https://trustworthy.systems/Downloads/libvmm/images/

LIBVMM_TOOLS := $(LIBVMM)/tools
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
SDDF_INCLUDE := $(SDDF)/include/sddf
UTIL := $(SDDF)/util

TIMER_DRIVER := $(SDDF)/drivers/timer/$(TIMER_DRIVER)
SERIAL_DRIVER := $(SDDF)/drivers/serial/$(UART_DRIVER)
BLK_DRIVER := $(SDDF)/drivers/blk/$(BLK_DRIVER)
ETH_DRIVER := $(SDDF)/drivers/network/$(ETH_DRIVER)
SERIAL_COMPONENTS := $(SDDF)/serial/components
BLK_COMPONENTS := $(SDDF)/blk/components
NET_COMPONENTS := $(SDDF)/network/components

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
ARCH := ${shell grep 'CONFIG_SEL4_ARCH  ' $(BOARD_DIR)/include/kernel/gen_config.h | cut -d' ' -f4}
SYSTEM_FILE := virtio.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt
DTS_FILE := $(SDDF)/dts/$(MICROKIT_BOARD).dts
DTB_FILE := $(MICROKIT_BOARD).dtb
CLIENT_DTB := client_vm/vm.dtb
METAPROGRAM := $(VIRTIO_EXAMPLE)/meta.py
SYSTEM_DIR := $(VIRTIO_EXAMPLE)/client_vm/$(ARCH)

SDDF_CUSTOM_LIBC := 1

CLIENT_VM_USERLEVEL_INIT := blk_client_init net_client_init
CLIENT_VM_USERLEVEL_HOME := $(LIBVMM_TOOLS)/linux/blk/blk_integration_tests.sh $(LIBVMM_TOOLS)/linux/blk/blk_bench.sh

ifeq ($(ARCH),aarch64)
	LINUX ?= 85000f3f42a882e4476e57003d53f2bbec8262b0-linux
	INITRD := b6a276df6a0e39f76bc8950e975daa2888ad83df-rootfs.cpio.gz
	ARCH_FLAGS := -target aarch64-none-elf -mstrict-align
	VMM_NAME := client_vmm_aarch64
	IMAGES = $(VMM_NAME).elf

        QEMU := qemu-system-aarch64
	QEMU_ARCH_ARGS := -machine virt,virtualization=on,secure=off \
					  -cpu cortex-a53 \
					  -device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
					  -device virtio-blk-device,drive=drive0,id=virtblk0,num-queues=1 \
					  -device virtio-net-device,netdev=netdev0 \

else ifeq ($(ARCH),x86_64)
	LINUX ?= $(SYSTEM_DIR)/bzImage
	INITRD ?= $(SYSTEM_DIR)/rootfs.cpio.gz

	ARCH_FLAGS := -target x86_64-unknown-elf
	VMM_NAME := client_vmm_x86_64
	IMAGES = $(VMM_NAME).elf timer_driver.elf

	KERNEL = sel4.elf
	KERNEL32 := sel4_32.elf

        QEMU := qemu-system-x86_64

	QEMU_ARCH_ARGS := -accel kvm -cpu host,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme \
					  -kernel $(KERNEL32) -initrd $(IMAGE_FILE) -vga none \
					  -device virtio-net-pci,netdev=netdev0,addr=0x2.0 \
					  -device virtio-blk-pci,drive=drive0,id=virtblk0,num-queues=1,addr=0x3.0

else
$(error Unsupported ARCH given)
endif

vpath %.c $(SDDF) $(LIBVMM) $(VIRTIO_EXAMPLE) $(NETWORK_COMPONENTS)

CFLAGS := \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -DBOARD_$(MICROKIT_BOARD) \
	  -I$(BOARD_DIR)/include \
	  -I$(SDDF)/include \
	  -I$(SDDF)/include/microkit \
	  -I$(LIBVMM)/include \
	  -I$(VIRTIO_EXAMPLE)/include \
	  -MD \
	  -MP \
	  $(ARCH_FLAGS)

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld libsddf_util_debug.a libvmm.a --end-group

include $(SDDF)/util/util.mk
include $(TIMER_DRIVER)/timer_driver.mk
include $(SERIAL_DRIVER)/serial_driver.mk
include $(SERIAL_COMPONENTS)/serial_components.mk
include ${BLK_DRIVER}/blk_driver.mk
include $(BLK_COMPONENTS)/blk_components.mk
include ${ETH_DRIVER}/eth_driver.mk
include $(NET_COMPONENTS)/network_components.mk
include $(LIBVMM)/vmm.mk
include $(LIBVMM_TOOLS)/linux/uio/uio.mk
include $(LIBVMM_TOOLS)/linux/blk/blk_init.mk
include $(LIBVMM_TOOLS)/linux/net/net_init.mk

IMAGES += client_vmm.elf timer_driver.elf blk_driver.elf blk_virt.elf serial_driver.elf serial_virt_tx.elf serial_virt_rx.elf \
	network_virt_rx.elf network_virt_tx.elf eth_driver.elf network_copy.elf eth_driver_virtio.elf

CHECK_FLAGS_BOARD_MD5 := .board_cflags-$(shell echo -- $(CFLAGS) $(BOARD) $(MICROKIT_CONFIG) | shasum | sed 's/ *-//')

$(CHECK_FLAGS_BOARD_MD5):
	-rm -f .board_cflags-*
	touch $@

all: loader.img

-include vmm.d

$(IMAGES): libsddf_util_debug.a libvmm.a

$(DTB_FILE): $(DTS_FILE)
	$(DTC) -q -I dts -O dtb $< > $@

eth_driver.elf: eth_driver_virtio.elf
	cp eth_driver_virtio.elf eth_driver.elf

ifeq ($(ARCH),x86_64)
$(SYSTEM_FILE): $(METAPROGRAM) $(IMAGES)
else
$(SYSTEM_FILE): $(METAPROGRAM) $(IMAGES) $(DTB_FILE) $(CLIENT_DTB)
endif

ifeq ($(ARCH),x86_64)
	$(PYTHON) $(METAPROGRAM) --sddf $(SDDF) --board $(MICROKIT_BOARD) --output . --sdf $(SYSTEM_FILE) $(PARTITION_ARG)
else
	$(PYTHON) $(METAPROGRAM) --sddf $(SDDF) --board $(MICROKIT_BOARD) --dtb $(DTB_FILE) --client-dtb $(CLIENT_DTB) --output . --sdf $(SYSTEM_FILE) $(PARTITION_ARG)
endif

ifeq ($(MICROKIT_BOARD), maaxboard)
	$(OBJCOPY) --update-section .device_resources=timer_driver_device_resources.data timer_driver.elf
	$(OBJCOPY) --update-section .timer_client_config=timer_client_blk_driver.data blk_driver.elf
endif
	$(OBJCOPY) --update-section .device_resources=blk_driver_device_resources.data blk_driver.elf
	$(OBJCOPY) --update-section .blk_driver_config=blk_driver.data blk_driver.elf
	$(OBJCOPY) --update-section .blk_virt_config=blk_virt.data blk_virt.elf
	$(OBJCOPY) --update-section .blk_client_config=blk_client_CLIENT_VMM.data client_vmm.elf
	$(OBJCOPY) --update-section .device_resources=serial_driver_device_resources.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_driver_config=serial_driver_config.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_virt_rx_config=serial_virt_rx.data serial_virt_rx.elf
	$(OBJCOPY) --update-section .serial_virt_tx_config=serial_virt_tx.data serial_virt_tx.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_CLIENT_VMM.data client_vmm.elf
	$(OBJCOPY) --update-section .vmm_config=vmm_CLIENT_VMM.data client_vmm.elf
	$(OBJCOPY) --update-section .device_resources=eth_driver_device_resources.data eth_driver.elf
	$(OBJCOPY) --update-section .net_driver_config=net_driver.data eth_driver.elf
	$(OBJCOPY) --update-section .net_virt_rx_config=net_virt_rx.data network_virt_rx.elf
	$(OBJCOPY) --update-section .net_virt_tx_config=net_virt_tx.data network_virt_tx.elf
	$(OBJCOPY) --update-section .net_copy_config=net_copy_client0_net_copier.data network_copy.elf network_copy.elf
	$(OBJCOPY) --update-section .net_client_config=net_client_CLIENT_VMM.data client_vmm.elf

$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) \
		--config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

.PHONY: vm_dir
vm_dir:
	mkdir -p client_vm

${LINUX}:
	curl -L ${LIBVMM_DOWNLOADS}/$(LINUX).tar.gz -o $(LINUX).tar.gz
	mkdir -p linux_download_dir
	tar -xf $@.tar.gz -C linux_download_dir
	cp linux_download_dir/${LINUX}/linux ${LINUX}

${INITRD}:
	curl -L ${LIBVMM_DOWNLOADS}/$(INITRD).tar.gz -o $(INITRD).tar.gz
	mkdir -p initrd_download_dir
	tar xf $@.tar.gz -C initrd_download_dir
	cp initrd_download_dir/${INITRD}/rootfs.cpio.gz ${INITRD}

client_vm/rootfs.cpio.gz: ${INITRD} \
	$(CLIENT_VM_USERLEVEL_INIT) $(CLIENT_VM_USERLEVEL_HOME) |client_vm
	$(LIBVMM)/tools/packrootfs ${INITRD} \
		client_vm/rootfs_staging -o $@ \
		--startup $(CLIENT_VM_USERLEVEL_INIT) \
		--home $(CLIENT_VM_USERLEVEL_HOME)

blk_storage:
	$(LIBVMM_TOOLS)/mkvirtdisk $@ $(BLK_NUM_PART) $(BLK_SIZE) $(BLK_MEM)

client_vm/vmm.o: $(VIRTIO_EXAMPLE)/$(VMM_NAME).c $(CHECK_FLAGS_BOARD_MD5) |vm_dir
	$(CC) $(CFLAGS) -c -o $@ $<

ifeq ($(ARCH),x86_64)
client_vm/images.o: $(LIBVMM)/tools/package_guest_images.S $(LINUX) client_vm/rootfs.cpio.gz
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"${LINUX}\" \
					-DGUEST_INITRD_IMAGE_PATH=\"client_vm/rootfs.cpio.gz\" \
					$(ARCH_FLAGS) \
					$(LIBVMM)/tools/package_guest_images.S -o $@
else
client_vm/vm.dts: $(SYSTEM_DIR)/linux.dts $(SYSTEM_DIR)/$(GIC_DT_OVERLAY) \
	$(CHECK_FLAGS_BOARD_MD5) |vm_dir
	$(LIBVMM)/tools/dtscat $^ > $@

client_vm/vm.dtb: client_vm/vm.dts
	$(DTC) -q -I dts -O dtb $< > $@

client_vm/images.o: $(LIBVMM)/tools/package_guest_images.S $(CHECK_FLAGS_BOARD_MD5) \
	${LINUX} client_vm/vm.dtb client_vm/rootfs.cpio.gz
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"${LINUX}\" \
					-DGUEST_DTB_IMAGE_PATH=\"client_vm/vm.dtb\" \
					-DGUEST_INITRD_IMAGE_PATH=\"client_vm/rootfs.cpio.gz\" \
					$(ARCH_FLAGS) \
					$(LIBVMM)/tools/package_guest_images.S -o $@
endif

client_vmm.elf: client_vm/vmm.o client_vm/images.o |vm_dir
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# Stop make from deleting intermediate files
.PRECIOUS: client_vm client_vm/vm.dts client_vm/vm.dtb \
	client_vm/rootfs.cpio.gz client_vm/images.o client_vm/vmm.o

qemu: $(IMAGE_FILE) blk_storage
	# [ ${MICROKIT_BOARD} = qemu_virt_aarch64 ]
	$(QEMU) $(QEMU_ARCH_ARGS) -serial mon:stdio \
							  -m size=3G \
							  -nographic \
							  -global virtio-mmio.force-legacy=false \
							  -drive file=blk_storage,format=raw,if=none,id=drive0 \
							  -netdev user,id=netdev0,hostfwd=tcp::1236-:1236,hostfwd=tcp::1237-:1237,hostfwd=udp::1235-:1235

clean::
	$(RM) -f *.elf .depend* $
	find . -name \*.[do] -type f |xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f $(IMAGE_FILE) $(REPORT_FILE)
