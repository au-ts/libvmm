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

BOARD_DIR := $(MICROKIT_SDK)/board/$(BOARD)/$(CONFIG)
VMM := ../../
VMM_TOOLS := $(VMM)/tools
VMM_SRC_DIR := $(VMM)/src
SYSTEM_DESCRIPTION := board/$(BOARD)/virtio-snd.system

CURRENT_DIR := $(shell pwd)

LINUX_DIR := $(VMM_TOOLS)/linux
LINUX_SND_DIR := $(LINUX_DIR)/snd
UIO_DRIVERS_DIR := $(LINUX_DIR)/uio_drivers

VM_USERLEVEL_DIR := userlevel/

SND_DRIVER_VM_DIR := board/$(BOARD)/snd_driver_vm
SND_DRIVER_VM_DTS_DIR := $(SND_DRIVER_VM_DIR)/dts
SND_DRIVER_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/snd_driver_vm_rootfs

CLIENT_VM_DIR := board/$(BOARD)/client_vm
CLIENT_VM_DTS_DIR := $(CLIENT_VM_DIR)/dts
CLIENT_VM_ROOTFS_TMP_DIR := $(BUILD_DIR)/client_vm_rootfs

SND_DRIVER_VM_LINUX := $(SND_DRIVER_VM_DIR)/linux
SND_DRIVER_VM_INITRD := $(SND_DRIVER_VM_DIR)/rootfs.cpio.gz
SND_DRIVER_VM_INITRD_OVERLAYED := $(BUILD_DIR)/snd_driver_rootfs.cpio.gz
SND_DRIVER_VM_BASE_DTS := $(SND_DRIVER_VM_DTS_DIR)/linux.dts
SND_DRIVER_VM_DTS_OVERLAYS := $(SND_DRIVER_VM_DTS_DIR)/init.dts \
							$(SND_DRIVER_VM_DTS_DIR)/io.dts

CLIENT_VM_LINUX := $(CLIENT_VM_DIR)/linux
CLIENT_VM_INITRD := $(CLIENT_VM_DIR)/rootfs.cpio.gz
CLIENT_VM_INITRD_OVERLAYED := $(BUILD_DIR)/client_vm_rootfs.cpio.gz
CLIENT_VM_BASE_DTS := $(CLIENT_VM_DTS_DIR)/linux.dts
CLIENT_VM_DTS_OVERLAYS :=	$(CLIENT_VM_DTS_DIR)/init.dts \
							$(CLIENT_VM_DTS_DIR)/io.dts

SND_DRIVER_VM_DTB := $(BUILD_DIR)/snd_driver_vm.dtb
CLIENT_VM_DTB := $(BUILD_DIR)/client_vm.dtb

SDDF_DIR := $(abspath ../../dep/sddf)

SDDF_SERIAL_DRIVER_qemu_arm_virt := $(SDDF_DIR)/drivers/serial/arm
SDDF_SERIAL_DRIVER_odroidc4 := $(SDDF_DIR)/drivers/serial/meson
SDDF_SERIAL_DRIVER := ${SDDF_SERIAL_DRIVER_${BOARD}}
SERIAL_CONFIG_INCLUDE:= include/serial_config

ELFS := client_vmm.elf \
		serial_virt_tx.elf \
		serial_virt_rx.elf \
		uart_driver.elf \
		sound_virt.elf \
		snd_driver_vmm.elf

SND_DRIVER_VM_USERLEVEL_ELFS := control.elf pcm_min.elf user_sound.elf pcm.elf record.elf feedback.elf
CLIENT_VM_USERLEVEL_ELFS := control.elf pcm_min.elf pcm.elf record.elf feedback.elf

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

SOUND_VIRT_OBJS :=	sound_virt.o \
					libsddf_util_debug.a 

SND_DRIVER_VMM_OBJS :=	$(VMM_OBJS) \
						snd_driver_images.o \
						snd_driver_vmm.o \
						console.o \
						mmio.o \
						libsddf_util_debug.a 

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
		  -I$(SDDF_DIR)/include \
		  -I$(SERIAL_CONFIG_INCLUDE) \
		  -I$(LINUX_DIR)/include \
		  -DBOARD_$(BOARD) \
		  -DCONFIG_$(CONFIG) \
		  -target aarch64-none-elf

# TODO: currently all userlevel procs link asound. this should instead be done per-process.
CFLAGS_USERLEVEL :=	-g \
					-Wno-unused-command-line-argument \
		  			-Wall -Wno-unused-function -Werror \
					-I$(SDDF_DIR) \
					-I$(SDDF_DIR)/include \
					-I$(VMM_SRC_DIR) \
					-I$(LINUX_DIR)/include \
					-DBOARD_$(BOARD) \
					-lasound \
					-lm \
					-target aarch64-linux-gnu \
					$(NIX_LDFLAGS) \
					$(NIX_CFLAGS_COMPILE)

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld $(BUILD_DIR)/libsddf_util_debug.a


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
	$(shell mkdir -p $(BUILD_DIR)/util)
	$(shell mkdir -p $(BUILD_DIR)/serial)

SDDF_LIB_UTIL_DBG_OBJS := cache.o sddf_printf.o newlibc.o assert.o putchar_debug.o bitarray.o fsmalloc.o

TOOLCHAIN ?= aarch64-none-elf
AR := $(TOOLCHAIN)-ar
RANLIB := ${TOOLCHAIN}-ranlib

$(BUILD_DIR)/libsddf_util_debug.a: $(addprefix ${BUILD_DIR}/util/, ${SDDF_LIB_UTIL_DBG_OBJS})
	${AR} rv $@ $^
	${RANLIB} $@

$(BUILD_DIR)/util/sddf_printf.o: ${SDDF_DIR}/util/printf.c
	${CC} ${CFLAGS} -c -o $@ $<

$(BUILD_DIR)/util/%.o: ${SDDF_DIR}/util/%.c
	${CC} ${CFLAGS} -c -o $@ $<

$(BUILD_DIR)/uart_driver.elf: $(BUILD_DIR)/serial/uart_driver.o
	$(LD) $(LDFLAGS) $< $(LIBS) -o $@

$(BUILD_DIR)/serial/uart_driver.o: ${SDDF_SERIAL_DRIVER}/uart.c
	$(CC) -c $(CFLAGS) -I${SDDF_SERIAL_DRIVER}/include -o $@ $< 

$(BUILD_DIR)/serial_virt_%.elf: $(BUILD_DIR)/serial/virt_%.o
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/serial/virt_%.o: ${SDDF_DIR}/serial/components/virt_%.c 
	${CC} ${CFLAGS} -DSERIAL_NUM_CLIENTS=3 -o $@ -c $<

SND_DRIVER_VM_USERLEVEL_INIT := $(LINUX_SND_DIR)/sound
SND_DRIVER_VM_ETC := $(LINUX_SND_DIR)/board/$(BOARD)/asound.conf

$(SND_DRIVER_VM_INITRD_OVERLAYED): $(addprefix $(BUILD_DIR)/, $(SND_DRIVER_VM_USERLEVEL_ELFS)) \
								  $(SND_DRIVER_VM_USERLEVEL_INIT) \
								  $(SND_DRIVER_VM_ETC)
	$(VMM_TOOLS)/packrootfs $(SND_DRIVER_VM_INITRD) $(SND_DRIVER_VM_ROOTFS_TMP_DIR) -o $(SND_DRIVER_VM_INITRD_OVERLAYED) \
							--startup $(SND_DRIVER_VM_USERLEVEL_INIT) \
							--home $(addprefix $(BUILD_DIR)/, $(SND_DRIVER_VM_USERLEVEL_ELFS)) \
							--etc $(SND_DRIVER_VM_ETC)
	
$(CLIENT_VM_INITRD_OVERLAYED): $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_USERLEVEL_ELFS)) \
								$(addprefix $(BUILD_DIR)/, $(CLIENT_VM_USERLEVEL_INIT))
	$(VMM_TOOLS)/packrootfs $(CLIENT_VM_INITRD) $(CLIENT_VM_ROOTFS_TMP_DIR) -o $(CLIENT_VM_INITRD_OVERLAYED) \
							--home $(addprefix $(BUILD_DIR)/, $(CLIENT_VM_USERLEVEL_ELFS))


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

$(BUILD_DIR)/%.elf: $(VM_USERLEVEL_DIR)/%.c
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

$(BUILD_DIR)/sound_%.o: $(SDDF_SOUND_COMPONENTS)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/client_vmm.elf: $(addprefix $(BUILD_DIR)/, $(CLIENT_VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/sound_virt.elf: $(addprefix $(BUILD_DIR)/, $(SOUND_VIRT_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/snd_driver_vmm.elf: $(addprefix $(BUILD_DIR)/, $(SND_DRIVER_VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(ELFS)) $(SYSTEM_DESCRIPTION) $(SND_DRIVER_VM_DIR) $(CLIENT_VM_DIR)
	$(MICROKIT_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) $(SND_DRIVER_VM_DIR) $(CLIENT_VM_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

clean:
	rm -rf build
