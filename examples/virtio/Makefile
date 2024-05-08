#
# Copyright 2021, Breakaway Consulting Pty. Ltd.
# Copyright 2022, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#

# Default build directory, pass BUILD_DIR=<dir> to override
BUILD_DIR ?= build
# Default config is a debug build, pass CONFIG=<debug/release/benchmark> to override
CONFIG ?= debug

ifeq ($(strip $(MICROKIT_SDK)),)
	$(error MICROKIT_SDK must be specified)
endif

ifndef BOARD
	$(error BOARD must be specified)
endif

ifeq ($(filter $(BOARD), odroidc4 qemu_arm_virt),)
	$(error "Board $(BOARD) is not supported")
endif

# @ivanv: check that all dependencies exist
# Specify that we use bash for all shell commands
SHELL=/bin/bash
# All dependencies needed to compile the VMM
QEMU := qemu-system-aarch64
QEMU_SND_BACKEND := coreaudio
QEMU_SND_FRONTEND := hda

DTC := dtc

CC := clang
CC_USERLEVEL := zig cc
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
LINUX_SND_DIR := $(LINUX_DIR)/snd
UIO_DIR := $(LINUX_DIR)/uio

SDDF_DIR := $(abspath ../../dep/sddf)

BLK_DRIVER_VM_DIR := board/$(BOARD)/blk_driver_vm
BLK_DRIVER_VM_DTS_DIR := $(BLK_DRIVER_VM_DIR)/dts
BLK_DRIVER_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/blk_driver_vm_rootfs

SND_DRIVER_VM_DIR := board/$(BOARD)/snd_driver_vm
SND_DRIVER_VM_DTS_DIR := $(SND_DRIVER_VM_DIR)/dts
SND_DRIVER_VM_USERLEVEL_DIR := userlevel/snd_driver_vm
SND_DRIVER_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/snd_driver_vm_rootfs

CLIENT_VM_DIR := board/$(BOARD)/client_vm
CLIENT_VM_DTS_DIR := $(CLIENT_VM_DIR)/dts
CLIENT_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/client_vm_rootfs

BLK_DRIVER_VM_LINUX := $(BLK_DRIVER_VM_DIR)/linux
BLK_DRIVER_VM_INITRD := $(BLK_DRIVER_VM_DIR)/rootfs.cpio.gz
BLK_DRIVER_VM_INITRD_MODIFIED := $(BUILD_DIR)/blk_driver_vm_rootfs.cpio.gz
BLK_DRIVER_VM_BASE_DTS := $(BLK_DRIVER_VM_DTS_DIR)/linux.dts
BLK_DRIVER_VM_DTS_OVERLAYS_qemu_arm_virt :=	$(BLK_DRIVER_VM_DTS_DIR)/init.dts \
											$(BLK_DRIVER_VM_DTS_DIR)/io.dts \
											$(BLK_DRIVER_VM_DTS_DIR)/disable.dts
BLK_DRIVER_VM_DTS_OVERLAYS_odroidc4 :=	$(BLK_DRIVER_VM_DTS_DIR)/init.dts \
										$(BLK_DRIVER_VM_DTS_DIR)/io.dts \
										$(BLK_DRIVER_VM_DTS_DIR)/disable.dts
BLK_DRIVER_VM_DTS_OVERLAYS := ${BLK_DRIVER_VM_DTS_OVERLAYS_${BOARD}}
BLK_DRIVER_VM_DTB := $(BUILD_DIR)/blk_driver_vm.dtb

SND_DRIVER_VM_LINUX := $(SND_DRIVER_VM_DIR)/linux
SND_DRIVER_VM_INITRD := $(SND_DRIVER_VM_DIR)/rootfs.cpio.gz
SND_DRIVER_VM_INITRD_MODIFIED := $(SND_DRIVER_VM_DIR)/snd_driver_vm_rootfs.cpio.gz
SND_DRIVER_VM_BASE_DTS := $(SND_DRIVER_VM_DTS_DIR)/linux.dts
SND_DRIVER_VM_DTS_OVERLAYS := $(SND_DRIVER_VM_DTS_DIR)/init.dts \
							$(SND_DRIVER_VM_DTS_DIR)/io.dts
SND_DRIVER_VM_DTB := $(BUILD_DIR)/snd_driver_vm.dtb


CLIENT_VM_LINUX := $(CLIENT_VM_DIR)/linux
CLIENT_VM_INITRD := $(CLIENT_VM_DIR)/rootfs.cpio.gz
CLIENT_VM_INITRD_MODIFIED := $(BUILD_DIR)/client_vm_rootfs.cpio.gz
CLIENT_VM_BASE_DTS := $(CLIENT_VM_DTS_DIR)/linux.dts
CLIENT_VM_DTS_OVERLAYS_qemu_arm_virt :=	$(CLIENT_VM_DTS_DIR)/init.dts \
										$(CLIENT_VM_DTS_DIR)/virtio.dts
CLIENT_VM_DTS_OVERLAYS_odroidc4 :=	$(CLIENT_VM_DTS_DIR)/init.dts \
										$(CLIENT_VM_DTS_DIR)/virtio.dts\
										$(CLIENT_VM_DTS_DIR)/disable.dts
CLIENT_VM_DTS_OVERLAYS := ${CLIENT_VM_DTS_OVERLAYS_${BOARD}}
CLIENT_VM_DTB := $(BUILD_DIR)/client_vm.dtb


SDDF_UTIL := $(SDDF_DIR)/util

SDDF_BLK_COMPONENTS := $(SDDF_DIR)/blk/components
SDDF_BLK_UTIL := $(SDDF_DIR)/blk/util

SDDF_SND_COMPONENTS := $(SDDF_DIR)/sound/components

SDDF_SERIAL_COMPONENTS := $(SDDF_DIR)/serial/components

SDDF_SERIAL_DRIVER_qemu_arm_virt := $(SDDF_DIR)/drivers/serial/arm
SDDF_SERIAL_DRIVER_odroidc4 := $(SDDF_DIR)/drivers/serial/meson
SDDF_SERIAL_DRIVER := ${SDDF_SERIAL_DRIVER_${BOARD}}

ELFS := client_vmm.elf blk_driver_vmm.elf snd_driver_vmm.elf \
		serial_virt_tx.elf serial_virt_rx.elf uart_driver.elf \
		blk_virt.elf sound_virt.elf

BLK_DRIVER_VM_USERLEVEL_qemu_arm_virt := uio_blk_driver
BLK_DRIVER_VM_USERLEVEL_odroidc4 := uio_blk_driver
BLK_DRIVER_VM_USERLEVEL := ${BLK_DRIVER_VM_USERLEVEL_${BOARD}}
BLK_DRIVER_VM_USERLEVEL_INIT_qemu_arm_virt := qemu_blk_driver_init
BLK_DRIVER_VM_USERLEVEL_INIT_odroidc4 := blk_driver_init
BLK_DRIVER_VM_USERLEVEL_INIT := ${BLK_DRIVER_VM_USERLEVEL_INIT_${BOARD}}

SND_DRIVER_VM_USERLEVEL := uio_snd_driver
SND_DRIVER_VM_USERLEVEL_INIT := $(LINUX_DIR)/snd/sound
SND_DRIVER_VM_USERLEVEL_ETC := $(LINUX_DIR)/snd/board/$(BOARD)/asound.conf
UIO_SND_BUILD_DIR := $(BUILD_DIR)/uio_snd_driver.dir

CLIENT_VM_USERLEVEL :=
CLIENT_VM_USERLEVEL_INIT_qemu_arm_virt := qemu_blk_client_init
CLIENT_VM_USERLEVEL_INIT_odroidc4 := blk_client_init
CLIENT_VM_USERLEVEL_INIT := ${CLIENT_VM_USERLEVEL_INIT_${BOARD}}

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
			vcpu.o

CLIENT_VMM_OBJS := $(VMM_OBJS) \
					client_images.o \
					client_vmm.o \
					console.o \
					block.o \
					sound.o \
					mmio.o \
					sddf_blk_bitarray.o \
					sddf_blk_fsmalloc.o \
					sddf_printf.o sddf_putchar_debug.o \

BLK_DRIVER_VMM_OBJS := $(VMM_OBJS) \
					blk_driver_images.o \
					blk_driver_vmm.o \
					console.o \
					mmio.o \

SND_DRIVER_VMM_OBJS :=	$(VMM_OBJS) \
						snd_driver_images.o \
						snd_driver_vmm.o \
						console.o \
						mmio.o \
						sddf_cache.o

# sddf_printf.o sdd_putchar_debug.o only needed during debug mode
SERIAL_VIRT_TX_OBJS := virt_tx.o sddf_printf.o sddf_putchar_debug.o
SERIAL_VIRT_RX_OBJS := virt_rx.o sddf_printf.o sddf_putchar_debug.o
SERIAL_DRIVER_OBJS := uart.o sddf_printf.o sddf_putchar_debug.o

BLK_VIRT_OBJS := 	sddf_blk_virt.o \
					sddf_cache.o \
					sddf_blk_fsmalloc.o \
					sddf_blk_bitarray.o \
					sddf_blk_util.o \
					sddf_printf.o \
					sddf_putchar_debug.o

SOUND_VIRT_OBJS :=	sddf_snd_virt.o \
					sddf_printf.o \
					sddf_putchar_debug.o \
					sddf_cache.o

UIO_BLK_DRIVER_OBJS := blk.o libuio.o
UIO_SND_DRIVER_OBJS := main.o stream.o queue.o convert.o

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
		  -I$(SDDF_DIR)/include \
		  -I$(LINUX_DIR)/include \
		  -DBOARD_$(BOARD) \
		  -DCONFIG_$(CONFIG) \
		  -target aarch64-none-elf

CFLAGS_LINUX :=	-g3 \
				-O3 \
				-Wno-unused-command-line-argument \
				-Wall -Wno-unused-function -Werror \
				-D_GNU_SOURCE \
				-I$(SDDF_DIR)/include \
				-I$(LINUX_DIR)/include \
				-I$(VMM_SRC_DIR) \
				-DBOARD_$(BOARD) \
				-target aarch64-linux-gnu

CFLAGS_UIO_SOUND := $(CFLAGS_LINUX) \
					-lasound -lm \
					$(NIX_LDFLAGS) \
					$(NIX_CFLAGS_COMPILE) \

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

ifeq ($(BOARD), qemu_arm_virt)
	NUM_PART = 2
	BLK_SIZE = 512
	BLK_MEM ?= 2101248
endif

all: directories $(IMAGE_FILE)

clean:
	rm -rf $(BUILD_DIR)

qemu: all $(BUILD_DIR)/storage
	# @ivanv: check that the amount of RAM given to QEMU is at least the number of RAM that QEMU is setup with for seL4.
	if ! command -v $(QEMU) &> /dev/null; then echo "Could not find dependency: qemu-system-aarch64"; exit 1; fi
	$(QEMU) -machine virt,virtualization=on,secure=off \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-audio driver=$(QEMU_SND_BACKEND),model=$(QEMU_SND_FRONTEND),id=$(QEMU_SND_BACKEND) \
			-m size=2G \
			-nographic \
			-drive file=$(BUILD_DIR)/storage,format=raw,if=none,id=drive0 \
			-device virtio-blk-device,drive=drive0,id=virtblk0,num-queues=1

directories:
	$(shell mkdir -p $(BUILD_DIR))

$(BUILD_DIR)/storage:
	$(VMM_TOOLS)/mkvirtdisk $@ $(NUM_PART) $(BLK_SIZE) $(BLK_MEM)

# Overlay rootfs, put the userlevel in /root and userlevel_init in /etc/init.d/S99, repack rootfs
$(BLK_DRIVER_VM_INITRD_MODIFIED): $(addprefix $(BUILD_DIR)/, $(BLK_DRIVER_VM_USERLEVEL)) \
								  $(addprefix $(BUILD_DIR)/, $(BLK_DRIVER_VM_USERLEVEL_INIT))
	$(VMM_TOOLS)/packrootfs $(BLK_DRIVER_VM_INITRD) $(BLK_DRIVER_VM_ROOTFS_TMP_DIR) -o $(BLK_DRIVER_VM_INITRD_MODIFIED) \
							--startup $(addprefix $(BUILD_DIR)/, $(BLK_DRIVER_VM_USERLEVEL_INIT)) \
							--home $(addprefix $(BUILD_DIR)/, $(BLK_DRIVER_VM_USERLEVEL))

$(SND_DRIVER_VM_INITRD_MODIFIED): $(addprefix $(BUILD_DIR)/, $(SND_DRIVER_VM_USERLEVEL)) \
								  $(SND_DRIVER_VM_USERLEVEL_INIT) \
								  $(SND_DRIVER_VM_ETC)
	$(VMM_TOOLS)/packrootfs $(SND_DRIVER_VM_INITRD) $(SND_DRIVER_VM_ROOTFS_TMP_DIR) -o $(SND_DRIVER_VM_INITRD_MODIFIED) \
							--startup $(SND_DRIVER_VM_USERLEVEL_INIT) \
							--home $(addprefix $(BUILD_DIR)/, $(SND_DRIVER_VM_USERLEVEL)) \
							--etc $(SND_DRIVER_VM_ETC)
	
$(CLIENT_VM_INITRD_MODIFIED): $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_USERLEVEL)) \
								$(addprefix $(BUILD_DIR)/, $(CLIENT_VM_USERLEVEL_INIT))
	$(VMM_TOOLS)/packrootfs $(CLIENT_VM_INITRD) $(CLIENT_VM_ROOTFS_TMP_DIR) -o $(CLIENT_VM_INITRD_MODIFIED) \
							--startup $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_USERLEVEL_INIT)) \
							--home $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_USERLEVEL))

$(BLK_DRIVER_VM_DTB): $(BLK_DRIVER_VM_BASE_DTS) $(BLK_DRIVER_VM_DTS_OVERLAYS)
	$(VMM_TOOLS)/dtscat $^ > $(BUILD_DIR)/blk_driver_vm.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/blk_driver_vm.dts > $@

$(SND_DRIVER_VM_DTB): $(SND_DRIVER_VM_BASE_DTS) $(SND_DRIVER_VM_DTS_OVERLAYS)
	sh ../../tools/dtscat $^ > $(BUILD_DIR)/snd_driver_vm.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/snd_driver_vm.dts > $@

$(CLIENT_VM_DTB): $(CLIENT_VM_BASE_DTS) $(CLIENT_VM_DTS_OVERLAYS)
	$(VMM_TOOLS)/dtscat $^ > $(BUILD_DIR)/client_vm.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/client_vm.dts > $@

$(BUILD_DIR)/blk_driver_images.o: $(VMM_TOOLS)/package_guest_images.S $(BLK_DRIVER_VM_LINUX) $(BLK_DRIVER_VM_DTB) $(BLK_DRIVER_VM_INITRD_MODIFIED)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(BLK_DRIVER_VM_LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(BLK_DRIVER_VM_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(BLK_DRIVER_VM_INITRD_MODIFIED)\" \
					-target aarch64-none-elf \
					$< -o $@

$(BUILD_DIR)/snd_driver_images.o: $(VMM_TOOLS)/package_guest_images.S $(SND_DRIVER_VM_LINUX) $(SND_DRIVER_VM_DTB) $(SND_DRIVER_VM_INITRD_MODIFIED)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(SND_DRIVER_VM_LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(SND_DRIVER_VM_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(SND_DRIVER_VM_INITRD_MODIFIED)\" \
					-target aarch64-none-elf \
					$< -o $@

$(BUILD_DIR)/client_images.o: $(VMM_TOOLS)/package_guest_images.S $(CLIENT_VM_LINUX) $(CLIENT_VM_DTB) $(CLIENT_VM_INITRD_MODIFIED)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(CLIENT_VM_LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(CLIENT_VM_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(CLIENT_VM_INITRD_MODIFIED)\" \
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

$(BUILD_DIR)/sddf_%.o: $(SDDF_UTIL)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SDDF_SERIAL_COMPONENTS)/%.c Makefile
	$(CC) -c $(CFLAGS) -DSERIAL_NUM_CLIENTS=4 -DSERIAL_TRANSFER_WITH_COLOUR=1 $< -o $@

$(BUILD_DIR)/%.o: $(SDDF_SERIAL_DRIVER)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/sddf_blk_%.o: $(SDDF_BLK_COMPONENTS)/%.c Makefile
	$(CC) -c $(CFLAGS) -DBLK_NUM_CLIENTS=2 $< -o $@

$(BUILD_DIR)/sddf_blk_%.o: $(SDDF_BLK_UTIL)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/sddf_snd_%.o: $(SDDF_SND_COMPONENTS)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/client_vmm.elf: $(addprefix $(BUILD_DIR)/, $(CLIENT_VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/blk_driver_vmm.elf: $(addprefix $(BUILD_DIR)/, $(BLK_DRIVER_VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/snd_driver_vmm.elf: $(addprefix $(BUILD_DIR)/, $(SND_DRIVER_VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/blk_virt.elf: $(addprefix $(BUILD_DIR)/, $(BLK_VIRT_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/sound_virt.elf: $(addprefix $(BUILD_DIR)/, $(SOUND_VIRT_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial_virt_tx.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_VIRT_TX_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial_virt_rx.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_VIRT_RX_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/uart_driver.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_DRIVER_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/postmark: $(LINUX_BLK_DIR)/postmark.c
	$(CC_USERLEVEL) $(CFLAGS_LINUX) $< -o $@

$(BUILD_DIR)/blk_client_init: $(LINUX_BLK_DIR)/blk_client_init
	cp $< $@

$(BUILD_DIR)/blk_driver_init: $(LINUX_BLK_DIR)/blk_driver_init
	cp $< $@

$(BUILD_DIR)/qemu_blk_client_init: $(LINUX_BLK_DIR)/qemu_blk_client_init
	cp $< $@

$(BUILD_DIR)/qemu_blk_driver_init: $(LINUX_BLK_DIR)/qemu_blk_driver_init
	cp $< $@

$(BUILD_DIR)/%.o: $(UIO_DIR)/%.c Makefile
	$(CC_USERLEVEL) -c $(CFLAGS_LINUX) $< -o $@

$(BUILD_DIR)/uio_blk_driver: $(addprefix $(BUILD_DIR)/, $(UIO_BLK_DRIVER_OBJS))
	$(CC_USERLEVEL) $(CFLAGS_LINUX) $^ -o $@

$(UIO_SND_BUILD_DIR)/%.o: $(UIO_DIR)/snd/%.c
	mkdir -p $(UIO_SND_BUILD_DIR)
	$(CC_USERLEVEL) -c $(CFLAGS_UIO_SOUND) $^ -o $@

$(BUILD_DIR)/uio_snd_driver: $(addprefix $(UIO_SND_BUILD_DIR)/, $(UIO_SND_DRIVER_OBJS))
	$(CC_USERLEVEL) $(CFLAGS_UIO_SOUND) $^ -o $@
	patchelf --set-interpreter /lib64/ld-linux-aarch64.so.1 $@

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(ELFS)) $(SYSTEM_DESCRIPTION)
	$(MICROKIT_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)