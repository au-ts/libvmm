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
SYSTEM_DESCRIPTION := board/$(BOARD)/virtio.system

CURRENT_DIR := $(shell pwd)

# BLK_DRIVER_VM_DIR := board/$(BOARD)/blk_driver_vm
# BLK_DRIVER_VM_DTS_DIR := $(BLK_DRIVER_VM_DIR)/dts
# BLK_DRIVER_VM_USERLEVEL_DIR := userlevel/blk_driver_vm
# BLK_DRIVER_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/blk_driver_vm_rootfs

CLIENT_VM_DIR := board/$(BOARD)/client_vm
CLIENT_VM_DTS_DIR := $(CLIENT_VM_DIR)/dts
CLIENT_VM_USERLEVEL_DIR := userlevel/client_vm
CLIENT_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/client_vm_rootfs

# BLK_DRIVER_VM_LINUX := $(BLK_DRIVER_VM_DIR)/linux
# BLK_DRIVER_VM_INITRD := $(BLK_DRIVER_VM_DIR)/rootfs.cpio.gz
# BLK_DRIVER_VM_INITRD_OVERLAYED := $(BLK_DRIVER_VM_DIR)/rootfs_overlayed.cpio.gz
# BLK_DRIVER_VM_BASE_DTS := $(BLK_DRIVER_VM_DTS_DIR)/linux.dts
# BLK_DRIVER_VM_DTS_OVERLAYS :=	$(BLK_DRIVER_VM_DTS_DIR)/init.dts \

CLIENT_VM_LINUX := $(CLIENT_VM_DIR)/linux
CLIENT_VM_INITRD := $(CLIENT_VM_DIR)/rootfs.cpio.gz
CLIENT_VM_INITRD_OVERLAYED := $(CLIENT_VM_DIR)/rootfs_overlayed.cpio.gz
CLIENT_VM_BASE_DTS := $(CLIENT_VM_DTS_DIR)/linux.dts
CLIENT_VM_DTS_OVERLAYS :=	$(CLIENT_VM_DTS_DIR)/init.dts \
							$(CLIENT_VM_DTS_DIR)/virtio.dts \

# BLK_DRIVER_VM_DTB := $(BUILD_DIR)/blk_driver_vm.dtb
CLIENT_VM_DTB := $(BUILD_DIR)/client_vm.dtb

SDDF := sddf
SDDF_SERIAL_RINGBUFFER := $(SDDF)/serial/libserialsharedringbuffer
SDDF_SERIAL_COMPONENTS := $(SDDF)/serial/components
SDDF_SERIAL_DRIVER := $(SDDF)/drivers/serial/arm
SDDF_UTIL := $(SDDF)/util

# ELFS := blk_driver_vmm.elf client_vmm.elf serial_mux_tx.elf serial_mux_rx.elf uart_driver.elf
ELFS := client_vmm.elf serial_mux_tx.elf serial_mux_rx.elf uart_driver.elf

# BLK_DRIVER_VM_USERLEVEL_ELFS := cmdq_test.elf
CLIENT_VM_USERLEVEL_ELFS := virtio_blk_test.elf

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

CLIENT_VMM_OBJS :=	$(VMM_OBJS) \
					client_images.o \
					client_vmm.o \
					virtio.o \
					console.o \
					mmio.o \
					shared_ringbuffer.o \

# BLK_DRIVER_VMM_OBJS :=	$(VMM_OBJS) \
# 						blk_driver_images.o \
# 						blk_driver_vmm.o \

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
		  -DCONFIG_$(CONFIG) \
		  -target aarch64-none-elf

CFLAGS_USERLEVEL :=	-g3 \
		  			-O3 \
					-Wno-unused-command-line-argument \
		  			-Wall -Wno-unused-function -Werror \
					-target aarch64-linux-gnu

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

all: directories client_vm_userlevel $(IMAGE_FILE)
# all: directories blk_driver_vm_userlevel client_vm_userlevel $(IMAGE_FILE)

qemu: all
	# @ivanv: check that the amount of RAM given to QEMU is at least the number of RAM that QEMU is setup with for seL4.
	if ! command -v $(QEMU) &> /dev/null; then echo "Could not find dependency: qemu-system-aarch64"; exit 1; fi
	$(QEMU) -machine virt,virtualization=on,secure=off \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-m size=2G \
			-nographic

directories:
	$(shell mkdir -p $(BUILD_DIR))

# Unpack rootfs, put the userlevel binary in /root, repack rootfs
# blk_driver_vm_userlevel: $(addprefix $(BUILD_DIR)/, $(BLK_DRIVER_VM_USERLEVEL_ELFS))
# 	rm -r $(BLK_DRIVER_VM_ROOTFS_TMP_DIR) 2> /dev/null; \
# 	mkdir -p $(BLK_DRIVER_VM_ROOTFS_TMP_DIR) && \
# 	gunzip -dc $(BLK_DRIVER_VM_INITRD) | fakeroot cpio -idmv -D$(BLK_DRIVER_VM_ROOTFS_TMP_DIR) && \
# 	cp $< $(BLK_DRIVER_VM_ROOTFS_TMP_DIR)/root && \
# 	(cd $(BLK_DRIVER_VM_ROOTFS_TMP_DIR) && find . | cpio -o -H newc -R root:root | gzip > $(CURRENT_DIR)/$(BLK_DRIVER_VM_INITRD_OVERLAYED))
	
client_vm_userlevel: $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_USERLEVEL_ELFS))
	rm -r $(CLIENT_VM_ROOTFS_TMP_DIR) 2> /dev/null; \
	mkdir -p $(CLIENT_VM_ROOTFS_TMP_DIR) && \
	gunzip -dc $(CLIENT_VM_INITRD) | fakeroot cpio -idmv -D$(CLIENT_VM_ROOTFS_TMP_DIR) 2> /dev/null && \
	cp $< $(CLIENT_VM_ROOTFS_TMP_DIR)/root && \
	(cd $(CLIENT_VM_ROOTFS_TMP_DIR) && find . | cpio -o -H newc -R root:root 2> /dev/null | gzip > $(CURRENT_DIR)/$(CLIENT_VM_INITRD_OVERLAYED))

# $(BLK_DRIVER_VM_DTB): $(BLK_DRIVER_VM_BASE_DTS) $(BLK_DRIVER_VM_DTS_OVERLAYS)
# 	sh ../../scripts/dtscat $^ > $(BUILD_DIR)/blk_driver_vm.dts
# 	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/blk_driver_vm.dts > $@

# $(BUILD_DIR)/blk_driver_images.o: $(VMM_TOOLS)/package_guest_images.S $(BLK_DRIVER_VM_LINUX) $(BLK_DRIVER_VM_DTB) $(BLK_DRIVER_VM_INITRD_OVERLAYED)
# 	$(CC) -c -g3 -x assembler-with-cpp \
# 					-DGUEST_KERNEL_IMAGE_PATH=\"$(BLK_DRIVER_VM_LINUX)\" \
# 					-DGUEST_DTB_IMAGE_PATH=\"$(BLK_DRIVER_VM_DTB)\" \
# 					-DGUEST_INITRD_IMAGE_PATH=\"$(BLK_DRIVER_VM_INITRD_OVERLAYED)\" \
# 					-target aarch64-none-elf \
# 					$< -o $@

$(CLIENT_VM_DTB): $(CLIENT_VM_BASE_DTS) $(CLIENT_VM_DTS_OVERLAYS)
	sh ../../scripts/dtscat $^ > $(BUILD_DIR)/client_vm.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/client_vm.dts > $@

$(BUILD_DIR)/client_images.o: $(VMM_TOOLS)/package_guest_images.S $(CLIENT_VM_LINUX) $(CLIENT_VM_DTB) $(CLIENT_VM_INITRD_OVERLAYED)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(CLIENT_VM_LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(CLIENT_VM_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(CLIENT_VM_INITRD_OVERLAYED)\" \
					-target aarch64-none-elf \
					$< -o $@

# $(BUILD_DIR)/%.elf: $(BLK_DRIVER_VM_USERLEVEL_DIR)/%.c Makefile
# 	$(CC) -c $(CFLAGS_USERLEVEL) $< -o $@

$(BUILD_DIR)/%.elf: $(CLIENT_VM_USERLEVEL_DIR)/%.c Makefile
	$(TOOLCHAIN)-gcc $< -o $@

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

$(BUILD_DIR)/client_vmm.elf: $(addprefix $(BUILD_DIR)/, $(CLIENT_VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# $(BUILD_DIR)/blk_driver_vmm.elf: $(addprefix $(BUILD_DIR)/, $(BLK_DRIVER_VMM_OBJS))
# 	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial_mux_tx.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_MUX_TX_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial_mux_rx.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_MUX_RX_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/uart_driver.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_DRIVER_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# $(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(ELFS)) $(SYSTEM_DESCRIPTION) $(BLK_DRIVER_VM_DIR) $(CLIENT_VM_DIR)
# 	$(MICROKIT_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) $(BLK_DRIVER_VM_DIR) $(CLIENT_VM_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(ELFS)) $(SYSTEM_DESCRIPTION) $(CLIENT_VM_DIR)
	$(MICROKIT_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) $(CLIENT_VM_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)