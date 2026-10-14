#
# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
# This Makefile is copied into the build directory
# and operated on from there.
#

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif

ifeq ($(strip $(SDDF)),)
$(error SDDF must be specified)
endif

BUILD_DIR ?= build
MICROKIT_CONFIG ?= debug
IMAGE_FILE = loader.img
REPORT_FILE = report.txt

SYSTEM_DIR := $(EXAMPLE_DIR)/board/$(MICROKIT_BOARD)
BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
ARCH := ${shell grep 'CONFIG_SEL4_ARCH  ' $(BOARD_DIR)/include/kernel/gen_config.h | cut -d' ' -f4}
SDDF_CUSTOM_LIBC := 1

DTC := dtc
PYTHON ?= python3
OBJCOPY := llvm-objcopy

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

TIMER_DRV_DIR := meson

METAPROGRAM := $(EXAMPLE_DIR)/meta.py

TIMER_DRIVER := $(SDDF)/drivers/timer/$(TIMER_DRV_DIR)
SYSTEM_FILE := mibench.system
DTS := $(SDDF)/dts/$(MICROKIT_BOARD).dts
DTB := $(MICROKIT_BOARD).dtb

vpath %.c $(LIBVMM) $(EXAMPLE_DIR) $(SDDF)

IMAGES := vmm.elf timer_driver.elf

CFLAGS := $(CFLAGS_ARCH) \
	  -nostdlib \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -DBOARD_$(MICROKIT_BOARD) \
	  -I$(BOARD_DIR)/include \
	  -I$(LIBVMM)/include \
	  -I$(SDDF)/include/microkit \
	  -I$(SDDF)/include \
	  -I$(SDDF)/include/sddf/util/custom_libc \
	  -MD \
	  -MP \
	  -target $(TARGET)

ifdef NOWFI
CFLAGS += -DNOWFI
endif

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld libvmm.a libsddf_util_debug.a --end-group

CHECK_FLAGS_BOARD_MD5 := .board_cflags-$(shell echo -- ${CFLAGS} ${MICROKIT_SDK} ${MICROKIT_BOARD} ${MICROKIT_CONFIG} | shasum | sed 's/ *-//')

${CHECK_FLAGS_BOARD_MD5}:
	-rm -f .board_cflags-*
	touch $@

vmm.elf: vmm.o images.o
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

all: loader.img

-include vmm.d

$(IMAGES): libvmm.a libsddf_util_debug.a

LINUX ?= linux
LINUX_IMAGE ?= /home/alexkv/thesis/linux/arch/arm64/boot/Image

${LINUX}: ${LINUX_IMAGE}
	cp $< $@

INITRD ?= initrd
ROOTFS ?= /home/alexkv/thesis/buildroot-2022.08-rc2/output/images/rootfs.cpio.gz

${INITRD}: ${ROOTFS}
	cp $< $@

vm.dts: $(SYSTEM_DIR)/linux.dts $(SYSTEM_DIR)/overlay.dts $(CHECK_FLAGS_BOARD_MD5)
	$(LIBVMM)/tools/dtscat $^ > $@

vm.dtb: vm.dts $(CHECK_FLAGS_BOARD_MD5)
	$(DTC) -q -I dts -O dtb $< > $@

vmm.o: $(EXAMPLE_DIR)/vmm.c $(CHECK_FLAGS_BOARD_MD5)
	$(CC) $(CFLAGS) -c -o $@ $<

images.o: $(LIBVMM)/tools/package_guest_images.S $(LINUX) $(INITRD) vm.dtb
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"${LINUX}\" \
					-DGUEST_DTB_IMAGE_PATH=\"vm.dtb\" \
					-DGUEST_INITRD_IMAGE_PATH=\"${INITRD}\" \
					-target $(TARGET) \
					$(LIBVMM)/tools/package_guest_images.S -o $@

$(DTB): $(DTS)
	dtc -q -I dts -O dtb $(DTS) > $(DTB)

$(SYSTEM_FILE): $(METAPROGRAM) $(IMAGES) $(DTB) vm.dtb
	$(PYTHON) $(METAPROGRAM) --sddf $(SDDF) --board $(MICROKIT_BOARD) --dtb_native $(DTB) \
	--dtb_guest vm.dtb --output . --sdf $(SYSTEM_FILE)
	$(OBJCOPY) --update-section .timer_client_config=timer_client_mibench_pd.data vmm.elf
	$(OBJCOPY) --update-section .device_resources=timer_driver_device_resources.data timer_driver.elf

${IMAGE_FILE} $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)


include ${TIMER_DRIVER}/timer_driver.mk
include ${SDDF}/util/util.mk
include $(LIBVMM)/vmm.mk

clean::
	${RM} -f *.elf .depend*
	find . -name \*.[do] |xargs --no-run-if-empty rm

clobber:: clean
	rm -f ${IMAGE_FILE} ${REPORT_FILE}
