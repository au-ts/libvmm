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

ifeq ($(strip $(MICROKIT_BOARD)), odroidc4)
	export UART_DRIVER := meson
	export CPU := cortex-a55
else ifeq ($(strip $(MICROKIT_BOARD)), qemu_virt_aarch64)
	export UART_DRIVER := arm
	export CPU := cortex-a53
	QEMU := qemu-system-aarch64
else
$(error Unsupported MICROKIT_BOARD given)
endif

export BUILD_DIR:=$(abspath $(BUILD_DIR))
export MICROKIT_SDK:=$(abspath $(MICROKIT_SDK))
export VIRTIO_EXAMPLE:=$(abspath .)

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

# If on macOS, override backend to `coreaudio`
export QEMU_SND_BACKEND := alsa
export QEMU_SND_FRONTEND := hda

IMAGE_FILE := $(BUILD_DIR)/loader.img
REPORT_FILE := $(BUILD_DIR)/report.txt

all: $(IMAGE_FILE)

qemu $(IMAGE_FILE) $(REPORT_FILE) clean clobber: $(BUILD_DIR)/Makefile FORCE
	$(MAKE) -C $(BUILD_DIR) MICROKIT_SDK=$(MICROKIT_SDK) $(notdir $@)

$(BUILD_DIR)/Makefile: virtio_snd.mk
	mkdir -p $(BUILD_DIR)
	cp virtio_snd.mk $@

FORCE:
