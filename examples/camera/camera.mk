#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
QEMU := qemu-system-aarch64

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
SYSTEM_DIR := $(EXAMPLE_DIR)/board/$(MICROKIT_BOARD)
SYSTEM_FILE := $(SYSTEM_DIR)/camera.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt

vpath %.c $(LIBVMM) $(EXAMPLE_DIR)

IMAGES := vmm.elf camera_client.elf

CFLAGS := \
	  -mstrict-align \
	  -ffreestanding \
	  -g3 -O0 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -DBOARD_$(MICROKIT_BOARD) \
	  -I$(BOARD_DIR)/include \
	  -I$(LIBVMM)/include \
	  -I$(SDDF)/include \
	  -I$(SDDF)/libco \
	  -MD \
	  -MP \
	  -target $(TARGET)

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld libvmm.a --end-group

CHECK_FLAGS_BOARD_MD5:=.board_cflags-$(shell echo -- $(CFLAGS) $(BOARD) $(MICROKIT_CONFIG) | shasum | sed 's/ *-//')

$(CHECK_FLAGS_BOARD_MD5):
	-rm -f .board_cflags-*
	touch $@

vmm.elf: vmm.o images.o
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

camera_client.elf: camera_client.o libco.a images.o
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

all: loader.img

-include vmm.d

$(IMAGES): libvmm.a libco.a

$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

#rootfs.cpio.gz: $(SYSTEM_DIR)/rootfs.cpio.gz
#	cp $(SYSTEM_DIR)/rootfs.cpio.gz $(BUILD_DIR)/rootfs.cpio.gz
initramsfs8: $(SYSTEM_DIR)/initramfs8
	cp $(SYSTEM_DIR)/initramfs8 $(BUILD_DIR)/initramfs8

vm.dts: $(SYSTEM_DIR)/linux.dts $(SYSTEM_DIR)/overlay.dts
	$(LIBVMM)/tools/dtscat $^ > $@

vm.dtb: vm.dts
	$(DTC) -q -I dts -O dtb $< > $@

camera_client.o: $(EXAMPLE_DIR)/board/rpi4b_1gb/client/camera_client.c $(CHECK_FLAGS_BOARD_MD5)
	$(CC) $(CFLAGS) -c -o $@ $<

vmm.o: $(EXAMPLE_DIR)/camera_vmm.c $(CHECK_FLAGS_BOARD_MD5)
	$(CC) $(CFLAGS) -c -o $@ $<

images.o: $(LIBVMM)/tools/package_guest_images.S $(SYSTEM_DIR)/linux vm.dtb initramsfs8
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(SYSTEM_DIR)/linux\" \
					-DGUEST_DTB_IMAGE_PATH=\"vm.dtb\" \
					-DGUEST_INITRD_IMAGE_PATH=\"initramfs8\" \
					-target $(TARGET) \
					$(LIBVMM)/tools/package_guest_images.S -o $@

include $(LIBVMM)/vmm.mk
include $(SDDF)/libco/libco.mk

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
