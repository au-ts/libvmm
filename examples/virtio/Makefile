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
VMM := ../..
VMM_TOOLS := $(VMM)/tools
VMM_SRC_DIR := $(VMM)/src
SYSTEM_DESCRIPTION := board/$(BOARD)/virtio.system

CURRENT_DIR := $(shell pwd)

LINUX_DIR := $(VMM_TOOLS)/linux
LINUX_BLK_DIR := $(LINUX_DIR)/blk
UIO_DRIVERS_DIR := $(LINUX_DIR)/uio_drivers

BLK_DRIVER_VM_DIR := board/$(BOARD)/blk_driver_vm
BLK_DRIVER_VM_DTS_DIR := $(BLK_DRIVER_VM_DIR)/dts
BLK_DRIVER_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/blk_driver_vm_rootfs

CLIENT_VM_1_DIR := board/$(BOARD)/client_vm_1
CLIENT_VM_1_DTS_DIR := $(CLIENT_VM_1_DIR)/dts
CLIENT_VM_1_ROOTFS_TMP_DIR := $(BUILD_DIR)/client_vm_1_rootfs

CLIENT_VM_2_DIR := board/$(BOARD)/client_vm_2
CLIENT_VM_2_DTS_DIR := $(CLIENT_VM_2_DIR)/dts
CLIENT_VM_2_ROOTFS_TMP_DIR := $(BUILD_DIR)/client_vm_2_rootfs

BLK_DRIVER_VM_LINUX := $(BLK_DRIVER_VM_DIR)/linux
BLK_DRIVER_VM_INITRD := $(BLK_DRIVER_VM_DIR)/rootfs.cpio.gz
BLK_DRIVER_VM_INITRD_OVERLAYED := $(BUILD_DIR)/blk_driver_vm_rootfs.cpio.gz
BLK_DRIVER_VM_BASE_DTS := $(BLK_DRIVER_VM_DTS_DIR)/linux.dts

CLIENT_VM_1_LINUX := $(CLIENT_VM_1_DIR)/linux
CLIENT_VM_1_INITRD := $(CLIENT_VM_1_DIR)/rootfs.cpio.gz
CLIENT_VM_1_INITRD_OVERLAYED := $(BUILD_DIR)/client_vm_1_rootfs.cpio.gz
CLIENT_VM_1_BASE_DTS := $(CLIENT_VM_1_DTS_DIR)/linux.dts

CLIENT_VM_2_LINUX := $(CLIENT_VM_2_DIR)/linux
CLIENT_VM_2_INITRD := $(CLIENT_VM_2_DIR)/rootfs.cpio.gz
CLIENT_VM_2_INITRD_OVERLAYED := $(BUILD_DIR)/client_vm_2_rootfs.cpio.gz
CLIENT_VM_2_BASE_DTS := $(CLIENT_VM_2_DTS_DIR)/linux.dts

BLK_DRIVER_VM_DTB := $(BUILD_DIR)/blk_driver_vm.dtb
CLIENT_VM_1_DTB := $(BUILD_DIR)/client_vm_1.dtb
CLIENT_VM_2_DTB := $(BUILD_DIR)/client_vm_2.dtb

SDDF := sddf
SDDF_BLK_COMPONENTS := $(SDDF)/blk/components
SDDF_BLK_SHAREDQUEUE := $(SDDF)/blk/libblksharedqueue
SDDF_SERIAL_RINGBUFFER := $(SDDF)/serial/libserialsharedringbuffer
SDDF_SERIAL_COMPONENTS := $(SDDF)/serial/components

ifeq ($(BOARD),qemu_arm_virt)
	SDDF_SERIAL_DRIVER := $(SDDF)/drivers/serial/arm
	BLK_DRIVER_VM_DTS_OVERLAYS :=	$(BLK_DRIVER_VM_DTS_DIR)/init.dts \
									$(BLK_DRIVER_VM_DTS_DIR)/uio.dts
	CLIENT_VM_1_DTS_OVERLAYS :=	$(CLIENT_VM_1_DTS_DIR)/init.dts \
								$(CLIENT_VM_1_DTS_DIR)/virtio.dts
	CLIENT_VM_2_DTS_OVERLAYS :=	$(CLIENT_VM_2_DTS_DIR)/init.dts \
								$(CLIENT_VM_2_DTS_DIR)/virtio.dts
	BLK_DRIVER_VM_USERLEVEL_INIT := qemu_blk_driver_init
	CLIENT_VM_1_USERLEVEL_INIT := qemu_blk_client_init
	CLIENT_VM_2_USERLEVEL_INIT := qemu_blk_client_init
else ifeq ($(BOARD),odroidc4)
	SDDF_SERIAL_DRIVER := $(SDDF)/drivers/serial/meson
	BLK_DRIVER_VM_DTS_OVERLAYS :=	$(BLK_DRIVER_VM_DTS_DIR)/init.dts \
									$(BLK_DRIVER_VM_DTS_DIR)/uio.dts \
									$(BLK_DRIVER_VM_DTS_DIR)/disable.dts
	CLIENT_VM_1_DTS_OVERLAYS :=	$(CLIENT_VM_1_DTS_DIR)/init.dts \
								$(CLIENT_VM_1_DTS_DIR)/virtio.dts\
								$(CLIENT_VM_1_DTS_DIR)/disable.dts
	CLIENT_VM_2_DTS_OVERLAYS :=	$(CLIENT_VM_2_DTS_DIR)/init.dts \
								$(CLIENT_VM_2_DTS_DIR)/virtio.dts \
								$(CLIENT_VM_2_DTS_DIR)/disable.dts
	BLK_DRIVER_VM_USERLEVEL_INIT := blk_driver_init
	CLIENT_VM_1_USERLEVEL_INIT := blk_client_init
	CLIENT_VM_2_USERLEVEL_INIT := blk_client_init
else
	$(error "Board $(BOARD) is not supported")
endif

ELFS := client_vmm_1.elf client_vmm_2.elf blk_driver_vmm.elf serial_mux_tx.elf serial_mux_rx.elf uart_driver.elf blk_virt.elf

CLIENT_VM_1_USERLEVEL := postmark
CLIENT_VM_1_USERLEVEL_INIT := blk_client_init
CLIENT_VM_2_USERLEVEL := postmark
CLIENT_VM_2_USERLEVEL_INIT := blk_client_init
BLK_DRIVER_VM_USERLEVEL := uio_blk_driver storage_0 storage_1

IMAGE_FILE = $(BUILD_DIR)/loader.img
REPORT_FILE = $(BUILD_DIR)/report.txt

# @ivanv: should only compile printf.o in debug
VMM_OBJS := printf.o \
			bitarray.o \
			fsmem.o \
			datastore.o \
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

CLIENT_VMM_1_OBJS := $(VMM_OBJS) \
					client_images_1.o \
					client_vmm_1.o \
					virtio.o \
					console.o \
					block.o \
					mmio.o \
					sddf_serial_sharedringbuffer.o \
					sddf_blk_shared_queue.o

CLIENT_VMM_2_OBJS := $(VMM_OBJS) \
					client_images_2.o \
					client_vmm_2.o \
					virtio.o \
					console.o \
					block.o \
					mmio.o \
					sddf_serial_sharedringbuffer.o \
					sddf_blk_shared_queue.o

SERIAL_MUX_TX_OBJS := mux_tx.o sddf_serial_sharedringbuffer.o
SERIAL_MUX_RX_OBJS := mux_rx.o sddf_serial_sharedringbuffer.o
SERIAL_DRIVER_OBJS := uart.o sddf_serial_sharedringbuffer.o

BLK_VIRT_OBJS := virt.o sddf_blk_shared_queue.o
BLK_DRIVER_VMM_OBJS := $(VMM_OBJS) \
					blk_driver_images.o \
					blk_driver_vmm.o \
					virtio.o \
					console.o \
					block.o \
					mmio.o \
					sddf_serial_sharedringbuffer.o \
					sddf_blk_shared_queue.o
UIO_BLK_DRIVER_OBJS := blk.o libuio.o sddf_blk_shared_queue.o

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
		  -I$(SDDF_SERIAL_DRIVER)/include \
		  -I$(SDDF)/include \
		  -DBOARD_$(BOARD) \
		  -DCONFIG_$(CONFIG) \
		  -target aarch64-none-elf

CFLAGS_LINUX :=	-g3 \
				-O3 \
				-Wno-unused-command-line-argument \
				-Wall -Wno-unused-function -Werror \
				-I$(SDDF)/include \
				-I$(LINUX_DIR)/include

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

all: directories $(IMAGE_FILE)

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

# Unpack rootfs, put the userlevel in /root and userlevel_init in /etc/init.d/S99, repack rootfs
$(BLK_DRIVER_VM_INITRD_OVERLAYED): $(addprefix $(BUILD_DIR)/, $(BLK_DRIVER_VM_USERLEVEL)) $(addprefix $(BUILD_DIR)/, $(BLK_DRIVER_VM_USERLEVEL_INIT))
	rm -r $(BLK_DRIVER_VM_ROOTFS_TMP_DIR) 2> /dev/null; \
	mkdir -p $(BLK_DRIVER_VM_ROOTFS_TMP_DIR) && \
	gunzip -dc $(BLK_DRIVER_VM_INITRD) | fakeroot cpio -idmv -D$(BLK_DRIVER_VM_ROOTFS_TMP_DIR) 2> /dev/null && \
	for f in $(BLK_DRIVER_VM_USERLEVEL); do \
		cp -- "$(BUILD_DIR)/$$f" "$(BLK_DRIVER_VM_ROOTFS_TMP_DIR)/root/$$f"; \
		chmod +x "$(BLK_DRIVER_VM_ROOTFS_TMP_DIR)/root/$$f"; \
	done && \
	for f in $(BLK_DRIVER_VM_USERLEVEL_INIT); do \
		cp -- "$(BUILD_DIR)/$$f" "$(BLK_DRIVER_VM_ROOTFS_TMP_DIR)/etc/init.d/S99$$f"; \
		chmod +x "$(BLK_DRIVER_VM_ROOTFS_TMP_DIR)/etc/init.d/S99$$f"; \
	done && \
	(cd $(BLK_DRIVER_VM_ROOTFS_TMP_DIR) && find . | cpio -o -H newc -R root:root 2> /dev/null | gzip > $(CURRENT_DIR)/$(BLK_DRIVER_VM_INITRD_OVERLAYED))
	
$(CLIENT_VM_1_INITRD_OVERLAYED): $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_1_USERLEVEL)) $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_1_USERLEVEL_INIT))
	rm -r $(CLIENT_VM_1_ROOTFS_TMP_DIR) 2> /dev/null; \
	mkdir -p $(CLIENT_VM_1_ROOTFS_TMP_DIR) && \
	gunzip -dc $(CLIENT_VM_1_INITRD) | fakeroot cpio -idmv -D$(CLIENT_VM_1_ROOTFS_TMP_DIR) 2> /dev/null && \
	for f in $(CLIENT_VM_1_USERLEVEL); do \
		cp -- "$(BUILD_DIR)/$$f" "$(CLIENT_VM_1_ROOTFS_TMP_DIR)/root/$$f"; \
		chmod +x "$(CLIENT_VM_1_ROOTFS_TMP_DIR)/root/$$f";	\
	done && \
	for f in $(CLIENT_VM_1_USERLEVEL_INIT); do \
		cp -- "$(BUILD_DIR)/$$f" "$(CLIENT_VM_1_ROOTFS_TMP_DIR)/etc/init.d/S99$$f"; \
		chmod +x "$(CLIENT_VM_1_ROOTFS_TMP_DIR)/etc/init.d/S99$$f"; \
	done && \
	(cd $(CLIENT_VM_1_ROOTFS_TMP_DIR) && find . | cpio -o -H newc -R root:root 2> /dev/null | gzip > $(CURRENT_DIR)/$(CLIENT_VM_1_INITRD_OVERLAYED))

$(CLIENT_VM_2_INITRD_OVERLAYED): $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_2_USERLEVEL)) $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_2_USERLEVEL_INIT))
	rm -r $(CLIENT_VM_2_ROOTFS_TMP_DIR) 2> /dev/null; \
	mkdir -p $(CLIENT_VM_2_ROOTFS_TMP_DIR) && \
	gunzip -dc $(CLIENT_VM_2_INITRD) | fakeroot cpio -idmv -D$(CLIENT_VM_2_ROOTFS_TMP_DIR) 2> /dev/null && \
	for f in $(CLIENT_VM_2_USERLEVEL); do \
		cp -- "$(BUILD_DIR)/$$f" "$(CLIENT_VM_2_ROOTFS_TMP_DIR)/root/$$f"; \
		chmod +x "$(CLIENT_VM_2_ROOTFS_TMP_DIR)/root/$$f";	\
	done && \
	for f in $(CLIENT_VM_2_USERLEVEL_INIT); do \
		cp -- "$(BUILD_DIR)/$$f" "$(CLIENT_VM_2_ROOTFS_TMP_DIR)/etc/init.d/S99$$f"; \
		chmod +x "$(CLIENT_VM_2_ROOTFS_TMP_DIR)/etc/init.d/S99$$f"; \
	done && \
	(cd $(CLIENT_VM_2_ROOTFS_TMP_DIR) && find . | cpio -o -H newc -R root:root 2> /dev/null | gzip > $(CURRENT_DIR)/$(CLIENT_VM_2_INITRD_OVERLAYED))

$(BLK_DRIVER_VM_DTB): $(BLK_DRIVER_VM_BASE_DTS) $(BLK_DRIVER_VM_DTS_OVERLAYS)
	sh $(VMM_TOOLS)/dtscat $^ > $(BUILD_DIR)/blk_driver_vm.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/blk_driver_vm.dts > $@

$(CLIENT_VM_1_DTB): $(CLIENT_VM_1_BASE_DTS) $(CLIENT_VM_1_DTS_OVERLAYS)
	sh $(VMM_TOOLS)/dtscat $^ > $(BUILD_DIR)/client_vm_1.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/client_vm_1.dts > $@

$(CLIENT_VM_2_DTB): $(CLIENT_VM_2_BASE_DTS) $(CLIENT_VM_2_DTS_OVERLAYS)
	sh $(VMM_TOOLS)/dtscat $^ > $(BUILD_DIR)/client_vm_2.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/client_vm_2.dts > $@

$(BUILD_DIR)/blk_driver_images.o: $(VMM_TOOLS)/package_guest_images.S $(BLK_DRIVER_VM_LINUX) $(BLK_DRIVER_VM_DTB) $(BLK_DRIVER_VM_INITRD_OVERLAYED)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(BLK_DRIVER_VM_LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(BLK_DRIVER_VM_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(BLK_DRIVER_VM_INITRD_OVERLAYED)\" \
					-target aarch64-none-elf \
					$< -o $@

$(BUILD_DIR)/client_images_1.o: $(VMM_TOOLS)/package_guest_images.S $(CLIENT_VM_1_LINUX) $(CLIENT_VM_1_DTB) $(CLIENT_VM_1_INITRD_OVERLAYED)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(CLIENT_VM_1_LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(CLIENT_VM_1_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(CLIENT_VM_1_INITRD_OVERLAYED)\" \
					-target aarch64-none-elf \
					$< -o $@

$(BUILD_DIR)/client_images_2.o: $(VMM_TOOLS)/package_guest_images.S $(CLIENT_VM_2_LINUX) $(CLIENT_VM_2_DTB) $(CLIENT_VM_2_INITRD_OVERLAYED)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(CLIENT_VM_2_LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(CLIENT_VM_2_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(CLIENT_VM_2_INITRD_OVERLAYED)\" \
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

$(BUILD_DIR)/%.o: $(SDDF_SERIAL_COMPONENTS)/%.c Makefile
	$(CC) -c $(CFLAGS) -DSERIAL_NUM_CLIENTS=2 -DSERIAL_TRANSFER_WITH_COLOUR=1 $< -o $@

$(BUILD_DIR)/sddf_serial_sharedringbuffer.o: Makefile
	$(MAKE) -C $(SDDF_SERIAL_RINGBUFFER) MICROKIT_INCLUDE=$(MICROKIT_SDK)
	cp $(SDDF_SERIAL_RINGBUFFER)/sddf_serial_sharedringbuffer.o $(BUILD_DIR)/sddf_serial_sharedringbuffer.o

$(BUILD_DIR)/%.o: $(SDDF_SERIAL_DRIVER)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SDDF_BLK_COMPONENTS)/%.c Makefile
	$(CC) -c $(CFLAGS) -DBLK_NUM_CLIENTS=2 $< -o $@

$(BUILD_DIR)/sddf_blk_shared_queue.o: Makefile
	$(MAKE) -C $(SDDF_BLK_SHAREDQUEUE) MICROKIT_INCLUDE=$(MICROKIT_SDK)
	cp $(SDDF_BLK_SHAREDQUEUE)/sddf_blk_shared_queue.o $(BUILD_DIR)/sddf_blk_shared_queue.o

$(BUILD_DIR)/client_vmm_1.elf: $(addprefix $(BUILD_DIR)/, $(CLIENT_VMM_1_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/client_vmm_2.elf: $(addprefix $(BUILD_DIR)/, $(CLIENT_VMM_2_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/blk_driver_vmm.elf: $(addprefix $(BUILD_DIR)/, $(BLK_DRIVER_VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/blk_virt.elf: $(addprefix $(BUILD_DIR)/, $(BLK_VIRT_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial_mux_tx.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_MUX_TX_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial_mux_rx.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_MUX_RX_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/uart_driver.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_DRIVER_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/%.o: $(UIO_DRIVERS_DIR)/%.c Makefile
	$(TOOLCHAIN)-gcc -c $(CFLAGS_LINUX) $< -o $@

$(BUILD_DIR)/postmark: $(LINUX_BLK_DIR)/postmark.c Makefile
	$(TOOLCHAIN)-gcc $< -o $@

$(BUILD_DIR)/storage_0: Makefile
	dd if=/dev/zero of=$@ bs=1K count=1000 && \
	mkfs.ext4 -F $@ -b 4096

$(BUILD_DIR)/storage_1: Makefile
	dd if=/dev/zero of=$@ bs=1K count=1000 && \
	mkfs.ext4 -F $@ -b 4096 

$(BUILD_DIR)/blk_client_init: $(LINUX_BLK_DIR)/blk_client_init Makefile
	cp $< $@

$(BUILD_DIR)/blk_driver_init: $(LINUX_BLK_DIR)/blk_driver_init Makefile
	cp $< $@

$(BUILD_DIR)/qemu_blk_driver_init: $(LINUX_BLK_DIR)/qemu_blk_driver_init Makefile
	cp $< $@

$(BUILD_DIR)/uio_blk_driver: $(addprefix $(BUILD_DIR)/, $(UIO_BLK_DRIVER_OBJS))
	$(TOOLCHAIN)-gcc $(CFLAGS_LINUX) $^ -o $@

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(ELFS)) $(SYSTEM_DESCRIPTION)
	$(MICROKIT_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)