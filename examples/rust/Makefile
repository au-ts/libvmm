#
# Copyright 2021, Breakaway Consulting Pty. Ltd.
# Copyright 2022, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif

# Default build directory, pass BUILD_DIR=<dir> to override
BUILD_DIR ?= build
# Default config is a debug build, pass CONFIG=<debug/release/benchmark> to override
CONFIG ?= debug
# Default board supported is QEMU ARM virt
BOARD ?= qemu_arm_virt

# Specify that we use bash for all shell commands
SHELL=/bin/bash
# All dependencies needed to compile the VMM
QEMU := qemu-system-aarch64
DTC := dtc
CC := clang
LD := ld.lld
AR := llvm-ar
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

BOARD_DIR := $(MICROKIT_SDK)/board/$(BOARD)/$(CONFIG)
LIBVMM := ../../src
SYSTEM_DESCRIPTION := rust_vmm.system

IMAGE_DIR := images
LINUX := $(IMAGE_DIR)/linux
DTS := $(IMAGE_DIR)/linux.dts
DTB := $(BUILD_DIR)/linux.dtb
INITRD := $(IMAGE_DIR)/rootfs.cpio.gz

ELFS := vmm.elf
IMAGE_FILE = $(BUILD_DIR)/loader.img
REPORT_FILE = $(BUILD_DIR)/report.txt

LIBVMM_OBJS := printf.o \
				virq.o \
				linux.o \
				guest.o \
				psci.o \
				smc.o \
				fault.o \
				util.o \
				vgic.o \
				vgic_v2.o \
				tcb.o \
				vcpu.o

# Toolchain flags
# FIXME: For optimisation we should consider providing the flag -mcpu.
# FIXME: We should also consider whether -mgeneral-regs-only should be
# used to avoid the use of the FPU and therefore seL4 does not have to
# context switch the FPU.
# Note we only need -Wno-unused-command-line-argument because in Nix
# passes an extra `--gcc-toolchain` flag which we do not need.
CFLAGS := -mstrict-align \
		  -g3 \
		  -O3 \
		  -ffreestanding \
		  -nostdlib \
		  -Wno-unused-command-line-argument \
		  -Wall -Wno-unused-function -Werror \
		  -I$(LIBVMM)/arch/aarch64 -I$(LIBVMM) -I$(LIBVMM)/util -I$(BOARD_DIR)/include \
		  -DBOARD_$(BOARD) \
		  -DCONFIG_$(CONFIG) \
		  -target aarch64-none-elf

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

RUST_TARGET_PATH := .
RUST_MICROKIT_TARGET := aarch64-microkit-minimal
TARGET_DIR := $(BUILD_DIR)/target

RUST_ENV := \
	RUST_TARGET_PATH=$(abspath $(RUST_TARGET_PATH)) \
	SEL4_INCLUDE_DIRS=$(abspath $(BOARD_DIR)/include) \
	MICROKIT_BOARD_DIR=$(abspath $(BOARD_DIR)) \
	BUILD_DIR=$(abspath $(BUILD_DIR)) \
	IMAGE_DIR=$(abspath $(IMAGE_DIR)) \

RUST_OPTIONS := \
	-Z unstable-options \
	-Z build-std=core,alloc,compiler_builtins \
	-Z build-std-features=compiler-builtins-mem \
	-Z bindeps \
	--release \
	--target $(RUST_MICROKIT_TARGET) \
	--target-dir $(abspath $(TARGET_DIR)) \
	--out-dir $(abspath $(BUILD_DIR)) \

all: directories $(IMAGE_FILE)

qemu: all
	$(QEMU) -machine virt,virtualization=on,highmem=off,secure=off \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-m size=2G \
			-nographic

directories:
	$(shell mkdir -p $(BUILD_DIR))

$(DTB): $(DTS)
	$(DTC) -q -I dts -O dtb $< > $@

$(BUILD_DIR)/%.o: %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(LIBVMM)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(LIBVMM)/util/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(LIBVMM)/arch/aarch64/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(LIBVMM)/arch/aarch64/vgic/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/libvmm.a: $(addprefix $(BUILD_DIR)/, $(LIBVMM_OBJS))
	$(AR) -rv $@ $^

$(BUILD_DIR)/vmm.elf: src/vmm.rs $(BUILD_DIR)/libvmm.a $(LINUX) $(INITRD) $(DTB)
	$(RUST_ENV) \
		cargo build $(RUST_OPTIONS)

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(ELFS)) $(SYSTEM_DESCRIPTION)
	$(MICROKIT_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) $(IMAGE_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)
