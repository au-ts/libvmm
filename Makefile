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

ifeq ($(strip $(SEL4CP_BOARD)),)
$(error SEL4CP_BOARD must be specified)
endif

ifeq ($(strip $(SEL4CP_CONFIG)),)
$(error SEL4CP_CONFIG must be specified)
endif

ifeq ($(strip $(SYSTEM)),)
$(error SYSTEM must be specified)
endif

TOOLCHAIN := aarch64-none-elf

CPU := cortex-a53

DTC := dtc
CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
AS := $(TOOLCHAIN)-as
SEL4CP_TOOL ?= $(SEL4CP_SDK)/bin/sel4cp

VMM_OBJS := vmm.o printf.o psci.o smc.o fault.o util.o vgic.o

# @ivanv: hack...
ifeq ($(SEL4CP_BOARD),imx8mm_evk)
	VMM_OBJS += vgic_v3.o
else
	VMM_OBJS += vgic_v2.o
endif

# @ivanv: Have a list of supported boards to check with, if it's not one of those
# have a helpful message that lists all the support boards.

SEL4CP_BOARD_DIR := $(SEL4CP_SDK)/board/$(SEL4CP_BOARD)/$(SEL4CP_CONFIG)
SRC_DIR := src
IMAGE_DIR := board/$(SEL4CP_BOARD)/images
SYSTEM_DESCRIPTION := board/$(SEL4CP_BOARD)/systems/$(SYSTEM)

IMAGES := vmm.elf linux.dtb

ifeq ($(SEL4CP_BOARD),qemu_arm_virt)
	IMAGES += linux_v5.18.dtb
endif

# @ivanv: no optimisation level for now, come back to this though
CFLAGS := -mcpu=$(CPU) -mstrict-align -nostdlib -ffreestanding -g3 -Wall -Wno-unused-function -Werror -I$(SEL4CP_BOARD_DIR)/include -DBOARD_$(SEL4CP_BOARD)
LDFLAGS := -L$(SEL4CP_BOARD_DIR)/lib
LIBS := -lsel4cp -Tsel4cp.ld

IMAGE_FILE = $(BUILD_DIR)/loader.img
REPORT_FILE = $(BUILD_DIR)/report.txt

all: $(IMAGE_FILE)

run: $(IMAGE_FILE)
	qemu-system-aarch64 -machine virt,virtualization=on,highmem=off,secure=off -cpu $(CPU) -serial mon:stdio -device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 -m size=2G -nographic

$(BUILD_DIR)/%.dtb: $(IMAGE_DIR)/%.dts Makefile
	# @ivanv: Shouldn't supress warnings
	$(DTC) -q -I dts -O dtb $< > $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/util/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/vgic/%.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/vmm.elf: $(addprefix $(BUILD_DIR)/, $(VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(IMAGES)) $(SYSTEM_DESCRIPTION)
	$(SEL4CP_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) $(IMAGE_DIR) --board $(SEL4CP_BOARD) --config $(SEL4CP_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)
