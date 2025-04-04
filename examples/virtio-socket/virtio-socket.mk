#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
QEMU := qemu-system-aarch64
PYTHON ?= python3

LIBVMM_DOWNLOADS := https://trustworthy.systems/Downloads/libvmm/images/

LIBVMM_TOOLS := $(LIBVMM)/tools
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
SDDF_INCLUDE := $(SDDF)/include/sddf
UTIL := $(SDDF)/util

SERIAL_DRIVER := $(SDDF)/drivers/serial/$(UART_DRIVER)
SERIAL_COMPONENTS := $(SDDF)/serial/components

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
SYSTEM_FILE := virtio.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt
DTS_FILE := $(SDDF)/dts/$(MICROKIT_BOARD).dts
DTB_FILE := $(MICROKIT_BOARD).dtb
CLIENT_VM := $(VIRTIO_EXAMPLE)/client_vm
CLIENT_DTB := client_vm/vm.dtb
METAPROGRAM := $(VIRTIO_EXAMPLE)/meta.py

CLIENT_VM_USERLEVEL_INIT := blk_client_init

vpath %.c $(SDDF) $(LIBVMM) $(VIRTIO_EXAMPLE)

CFLAGS := \
	  -mstrict-align \
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
	  -target $(TARGET)

CFLAGS_USERLEVEL := \
		-g3 \
		-O3 \
		-Wno-unused-command-line-argument \
		-Wall -Wno-unused-function -Werror \
		-D_GNU_SOURCE \
		-I$(VIRTIO_EXAMPLE)/include \
		-target aarch64-linux-musl

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld libsddf_util_debug.a libvmm.a --end-group

include $(SDDF)/util/util.mk
include $(SERIAL_DRIVER)/serial_driver.mk
include $(SERIAL_COMPONENTS)/serial_components.mk
include $(LIBVMM)/vmm.mk
include $(LIBVMM_TOOLS)/linux/uio/uio.mk

IMAGES := client_vmm0.elf client_vmm1.elf serial_driver.elf serial_virt_tx.elf serial_virt_rx.elf

CHECK_FLAGS_BOARD_MD5:=.board_cflags-$(shell echo -- $(CFLAGS) $(BOARD) $(MICROKIT_CONFIG) | shasum | sed 's/ *-//')

$(CHECK_FLAGS_BOARD_MD5):
	-rm -f .board_cflags-*
	touch $@

all: loader.img

-include vmm.d

$(IMAGES): libsddf_util_debug.a libvmm.a

$(DTB_FILE): $(DTS_FILE)
	$(DTC) -q -I dts -O dtb $< > $@

$(SYSTEM_FILE): $(METAPROGRAM) $(IMAGES) $(DTB_FILE) $(CLIENT_DTB)
	$(PYTHON) $(METAPROGRAM) --sddf $(SDDF) --board $(MICROKIT_BOARD) --dtb $(DTB_FILE) --client_dtb $(CLIENT_DTB) --output . --sdf $(SYSTEM_FILE) $(PARTITION_ARG)

	$(OBJCOPY) --update-section .device_resources=serial_driver_device_resources.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_driver_config=serial_driver_config.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_virt_rx_config=serial_virt_rx.data serial_virt_rx.elf
	$(OBJCOPY) --update-section .serial_virt_tx_config=serial_virt_tx.data serial_virt_tx.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_CLIENT_VMM0.data client_vmm0.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_CLIENT_VMM1.data client_vmm1.elf

	$(OBJCOPY) --update-section .vmm_config=vmm_CLIENT_VMM0.data client_vmm0.elf
	$(OBJCOPY) --update-section .vmm_config=vmm_CLIENT_VMM1.data client_vmm1.elf

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

vsock_send.elf: $(VIRTIO_EXAMPLE)/client_vm_userlevel/vsock_send.c
	zig cc $^ -target aarch64-linux-musl -o $@

vsock_recv.elf: $(VIRTIO_EXAMPLE)/client_vm_userlevel/vsock_recv.c
	zig cc $^ -target aarch64-linux-musl -o $@

client_vm/rootfs_sender.cpio.gz: ${INITRD} vsock_send.elf $(VIRTIO_EXAMPLE)/client_vm_userlevel/sender_init.sh | client_vm
	$(LIBVMM)/tools/packrootfs ${INITRD} \
		client_vm/rootfs_staging -o $@ \
		--home vsock_send.elf \
		--startup $(VIRTIO_EXAMPLE)/client_vm_userlevel/sender_init.sh

client_vm/rootfs_recver.cpio.gz: ${INITRD} vsock_recv.elf $(VIRTIO_EXAMPLE)/client_vm_userlevel/recver_init.sh | client_vm
	$(LIBVMM)/tools/packrootfs ${INITRD} \
		client_vm/rootfs_staging -o $@ \
		--home vsock_recv.elf \
		--startup $(VIRTIO_EXAMPLE)/client_vm_userlevel/recver_init.sh

client_vm/vm.dts: $(CLIENT_VM)/linux.dts $(CLIENT_VM)/$(GIC_DT_OVERLAY) \
	$(CHECK_FLAGS_BOARD_MD5) |vm_dir
	$(LIBVMM)/tools/dtscat $^ > $@

client_vm/vm.dtb: client_vm/vm.dts
	$(DTC) -q -I dts -O dtb $< > $@

client_vm/vmm0.o: $(VIRTIO_EXAMPLE)/client_vmm.c $(CHECK_FLAGS_BOARD_MD5) |vm_dir
	$(CC) $(CFLAGS) -DVIRTIO_VSOCK_GUEST_CID=3 -c -o $@ $<

client_vm/vmm1.o: $(VIRTIO_EXAMPLE)/client_vmm.c $(CHECK_FLAGS_BOARD_MD5) |vm_dir
	$(CC) $(CFLAGS) -DVIRTIO_VSOCK_GUEST_CID=4 -c -o $@ $<

client_vm/image_sender.o: $(LIBVMM)/tools/package_guest_images.S $(CHECK_FLAGS_BOARD_MD5) \
	${LINUX} client_vm/vm.dtb client_vm/rootfs_sender.cpio.gz
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"${LINUX}\" \
					-DGUEST_DTB_IMAGE_PATH=\"client_vm/vm.dtb\" \
					-DGUEST_INITRD_IMAGE_PATH=\"client_vm/rootfs_sender.cpio.gz\" \
					-target $(TARGET) \
					$(LIBVMM)/tools/package_guest_images.S -o $@

client_vm/image_recver.o: $(LIBVMM)/tools/package_guest_images.S $(CHECK_FLAGS_BOARD_MD5) \
	${LINUX} client_vm/vm.dtb client_vm/rootfs_recver.cpio.gz
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"${LINUX}\" \
					-DGUEST_DTB_IMAGE_PATH=\"client_vm/vm.dtb\" \
					-DGUEST_INITRD_IMAGE_PATH=\"client_vm/rootfs_recver.cpio.gz\" \
					-target $(TARGET) \
					$(LIBVMM)/tools/package_guest_images.S -o $@

client_vmm0.elf: client_vm/vmm0.o client_vm/image_recver.o |vm_dir
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

client_vmm1.elf: client_vm/vmm1.o client_vm/image_sender.o |vm_dir
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# Stop make from deleting intermediate files
.PRECIOUS: client_vm client_vm/vm.dts client_vm/vm.dtb \
	client_vm/rootfs.cpio.gz client_vm/images.o client_vm/vmm.o

qemu: $(IMAGE_FILE)
	[ ${MICROKIT_BOARD} = qemu_virt_aarch64 ]
	$(QEMU) -machine virt,virtualization=on,secure=off \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-m size=2G \
			-nographic \
			-global virtio-mmio.force-legacy=false

clean::
	$(RM) -f *.elf .depend* $
	find . -name \*.[do] -type f |xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f $(IMAGE_FILE) $(REPORT_FILE)
