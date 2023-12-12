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

# CC := clang
# LD := ld.lld
CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
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
SYSTEM_DESCRIPTION := board/$(BOARD)/virtio.system

IMAGE_DIR := board/$(BOARD)
LINUX := $(IMAGE_DIR)/linux
DTS := $(IMAGE_DIR)/linux.dts
VMM_DTS_OVERLAY := $(IMAGE_DIR)/vmm_overlay.dts
VMM_CLIENT_DTS_OVERLAY := $(IMAGE_DIR)/vmm_client_overlay.dts
VMM_DRIVER_DTS_OVERLAY := $(IMAGE_DIR)/vmm_driver_overlay.dts
VMM_CLIENT_DTB := $(BUILD_DIR)/vmm_client_linux.dtb
VMM_DRIVER_DTB := $(BUILD_DIR)/vmm_driver_linux.dtb
INITRD := $(IMAGE_DIR)/rootfs.cpio.gz

SDDF := sddf
SDDF_SERIAL_RINGBUFFER := $(SDDF)/serial/libserialsharedringbuffer
SDDF_SERIAL_COMPONENTS := $(SDDF)/serial/components
# SDDF_SERIAL_DRIVER := $(SDDF)/drivers/serial/arm
SDDF_SERIAL_DRIVER := $(SDDF)/drivers/serial/meson
SDDF_UTIL := $(SDDF)/util

ELFS := vmm_driver.elf vmm_client.elf serial_mux_tx.elf serial_mux_rx.elf uart_driver.elf

IMAGE_FILE = $(BUILD_DIR)/loader.img
REPORT_FILE = $(BUILD_DIR)/report.txt

# @ivanv: should only compile printf.o in debug
VMM_DRIVER_OBJS := vmm_driver.o \
			printf.o \
			virq.o \
			linux.o \
			guest.o \
			psci.o \
			smc.o \
			fault.o \
			util.o \
			vgic.o \
			vgic_v2.o \
			package_guest_images_driver.o \
			tcb.o \
			vcpu.o \
			mmio.o \
			console.o \
			shared_ringbuffer.o

VMM_CLIENT_OBJS := vmm_client.o \
			printf.o \
			virq.o \
			linux.o \
			guest.o \
			psci.o \
			smc.o \
			fault.o \
			util.o \
			vgic.o \
			vgic_v2.o \
			package_guest_images_client.o \
			tcb.o \
			vcpu.o \
			mmio.o \
			console.o \
			shared_ringbuffer.o

SERIAL_MUX_TX_OBJS := mux_tx.o shared_ringbuffer.o
SERIAL_MUX_RX_OBJS := mux_rx.o shared_ringbuffer.o
SERIAL_DRIVER_OBJS := uart.o shared_ringbuffer.o

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
		  -I$(SDDF_SERIAL_RINGBUFFER)/include \
		  -I$(SDDF_SERIAL_DRIVER)/include \
		  -I$(SDDF) \
		  -DBOARD_$(BOARD) \
		  -DCONFIG_$(CONFIG)

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

all: directories driver_vm_userlevel $(IMAGE_FILE)

qemu: all
	# @ivanv: check that the amount of RAM given to QEMU is at least the number of RAM that QEMU is setup with for seL4.
	if ! command -v $(QEMU) &> /dev/null; then echo "Could not find dependency: qemu-system-aarch64"; exit 1; fi
	$(QEMU) -machine virt,virtualization=on,highmem=off,secure=off \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-m size=2G \
			-nographic

directories:
	$(shell mkdir -p $(BUILD_DIR))

CURRENT_DIR := $(shell pwd)

DRIVER_VM_USERLEVEL_DIR := userlevel
DRIVER_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/driver_vm_rootfs

DRIVER_VM_USERLEVEL_ELFS := uio_enable_irq.elf uio_ethernet.elf uio_read.elf uio_write.elf

DRIVER_VM_INITRD := $(IMAGE_DIR)/rootfs.cpio.gz
DRIVER_VM_INITRD_OVERLAYED := $(BUILD_DIR)/rootfs_overlayed.cpio.gz

# Unpack rootfs, put the userlevel binary in /root, repack rootfs
driver_vm_userlevel: $(addprefix $(BUILD_DIR)/, $(DRIVER_VM_USERLEVEL_ELFS))
	rm -r $(DRIVER_VM_ROOTFS_TMP_DIR) 2> /dev/null; \
	mkdir -p $(DRIVER_VM_ROOTFS_TMP_DIR) && \
	gunzip -dc $(DRIVER_VM_INITRD) | fakeroot cpio -idmv -D$(DRIVER_VM_ROOTFS_TMP_DIR) && \
	cp $^ $(DRIVER_VM_ROOTFS_TMP_DIR)/root && \
	(cd $(DRIVER_VM_ROOTFS_TMP_DIR) && find . | cpio -o -H newc -R root:root 2> /dev/null | gzip > $(CURRENT_DIR)/$(DRIVER_VM_INITRD_OVERLAYED))

$(VMM_CLIENT_DTB): $(DTS) $(VMM_CLIENT_DTS_OVERLAY)
	if ! command -v $(DTC) &> /dev/null; then echo "Could not find dependency: Device Tree Compiler (dtc)"; exit 1; fi
	# @ivanv: Shouldn't supress warnings
	cat $^ > $(BUILD_DIR)/tem.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/tem.dts > $@

$(VMM_DRIVER_DTB): $(DTS) $(VMM_DRIVER_DTS_OVERLAY)
	if ! command -v $(DTC) &> /dev/null; then echo "Could not find dependency: Device Tree Compiler (dtc)"; exit 1; fi
	# @ivanv: Shouldn't supress warnings
	cat $^ > $(BUILD_DIR)/tem.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/tem.dts > $@

$(BUILD_DIR)/package_guest_images_client.o: $(VMM_TOOLS)/package_guest_images.S $(IMAGE_DIR) $(LINUX) $(INITRD) $(VMM_CLIENT_DTB)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(VMM_CLIENT_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(INITRD)\" \
					$< -o $@

$(BUILD_DIR)/package_guest_images_driver.o: $(VMM_TOOLS)/package_guest_images.S $(IMAGE_DIR) $(LINUX) $(DRIVER_VM_INITRD_OVERLAYED) $(VMM_DRIVER_DTB)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(VMM_DRIVER_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(DRIVER_VM_INITRD_OVERLAYED)\" \
					$< -o $@

$(BUILD_DIR)/%.elf: $(DRIVER_VM_USERLEVEL_DIR)/%.c Makefile
	$(CC) $< -o $@

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

$(BUILD_DIR)/%.o: $(SDDF_SERIAL_COMPONENTS)/%.c Makefile
	$(CC) -c $(CFLAGS) -DNUM_CLIENTS=2 $< -o $@

$(BUILD_DIR)/%.o: $(SDDF_SERIAL_RINGBUFFER)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SDDF_SERIAL_DRIVER)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/vmm_driver.elf: $(addprefix $(BUILD_DIR)/, $(VMM_DRIVER_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/vmm_client.elf: $(addprefix $(BUILD_DIR)/, $(VMM_CLIENT_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial_mux_tx.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_MUX_TX_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial_mux_rx.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_MUX_RX_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/uart_driver.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_DRIVER_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(ELFS)) $(SYSTEM_DESCRIPTION) $(IMAGE_DIR)
	$(MICROKIT_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) $(IMAGE_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)
