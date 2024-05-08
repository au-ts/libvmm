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
VMM := ../../
VMM_TOOLS := $(VMM)/tools
VMM_SRC_DIR := $(VMM)/src
SYSTEM_DESCRIPTION := board/$(BOARD)/virtio-snd.system

CURRENT_DIR := $(shell pwd)

LINUX_DIR := $(VMM_TOOLS)/linux
LINUX_SND_DIR := $(LINUX_DIR)/snd
UIO_DRIVERS_DIR := $(LINUX_DIR)/uio_drivers

SND_DRIVER_VM_DIR := board/$(BOARD)/snd_driver_vm
SND_DRIVER_VM_DTS_DIR := $(SND_DRIVER_VM_DIR)/dts
SND_DRIVER_VM_USERLEVEL_DIR := userlevel/snd_driver_vm
SND_DRIVER_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/snd_driver_vm_rootfs

CLIENT_VM_DIR := board/$(BOARD)/client_vm
CLIENT_VM_DTS_DIR := $(CLIENT_VM_DIR)/dts
CLIENT_VM_USERLEVEL_DIR := userlevel/client_vm
CLIENT_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/client_vm_rootfs

SND_DRIVER_VM_LINUX := $(SND_DRIVER_VM_DIR)/linux
SND_DRIVER_VM_INITRD := $(SND_DRIVER_VM_DIR)/rootfs.cpio.gz
SND_DRIVER_VM_INITRD_OVERLAYED := $(SND_DRIVER_VM_DIR)/rootfs_overlayed.cpio.gz
SND_DRIVER_VM_BASE_DTS := $(SND_DRIVER_VM_DTS_DIR)/linux.dts
SND_DRIVER_VM_DTS_OVERLAYS := $(SND_DRIVER_VM_DTS_DIR)/init.dts \
							$(SND_DRIVER_VM_DTS_DIR)/io.dts

CLIENT_VM_LINUX := $(CLIENT_VM_DIR)/linux
CLIENT_VM_INITRD := $(CLIENT_VM_DIR)/rootfs.cpio.gz
CLIENT_VM_INITRD_OVERLAYED := $(CLIENT_VM_DIR)/rootfs_overlayed.cpio.gz
CLIENT_VM_BASE_DTS := $(CLIENT_VM_DTS_DIR)/linux.dts
CLIENT_VM_DTS_OVERLAYS :=	$(CLIENT_VM_DTS_DIR)/init.dts \
							$(CLIENT_VM_DTS_DIR)/io.dts

SND_DRIVER_VM_DTB := $(BUILD_DIR)/snd_driver_vm.dtb
CLIENT_VM_DTB := $(BUILD_DIR)/client_vm.dtb

SDDF := sddf

SDDF_SERIAL_COMPONENTS := $(SDDF)/serial/components
SDDF_SOUND_COMPONENTS := $(SDDF)/sound/components
SDDF_UTIL := $(SDDF)/util

SDDF_SERIAL_DRIVER_qemu_arm_virt := $(SDDF)/drivers/serial/arm
SDDF_SERIAL_DRIVER_odroidc4 := $(SDDF)/drivers/serial/meson
SDDF_SERIAL_DRIVER := ${SDDF_SERIAL_DRIVER_${BOARD}}

ELFS := client_vmm.elf \
		serial_virt_tx.elf \
		serial_virt_rx.elf \
		uart_driver.elf \
		sound_virt.elf \
		snd_driver_vmm.elf

SND_DRIVER_VM_USERLEVEL_ELFS := control.elf pcm_min.elf user_sound.elf pcm.elf record.elf cantina.elf crab_rave.elf feedback.elf
CLIENT_VM_USERLEVEL_ELFS := control.elf pcm_min.elf pcm.elf record.elf cantina.elf crab_rave.elf feedback.elf

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
					console.o \
					sound.o \
					mmio.o
# block.o

SOUND_VIRT_OBJS :=	sound_virt.o \
					sddf_printf.o \
					sddf_putchar_debug.o \
					sddf_cache.o

SND_DRIVER_VMM_OBJS :=	$(VMM_OBJS) \
						snd_driver_images.o \
						snd_driver_vmm.o \
						console.o \
						mmio.o \
						sddf_cache.o

SERIAL_VIRT_TX_OBJS := virt_tx.o sddf_printf.o sddf_putchar_debug.o
SERIAL_VIRT_RX_OBJS := virt_rx.o sddf_printf.o sddf_putchar_debug.o
SERIAL_DRIVER_OBJS := uart.o sddf_printf.o sddf_putchar_debug.o

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
		  -fno-stack-protector \
		  -nostdlib \
		  -Wno-unused-command-line-argument \
		  -Wall -Wno-unused-function -Werror \
		  -I$(VMM_SRC_DIR)/arch/aarch64 -I$(VMM_SRC_DIR) -I$(VMM_SRC_DIR)/util -I$(BOARD_DIR)/include \
		  -I$(SDDF_SERIAL_DRIVER)/include \
		  -I$(SDDF)/include \
		  -I$(LINUX_DIR)/include \
		  -DBOARD_$(BOARD) \
		  -DCONFIG_$(CONFIG) \
		  -target aarch64-none-elf

# @alexbr: currently all userlevel procs link asound. this should instead be done per-process.
CFLAGS_USERLEVEL :=	-g \
					-Wno-unused-command-line-argument \
		  			-Wall -Wno-unused-function -Werror \
					-I$(SDDF) \
					-I$(SDDF)/include \
					-I$(VMM_SRC_DIR) \
					-I$(LINUX_DIR)/include \
					-DBOARD_$(BOARD) \
					-lasound \
					-lm \
					-target aarch64-linux-gnu \
					$(NIX_LDFLAGS) \
					$(NIX_CFLAGS_COMPILE)

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld


.PHONY: all qemu clean

all: directories $(IMAGE_FILE)

qemu: all
	# @ivanv: check that the amount of RAM given to QEMU is at least the number of RAM that QEMU is setup with for seL4.
	if ! command -v $(QEMU) &> /dev/null; then echo "Could not find dependency: qemu-system-aarch64"; exit 1; fi
	$(QEMU) -machine virt,virtualization=on,secure=off \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-audio driver=$(QEMU_SND_BACKEND),model=$(QEMU_SND_FRONTEND),id=$(QEMU_SND_BACKEND) \
			-m size=2G \
			-nographic

directories:
	$(shell mkdir -p $(BUILD_DIR))

# Append to rootfs

UIO_ASOUND_CONF := $(LINUX_SND_DIR)/asound_$(BOARD).conf

$(SND_DRIVER_VM_INITRD_OVERLAYED): $(addprefix $(BUILD_DIR)/, $(SND_DRIVER_VM_USERLEVEL_ELFS)) $(LINUX_SND_DIR)/S95sound $(UIO_ASOUND_CONF)
	rm -rf $(SND_DRIVER_VM_ROOTFS_TMP_DIR) && \
	mkdir -p $(SND_DRIVER_VM_ROOTFS_TMP_DIR)/root $(SND_DRIVER_VM_ROOTFS_TMP_DIR)/etc/init.d && \
	cp $(addprefix $(BUILD_DIR)/, $(SND_DRIVER_VM_USERLEVEL_ELFS)) $(SND_DRIVER_VM_ROOTFS_TMP_DIR)/root && \
	cp $(LINUX_SND_DIR)/S95sound $(SND_DRIVER_VM_ROOTFS_TMP_DIR)/etc/init.d && \
	chmod +x $(SND_DRIVER_VM_ROOTFS_TMP_DIR)/etc/init.d/* && \
	cp $(UIO_ASOUND_CONF) $(SND_DRIVER_VM_ROOTFS_TMP_DIR)/etc/asound.conf && \
	./pack.sh $(SND_DRIVER_VM_ROOTFS_TMP_DIR) $(SND_DRIVER_VM_INITRD) "$(CURRENT_DIR)/$(SND_DRIVER_VM_INITRD_OVERLAYED)"

$(CLIENT_VM_INITRD_OVERLAYED): $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_USERLEVEL_ELFS))
	rm -rf $(CLIENT_VM_ROOTFS_TMP_DIR) && \
	mkdir -p $(CLIENT_VM_ROOTFS_TMP_DIR)/root && \
	cp $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_USERLEVEL_ELFS)) $(CLIENT_VM_ROOTFS_TMP_DIR)/root && \
	./pack.sh $(CLIENT_VM_ROOTFS_TMP_DIR) $(CLIENT_VM_INITRD) $(CURRENT_DIR)/$(CLIENT_VM_INITRD_OVERLAYED) "$^"

$(SND_DRIVER_VM_DTB): $(SND_DRIVER_VM_BASE_DTS) $(SND_DRIVER_VM_DTS_OVERLAYS)
	sh ../../tools/dtscat $^ > $(BUILD_DIR)/snd_driver_vm.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/snd_driver_vm.dts > $@

$(BUILD_DIR)/snd_driver_images.o: $(VMM_TOOLS)/package_guest_images.S $(SND_DRIVER_VM_LINUX) $(SND_DRIVER_VM_DTB) $(SND_DRIVER_VM_INITRD_OVERLAYED)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(SND_DRIVER_VM_LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(SND_DRIVER_VM_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(SND_DRIVER_VM_INITRD_OVERLAYED)\" \
					-target aarch64-none-elf \
					$< -o $@

$(CLIENT_VM_DTB): $(CLIENT_VM_BASE_DTS) $(CLIENT_VM_DTS_OVERLAYS)
	sh ../../tools/dtscat $^ > $(BUILD_DIR)/client_vm.dts
	$(DTC) -q -I dts -O dtb $(BUILD_DIR)/client_vm.dts > $@

$(BUILD_DIR)/client_images.o: $(VMM_TOOLS)/package_guest_images.S $(CLIENT_VM_LINUX) $(CLIENT_VM_DTB) $(CLIENT_VM_INITRD_OVERLAYED)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(CLIENT_VM_LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(CLIENT_VM_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(CLIENT_VM_INITRD_OVERLAYED)\" \
					-target aarch64-none-elf \
					$< -o $@

$(BUILD_DIR)/user_sound/%.o: $(UIO_DRIVERS_DIR)/snd/%.c
	mkdir -p $(BUILD_DIR)/user_sound
	$(CC_USERLEVEL) -c $(CFLAGS_USERLEVEL) $^ -o $@

$(BUILD_DIR)/user_sound.elf: $(BUILD_DIR)/user_sound/main.o $(BUILD_DIR)/user_sound/stream.o $(BUILD_DIR)/user_sound/queue.o $(BUILD_DIR)/user_sound/convert.o
	$(CC_USERLEVEL) $(CFLAGS_USERLEVEL) $^ -o $@
	patchelf --set-interpreter /lib64/ld-linux-aarch64.so.1 $@

$(BUILD_DIR)/%.elf: $(SND_DRIVER_VM_USERLEVEL_DIR)/%.c
	$(CC_USERLEVEL) $(CFLAGS_USERLEVEL) $^ -o $@
	patchelf --set-interpreter /lib64/ld-linux-aarch64.so.1 $@

$(BUILD_DIR)/%.elf: $(CLIENT_VM_USERLEVEL_DIR)/%.c
	$(CC_USERLEVEL) $(CFLAGS_USERLEVEL) $^ -o $@
	patchelf --set-interpreter /lib64/ld-linux-aarch64.so.1 $@

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
	$(CC) -c $(CFLAGS) -DSERIAL_NUM_CLIENTS=3 -DSERIAL_TRANSFER_WITH_COLOUR=1 $< -o $@

$(BUILD_DIR)/%.o: $(SDDF_SERIAL_DRIVER)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/sound_%.o: $(SDDF_SOUND_COMPONENTS)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/client_vmm.elf: $(addprefix $(BUILD_DIR)/, $(CLIENT_VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/sound_virt.elf: $(addprefix $(BUILD_DIR)/, $(SOUND_VIRT_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/snd_driver_vmm.elf: $(addprefix $(BUILD_DIR)/, $(SND_DRIVER_VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial_virt_tx.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_VIRT_TX_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial_virt_rx.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_VIRT_RX_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/uart_driver.elf: $(addprefix $(BUILD_DIR)/, $(SERIAL_DRIVER_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@


$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(ELFS)) $(SYSTEM_DESCRIPTION) $(SND_DRIVER_VM_DIR) $(CLIENT_VM_DIR)
	$(MICROKIT_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) $(SND_DRIVER_VM_DIR) $(CLIENT_VM_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

clean:
	rm -rf build
