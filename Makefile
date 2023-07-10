#
# Copyright 2021, Breakaway Consulting Pty. Ltd.
# Copyright 2022, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#
ifeq ($(strip $(BUILD_DIR)),)
$(error BUILD_DIR must be specified)
endif

ifeq ($(strip $(SEL4CP_SDK)),)
$(error SEL4CP_SDK must be specified)
endif

ifeq ($(strip $(BOARD)),)
$(error BOARD must be specified)
endif

ifeq ($(strip $(CONFIG)),)
$(error CONFIG must be specified)
endif

ifeq ($(strip $(SYSTEM)),)
$(error SYSTEM must be specified)
endif

# @ivanv: Check for dependencies and make sure they are installed/in the path

# @ivanv: check that all dependencies exist
# Specify that we use bash for all shell commands
SHELL=/bin/bash
# All dependencies needed to compile the VMM
QEMU := qemu-system-aarch64
QEMU_SIZE := 2G
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

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
AS := $(TOOLCHAIN)-as
SEL4CP_TOOL ?= $(SEL4CP_SDK)/bin/sel4cp

# @ivanv: should only compile printf.o in debug
VMM_OBJS := vmm.o printf.o psci.o smc.o fault.o util.o vgic.o global_data.o

# @ivanv: hack...
# This step should be done based on the DTB
ifeq ($(BOARD),imx8mm_evk)
	VMM_OBJS += vgic_v3.o
else
	VMM_OBJS += vgic_v2.o
endif

# @ivanv: need to have a step for putting in the initrd node into the DTB,
# 		  right now it is unfortunately hard-coded.

# @ivanv: check that the path of SDK_PATH/BOARD exists
# @ivanv: Have a list of supported boards to check with, if it's not one of those
# have a helpful message that lists all the support boards.

# @ivanv: incremental builds don't work with IMAGE_DIR changing

BOARD_DIR := $(SEL4CP_SDK)/board/$(BOARD)/$(CONFIG)
SRC_DIR := src
SYSTEM_DESCRIPTION := board/$(BOARD)/systems/$(SYSTEM)

IMAGE_DIR := board/$(BOARD)/images
KERNEL_IMAGE := $(IMAGE_DIR)/linux
DTS := $(IMAGE_DIR)/linux.dts
DTB_IMAGE := $(BUILD_DIR)/linux.dtb
INITRD_IMAGE := $(IMAGE_DIR)/rootfs.cpio.gz
IMAGES := vmm.elf
IMAGE_FILE = $(BUILD_DIR)/loader.img
REPORT_FILE = $(BUILD_DIR)/report.txt

# Toolchain flags
# FIXME: For optimisation we should consider providing the flag -mcpu.
# FIXME: We should also consider whether -mgeneral-regs-only should be
# used to avoid the use of the FPU and therefore seL4 does not have to
# context switch the FPU.
CFLAGS := -mstrict-align -nostdlib -ffreestanding -g3 -O3 -Wall -Wno-unused-function -Werror -I$(BOARD_DIR)/include -DBOARD_$(BOARD) -DCONFIG_$(CONFIG)
LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lsel4cp -Tsel4cp.ld

ifdef VIRTIO_MMIO
VMM_OBJS += virtio_mmio.o virtio_gpu_emul.o virtio_gpu_uio.o virtio_net_emul.o virtio_net_vswitch.o shared_ringbuffer.o
CFLAGS += -DVIRTIO_MMIO
QEMU_SIZE := 4G
endif

ifdef VIRTIO_NET
DTS := $(IMAGE_DIR)/linux_virtio_net.dts
CFLAGS += -DVIRTIO_NET
endif

ifdef VIRTIO_GPU
DTS := $(IMAGE_DIR)/linux_virtio_gpu.dts
CFLAGS += -DVIRTIO_GPU
endif

all: directories $(IMAGE_FILE)

run: directories $(IMAGE_FILE)
	# @ivanv: check that the amount of RAM given to QEMU is at least the number of RAM that QEMU is setup with for seL4.
	if ! command -v $(QEMU) &> /dev/null; then echo "Could not find dependenyc: qemu-system-aarch64"; exit 1; fi
	$(QEMU) -machine virt,virtualization=on,secure=off \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-m size=$(QEMU_SIZE) \
			-nographic

directories:
	$(shell mkdir -p $(BUILD_DIR))

$(DTB_IMAGE): $(DTS)
	if ! command -v $(DTC) &> /dev/null; then echo "Could not find dependency: Device Tree Compiler (dtc)"; exit 1; fi
	# @ivanv: Shouldn't supress warnings
	$(DTC) -q -I dts -O dtb $< > $@

$(BUILD_DIR)/global_data.o: $(SRC_DIR)/global_data.S $(IMAGE_DIR) $(KERNEL_IMAGE) $(INITRD_IMAGE) $(DTB_IMAGE)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DVM_KERNEL_IMAGE_PATH=\"$(KERNEL_IMAGE)\" \
					-DVM_DTB_IMAGE_PATH=\"$(DTB_IMAGE)\" \
					-DVM_INITRD_IMAGE_PATH=\"$(INITRD_IMAGE)\" \
					$< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/util/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/vgic/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

ifdef VIRTIO_MMIO
$(BUILD_DIR)/%.o: $(SRC_DIR)/virtio/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/libsharedringbuffer/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@
endif

$(BUILD_DIR)/vmm.elf: $(addprefix $(BUILD_DIR)/, $(VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(IMAGES)) $(SYSTEM_DESCRIPTION) $(IMAGE_DIR)
	$(SEL4CP_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) $(IMAGE_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)
