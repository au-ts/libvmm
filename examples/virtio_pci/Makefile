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
export override MICROKIT_SDK:=$(abspath $(MICROKIT_SDK))

# Both QEMU and Maaxboard uses the same guest device tree as the guest does not
# need to access any physical device. The only difference is that QEMU is GICv2 while
# the Maaxboard is GICv3. We apply a GIC overlay for the correct version on the board
# at build time.
# TODO: we should probably determine this better if we have lots of supported platforms.
ifeq ($(strip $(MICROKIT_BOARD)), maaxboard)
	export UART_DRIVER := imx
	export TIMER_DRIVER := imx
	export BLK_DRIVER := mmc/imx
	export ETH_DRIVER := imx
	export CPU := cortex-a55
	export GIC_DT_OVERLAY := gic_v3_overlay.dts
else ifeq ($(strip $(MICROKIT_BOARD)), qemu_virt_aarch64)
	export UART_DRIVER := arm
	export TIMER_DRIVER := arm
	export BLK_DRIVER := virtio
	export ETH_DRIVER := virtio
	export CPU := cortex-a53
	export GIC_DT_OVERLAY := gic_v2_overlay.dts
	QEMU := qemu-system-aarch64

	export BLK_NUM_PART = 1
	export BLK_SIZE = 512
	# 16MiB of disk space
	export BLK_MEM ?= 16777216
else
$(error Unsupported MICROKIT_BOARD given)
endif

# Allow to user to specify a custom partition
PARTITION :=
ifdef PARTITION
export PARTITION_ARG := --partition $(PARTITION)
endif

# All platforms use the same Linux and initrd images.
export LINUX := a3f4bf9e2eb24fa8fc0d3d8cd02e4d8097062e8b-linux
export INITRD := b6a276df6a0e39f76bc8950e975daa2888ad83df-rootfs.cpio.gz

export BUILD_DIR:=$(abspath $(BUILD_DIR))
export MICROKIT_SDK:=$(abspath $(MICROKIT_SDK))
export VIRTIO_EXAMPLE:=$(abspath .)

export OBJCOPY := llvm-objcopy
export TARGET := aarch64-none-elf
export CC := clang
export CC_USERLEVEL := zig cc
export LD := ld.lld
export AS := llvm-as
export AR := llvm-ar
export DTC := dtc
export RANLIB := llvm-ranlib
export MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
export SDDF=$(abspath ../../dep/sddf)
export LIBVMM=$(abspath ../../)

IMAGE_FILE := $(BUILD_DIR)/loader.img
REPORT_FILE := $(BUILD_DIR)/report.txt

all: $(IMAGE_FILE)

qemu $(IMAGE_FILE) $(REPORT_FILE) clean clobber: $(BUILD_DIR)/Makefile FORCE
	$(MAKE) -C $(BUILD_DIR) MICROKIT_SDK=$(MICROKIT_SDK) $(notdir $@)

$(BUILD_DIR)/Makefile: virtio.mk
	mkdir -p $(BUILD_DIR)
	cp virtio.mk $@

FORCE:
