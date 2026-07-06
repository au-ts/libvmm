#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

BUILD_DIR ?= build
export MICROKIT_CONFIG ?= debug

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif
override MICROKIT_SDK := $(abspath $(MICROKIT_SDK))

# Both QEMU and Maaxboard uses the same guest device tree as the guest does not
# need to access any physical device. The only difference is that QEMU is GICv2 while
# the Maaxboard is GICv3. We apply a GIC overlay for the correct version on the board
# at build time.
# TODO: we should probably determine this better if we have lots of supported platforms.
ifeq ($(strip $(MICROKIT_BOARD)), maaxboard)
	export GIC_DT_OVERLAY := gic_v3_overlay.dts
else ifeq ($(strip $(MICROKIT_BOARD)), qemu_virt_aarch64)
	export GIC_DT_OVERLAY := gic_v2_overlay.dts
endif

# create a virtual disk image for QEMU targets
export BLK_NUM_PART = 1
export BLK_SIZE = 512
# 16MiB of disk space
export BLK_MEM ?= 16777216

# Allow to user to specify a custom partition
PARTITION :=
ifdef PARTITION
export PARTITION_ARG := --partition $(PARTITION)
endif

BUILD_DIR ?= build
export BUILD_DIR := $(abspath ${BUILD_DIR})
export VIRTIO_EXAMPLE := $(abspath .)

export CC_USERLEVEL := clang
export MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
export SDDF := $(abspath ../../dep/sddf)
export LIBVMM := $(abspath ../../)

IMAGE_FILE := $(BUILD_DIR)/loader.img
REPORT_FILE := $(BUILD_DIR)/report.txt

all: $(IMAGE_FILE)

qemu $(IMAGE_FILE) $(REPORT_FILE) clean clobber: $(BUILD_DIR)/Makefile FORCE
	$(MAKE) -C $(BUILD_DIR) MICROKIT_SDK=$(MICROKIT_SDK) $(notdir $@)

$(BUILD_DIR)/Makefile: virtio_pci.mk
	mkdir -p $(BUILD_DIR)
	cp virtio_pci.mk $@

FORCE:
