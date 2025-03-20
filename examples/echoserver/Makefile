# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

BUILD_DIR ?= build
export MICROKIT_CONFIG ?= debug

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif
export override MICROKIT_SDK:=$(abspath ${MICROKIT_SDK})

ifeq ($(strip $(TOOLCHAIN)),)
	TOOLCHAIN := aarch64-none-elf
	export TOOLCHAIN := aarch64-none-elf
	export LIBC := $(dir $(realpath $(shell aarch64-none-elf-gcc --print-file-name libc.a)))
endif

ifeq ($(strip $(MICROKIT_BOARD)), qemu_virt_aarch64)
	export DRIV_DIR := virtio
	export SERIAL_DRIV_DIR := arm
	export TIMER_DRV_DIR := arm
	export ETH_DRV_DIR := virtio
	export CPU := cortex-a53
	export GIC_DT_OVERLAY := gic_v2_overlay.dts
else ifeq ($(strip $(MICROKIT_BOARD)), odroidc4)
	export DRIV_DIR := meson
	export SERIAL_DRIV_DIR := meson
	export TIMER_DRV_DIR := meson
	export ETH_DRV_DIR := meson
	export CPU := cortex-a55
	export GIC_DT_OVERLAY := gic_v3_overlay.dts
else ifeq ($(strip $(MICROKIT_BOARD)), maaxboard)
	export DRIV_DIR := imx
	export SERIAL_DRIV_DIR := imx
	export TIMER_DRV_DIR := imx
	export ETH_DRV_DIR := imx
	export CPU := cortex-a53
	export GIC_DT_OVERLAY := gic_v3_overlay.dts
endif

# All platforms use the same Linux and initrd images.
export LINUX := a3f4bf9e2eb24fa8fc0d3d8cd02e4d8097062e8b-linux
export INITRD := b6a276df6a0e39f76bc8950e975daa2888ad83df-rootfs.cpio.gz

export BUILD_DIR:=$(abspath ${BUILD_DIR})
export MICROKIT_SDK:=$(abspath ${MICROKIT_SDK})
export TOP := $(abspath $(dir ${MAKEFILE_LIST}))

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
export SDDF_BENCHMARK=$(abspath ../../dep/sddf_benchmarking)
export LIBVMM=$(abspath ../../)
export EXAMPLE_DIR=$(abspath .)

IMAGE_FILE := $(BUILD_DIR)/loader.img
REPORT_FILE := $(BUILD_DIR)/report.txt

all: ${IMAGE_FILE}

qemu ${IMAGE_FILE} ${REPORT_FILE} clean clobber: ${BUILD_DIR}/Makefile FORCE
	${MAKE}  -C ${BUILD_DIR} MICROKIT_SDK=${MICROKIT_SDK} $(notdir $@)

${BUILD_DIR}/Makefile: echo.mk
	mkdir -p ${BUILD_DIR}
	cp echo.mk $@

FORCE:
