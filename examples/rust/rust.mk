#
# Copyright 2021, Breakaway Consulting Pty. Ltd.
# Copyright 2025, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif

# Download the Linux kernel and initrd from Trustworthy Systems website
LINUX ?= 85000f3f42a882e4476e57003d53f2bbec8262b0-linux
INITRD ?= 6dcd1debf64e6d69b178cd0f46b8c4ae7cebe2a5-rootfs.cpio.gz

# Specify that we use bash for all shell commands
# TODO: do we need this?
# SHELL=/bin/bash
QEMU := qemu-system-aarch64
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

vpath %.c $(LIBVMM) $(EXAMPLE_DIR)

IMAGES := vmm.elf

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
SYSTEM_FILE := $(EXAMPLE_DIR)/rust_vmm.system

DTS := $(EXAMPLE_DIR)/images/linux.dts
DTB := linux.dtb

IMAGE_FILE = loader.img
REPORT_FILE = report.txt

# Toolchain flags
CFLAGS := \
	  -mstrict-align \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -DBOARD_$(MICROKIT_BOARD) \
	  -I$(BOARD_DIR)/include \
	  -I$(LIBVMM)/include \
	  -I$(SDDF)/include \
	  -I$(SDDF)/include/microkit \
	  -MD \
	  -MP \
	  -target $(TARGET)

RUST_TARGET_PATH := $(EXAMPLE_DIR)
RUST_MICROKIT_TARGET := aarch64-microkit-minimal
TARGET_DIR := $(BUILD_DIR)/target

RUST_ENV = \
	RUST_TARGET_PATH=$(abspath $(RUST_TARGET_PATH)) \
	SEL4_INCLUDE_DIRS=$(abspath $(BOARD_DIR)/include) \
	MICROKIT_BOARD_DIR=$(abspath $(BOARD_DIR)) \
	BUILD_DIR=$(abspath $(BUILD_DIR)) \
	LINUX_PATH=$(abspath $(LINUX)) \
	INITRD_PATH=$(abspath $(INITRD))

RUST_OPTIONS := \
	-Z unstable-options \
	-Z build-std=core,alloc,compiler_builtins \
	-Z build-std-features=compiler-builtins-mem \
	-Z bindeps \
	--release \
	--manifest-path $(EXAMPLE_DIR)/Cargo.toml \
	--target $(RUST_MICROKIT_TARGET) \
	--target-dir $(abspath $(TARGET_DIR)) \
	--out-dir $(abspath $(BUILD_DIR)) \

all: $(IMAGE_FILE)

-include vmm.d

$(IMAGES): libvmm.a

qemu: $(IMAGE_FILE)
	$(QEMU) -machine virt,virtualization=on,highmem=off,secure=off \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-m size=2G \
			-nographic

${LINUX}:
	curl -L https://trustworthy.systems/Downloads/libvmm/images/${LINUX}.tar.gz -o $(BUILD_DIR)/$@.tar.gz
	mkdir -p $(BUILD_DIR)/linux_download_dir
	tar -xf $(BUILD_DIR)/$@.tar.gz -C $(BUILD_DIR)/linux_download_dir
# This eval step is to propagate the image's absolute path into RUST_OPTIONS
# so that src/vmm.rs can include_bytes! the correct file.
	$(eval LINUX=$(BUILD_DIR)/linux_download_dir/${LINUX}/linux)

${INITRD}:
	curl -L https://trustworthy.systems/Downloads/libvmm/images/${INITRD}.tar.gz -o $(BUILD_DIR)/$@.tar.gz
	mkdir -p $(BUILD_DIR)/initrd_download_dir
	tar xf $(BUILD_DIR)/$@.tar.gz -C $(BUILD_DIR)/initrd_download_dir
	$(eval INITRD=$(BUILD_DIR)/initrd_download_dir/${INITRD}/rootfs.cpio.gz)

$(DTB): $(DTS)
	$(DTC) -q -I dts -O dtb $< > $@

include $(LIBVMM)/vmm.mk

vmm.elf: ${EXAMPLE_DIR}/src/vmm.rs ${LINUX} ${INITRD} $(DTB)
	cp ${EXAMPLE_DIR}/rust-toolchain.toml .
	$(RUST_ENV) \
		cargo build $(RUST_OPTIONS)

$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)
