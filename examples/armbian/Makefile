#
# Copyright 2021, Breakaway Consulting Pty. Ltd.
# Copyright 2022, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif

ifeq ($(strip $(BOARD)),)
$(error BOARD must be specified)
endif

ifeq ($(BOARD),qemu_arm_virt)
ifeq ($(strip $(ARMBIAN)),)
$(error ARMBIAN image must be specified)
endif
endif

# Default build directory, pass BUILD_DIR=<dir> to override
BUILD_DIR ?= build
# Default config is a debug build, pass CONFIG=<debug/release/benchmark> to override
CONFIG ?= debug

# @ivanv: Check for dependencies and make sure they are installed/in the path

# @ivanv: check that all dependencies exist
# Specify that we use bash for all shell commands
SHELL=/bin/bash
# All dependencies needed to compile the VMM
QEMU := qemu-system-aarch64
DTC := dtc

ifndef TOOLCHAIN
	# Get whether the common toolchain triples exist
	TOOLCHAIN_AARCH64_LINUX_GNU := $(shell command -v aarch64-linux-gnu-gcc 2> /dev/null)
	TOOLCHAIN_AARCH64_UNKNOWN_LINUX_GNU := $(shell command -v aarch64-unknown-linux-gnu-gcc 2> /dev/null)
	# Then check if they are defined and select the appropriate one
	ifdef TOOLCHAIN_AARCH64_LINUX_GNU
		TOOLCHAIN := aarch64-linux-gnu
	else ifdef TOOLCHAIN_AARCH64_UNKNOWN_LINUX_GNU
		TOOLCHAIN := aarch64-unknown-linux-gnu
	else
		$(error "Could not find an AArch64 cross-compiler")
	endif
endif

CC := clang
LD := ld.lld
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

# @ivanv: need to have a step for putting in the initrd node into the DTB,
# 		  right now it is unfortunately hard-coded.

# @ivanv: check that the path of SDK_PATH/BOARD exists
# @ivanv: Have a list of supported boards to check with, if it's not one of those
# have a helpful message that lists all the support boards.

# @ivanv: incremental builds don't work with IMAGE_DIR changing
BOARD_DIR := $(MICROKIT_SDK)/board/$(BOARD)/$(CONFIG)
VMM := ../../
VMM_TOOLS := $(VMM)/tools
VMM_SRC_DIR := $(VMM)/src
SCRIPTS := $(VMM)/scripts
SYSTEM_DESCRIPTION := board/$(BOARD)/armbian.system

CURRENT_DIR := $(shell pwd)

ARMBIAN_VM_DIR := board/$(BOARD)/armbian_vm
ARMBIAN_VM_DTS_DIR := $(ARMBIAN_VM_DIR)/dts

ARMBIAN_VM_LINUX := $(ARMBIAN_VM_DIR)/linux
ARMBIAN_VM_INITRD := $(ARMBIAN_VM_DIR)/initrd.cpio.gz
ARMBIAN_VM_BASE_DTS := $(ARMBIAN_VM_DTS_DIR)/linux.dts
ARMBIAN_VM_DTS_OVERLAYS_qemu_arm_virt :=	$(ARMBIAN_VM_DTS_DIR)/init.dts \
											$(ARMBIAN_VM_DTS_DIR)/disable.dts \
											$(ARMBIAN_VM_DTS_DIR)/io.dts 
ARMBIAN_VM_DTS_OVERLAYS_odroidc4 :=		$(ARMBIAN_VM_DTS_DIR)/init.dts \
										$(ARMBIAN_VM_DTS_DIR)/disable.dts
ARMBIAN_VM_DTS_OVERLAYS := ${ARMBIAN_VM_DTS_OVERLAYS_${BOARD}}
ARMBIAN_VM_DTB := $(BUILD_DIR)/armbian_vm.dtb

ELFS := armbian_vmm.elf

IMAGE_FILE = $(BUILD_DIR)/loader.img
REPORT_FILE = $(BUILD_DIR)/report.txt

# @ivanv: should only compile printf.o in debug
VMM_OBJS := printf.o \
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
			vcpu.o \

ARMBIAN_VMM_OBJS :=	$(VMM_OBJS) \
						armbian_images.o \
						armbian_vmm.o \

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
		  -I$(VMM_SRC_DIR)/arch/aarch64 -I$(VMM_SRC_DIR) -I$(VMM_SRC_DIR)/util -I$(BOARD_DIR)/include \
		  -DBOARD_$(BOARD) \
		  -DCONFIG_$(CONFIG) \
		  -target aarch64-none-elf

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

all: directories $(IMAGE_FILE)

directories:
	$(shell mkdir -p $(BUILD_DIR))

qemu: all
	# @ivanv: check that the amount of RAM given to QEMU is at least the number of RAM that QEMU is setup with for seL4.
	if ! command -v $(QEMU) &> /dev/null; then echo "Could not find dependency: qemu-system-aarch64"; exit 1; fi
	$(QEMU) -machine virt,virtualization=on,secure=off \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-m size=2G \
			-drive file=$(ARMBIAN),format=qcow2,if=none,id=drive0 \
			-device virtio-blk-device,drive=drive0,id=virtblk0,num-queues=1

$(ARMBIAN_VM_DTB): $(ARMBIAN_VM_BASE_DTS) $(ARMBIAN_VM_DTS_OVERLAYS)
	sh $(SCRIPTS)/dtscat $^ > $(BUILD_DIR)/armbian_vm.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/armbian_vm.dts > $@

$(BUILD_DIR)/armbian_images.o: $(VMM_TOOLS)/package_guest_images.S $(ARMBIAN_VM_LINUX) $(ARMBIAN_VM_DTB) $(ARMBIAN_VM_INITRD)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(ARMBIAN_VM_LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(ARMBIAN_VM_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(ARMBIAN_VM_INITRD)\" \
					-target aarch64-none-elf \
					$< -o $@

$(BUILD_DIR)/%.o: %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(VMM_SRC_DIR)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(VMM_SRC_DIR)/util/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(VMM_SRC_DIR)/arch/aarch64/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(VMM_SRC_DIR)/arch/aarch64/vgic/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(VMM_SRC_DIR)/virtio/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/armbian_vmm.elf: $(addprefix $(BUILD_DIR)/, $(ARMBIAN_VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(ELFS)) $(SYSTEM_DESCRIPTION) $(ARMBIAN_VM_DIR)
	$(MICROKIT_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) $(ARMBIAN_VM_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)
