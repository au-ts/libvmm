#
# Copyright 2021, Breakaway Consulting Pty. Ltd.
# Copyright 2022, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#
ifneq ($(strip $(MAKECMDGOALS)), clean)
ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif

ifeq ($(strip $(BOARD)),)
$(error BOARD must be specified)
endif

TARGET_BOARD := $(BOARD)

ifeq ($(TARGET_BOARD), qemu_arm_virt)
TARGET_BOARD := qemu-aarch64-virt
endif

ifeq ($(TARGET_BOARD), imx8mq_evk)
TARGET_BOARD := imx8qm
endif

# Tests must be one of
# - mibench
# - mibench+interf

ifeq ($(strip $(TEST)),)
$(error TEST must be specified)
endif
endif
# Default build directory, pass BUILD_DIR=<dir> to override
BUILD_DIR ?= build
TEST_BUILD_DIR := output/$(BOARD)/$(TEST)
# Default config is a debug build, pass CONFIG=<debug/release/benchmark> to override
CONFIG ?= debug

# @ivanv: Check for dependencies and make sure they are installed/in the path

# @ivanv: check that all dependencies exist
# Specify that we use bash for all shell commands
SHELL=/bin/bash
# All dependencies needed to compile the VMM
QEMU := qemu-system-aarch64
DTC := dtc

# ifndef TOOLCHAIN
# 	# Get whether the common toolchain triples exist
# 	TOOLCHAIN_AARCH64_LINUX_GNU := $(shell command -v aarch64-linux-gnu-gcc 2> /dev/null)
# 	TOOLCHAIN_AARCH64_UNKNOWN_LINUX_GNU := $(shell command -v aarch64-unknown-linux-gnu-gcc 2> /dev/null)
# 	# Then check if they are defined and select the appropriate one
# 	ifdef TOOLCHAIN_AARCH64_LINUX_GNU
# 		TOOLCHAIN := aarch64-linux-gnu
# 	else ifdef TOOLCHAIN_AARCH64_UNKNOWN_LINUX_GNU
# 		TOOLCHAIN := aarch64-unknown-linux-gnu
# 	else
# 		$(error "Could not find an AArch64 cross-compiler")
# 	endif
# endif

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
SYSTEM_DESCRIPTION := board/$(BOARD)/$(TEST).system
ifneq ($(strip $(MAKECMDGOALS)), clean)
ifeq ("$(wildcard $(SYSTEM_DESCRIPTION))","")
$(error specified TEST does not exist!)
endif
endif


IMAGE_DIR := board/$(BOARD)
LINUX := $(IMAGE_DIR)/linux

INTERF_IMAGE_DIR := interf/build/$(TARGET_BOARD)
INTERF := $(INTERF_IMAGE_DIR)/baremetal.bin
INTERF_DO_READ := 1
BARE_IS_INTERF := 0

IRQLAT_IMAGE_DIR := irqlat/build/$(TARGET_BOARD)
IRQLAT := $(IRQLAT_IMAGE_DIR)/baremetal.bin

IRQSTORM_IMAGE_DIR := irqstorm/build/$(TARGET_BOARD)
IRQSTORM := $(IRQSTORM_IMAGE_DIR)/baremetal.bin

BOOT_IMAGE_DIR := boot/build/$(TARGET_BOARD)/128k
BOOT := $(BOOT_IMAGE_DIR)/baremetal.bin


DTS := $(IMAGE_DIR)/linux.dts
DTB := $(BUILD_DIR)/linux.dtb
INITRD := $(IMAGE_DIR)/rootfs.cpio.gz

WTF_PATCH ?= no

ELFS := 

IMAGE_FILE = $(TEST_BUILD_DIR)/loader.img
REPORT_FILE = $(TEST_BUILD_DIR)/report.txt

OBJS := printf.o virq.o linux.o guest.o psci.o smc.o fault.o util.o vgic.o package_guest_images.o tcb.o vcpu.o temp_bench.o

# check if the board is the maaxboard
ifeq ($(BOARD), maaxboard)
	OBJS += vgic_v3.o
else
	OBJS += vgic_v2.o
endif

# @ivanv: should only compile printf.o in debug
VMM_OBJS := vmm.o $(OBJS)
BARE_OBJS := bare_vmm.o $(OBJS)
INTERF_OBJS := interf_vmm.o $(OBJS)
BENCH_OBJS := benchmark.o printf.o virq.o linux.o guest.o psci.o smc.o fault.o util.o vgic.o vgic_v2.o tcb.o vcpu.o temp_bench.o
IDLE_OBJS := idle.o printf.o virq.o linux.o guest.o psci.o smc.o fault.o util.o vgic.o vgic_v2.o tcb.o vcpu.o temp_bench.o

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
		  -Wall -Wno-unused-function \
		  -I$(VMM_SRC_DIR)/arch/aarch64 -I$(VMM_SRC_DIR) -I$(VMM_SRC_DIR)/util -I$(BOARD_DIR)/include \
		  -DBOARD_$(BOARD) \
		  -DCONFIG_$(CONFIG) \
		  -DBENCH_$(BENCH) \
		  -target aarch64-none-elf

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld


DEPS := 

ifeq ($(findstring interf,$(TEST)), interf)
	DEPS += interf
endif

ifeq ($(findstring irqlat,$(TEST)), irqlat)
	DEPS += irqlat
endif

ifeq ($(findstring irqstorm,$(TEST)), irqstorm)
	DEPS += irqstorm
endif

ifeq ($(findstring boot,$(TEST)), boot)
	DEPS += boot
endif



all: $(DEPS) wtf_patch directories $(IMAGE_FILE)

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
	$(shell mkdir -p $(TEST_BUILD_DIR))

wtf_patch:
	$(shell if [ '$(WTF_PATCH)' = 'yes' ] ; \
	then \
		echo $(TEST) | sed -e 's/ .*//' | sed -e 's/\s\+/\n/g' | xargs --replace=% ./wtf.py % $(TARGET_BOARD); \
	fi)

boot:
	$(MAKE) -C boot IMG_SZ_KB=128 PLATFORM=$(TARGET_BOARD)

interf:
	$(MAKE) -C interf READ_SWITCH=$(INTERF_DO_READ) PLATFORM=$(TARGET_BOARD)

irqlat:
	$(MAKE) -C irqlat PLATFORM=$(TARGET_BOARD)

irqstorm:
	$(MAKE) -C irqstorm PLATFORM=$(TARGET_BOARD)

clean:
	@rm -rf build
	$(MAKE) -C interf clean
	$(MAKE) -C irqlat clean
	$(MAKE) -C irqstorm clean

$(DTB): $(DTS)
	if ! command -v $(DTC) &> /dev/null; then echo "Could not find dependency: Device Tree Compiler (dtc)"; exit 1; fi
	# @ivanv: Shouldn't supress warnings
	$(DTC) -q -I dts -O dtb $< > $@

PATHS := 
ELFS := 

ifeq ($(findstring mibench,$(TEST)),mibench)
	PATHS := \
		-DGUEST_KERNEL_IMAGE_PATH=\"$(LINUX)\" \
		-DGUEST_DTB_IMAGE_PATH=\"$(DTB)\" \
		-DGUEST_INITRD_IMAGE_PATH=\"$(INITRD)\" \


	ELFS += vmm.elf

endif

ifeq ($(findstring +bench,$(TEST)),+bench)


	ELFS += benchmark.elf idle.elf

endif

# If the $(TEST) variable contains the string "interf", add the interf image path to paths
ifeq ($(findstring interf,$(TEST)),interf)
	PATHS += \
	-DGUEST_INTERF_IMAGE_PATH=\"$(INTERF)\" \

	ELFS +=  interf_vmm.elf

endif

# Various test image linking
ifeq ($(findstring irqlat,$(TEST)),irqlat)
	PATHS += \
	-DGUEST_BARE_IMAGE_PATH=\"$(IRQLAT)\" \

	INTERF_DO_READ = 0
	ELFS +=  bare_vmm.elf

endif

ifeq ($(findstring irqstorm,$(TEST)),irqstorm)
	PATHS += \
	-DGUEST_BARE_IMAGE_PATH=\"$(IRQSTORM)\" \

	INTERF_DO_READ = 0
	ELFS +=  bare_vmm.elf

endif


ifeq ($(findstring boot,$(TEST)),boot)
	PATHS += \
	-DGUEST_BARE_IMAGE_PATH=\"$(BOOT)\" \

	INTERF_DO_READ = 0
	ELFS +=  bare_vmm.elf

endif


$(BUILD_DIR)/package_guest_images.o: $(VMM_TOOLS)/package_guest_images.S $(IMAGE_DIR) $(LINUX) $(INITRD) $(DTB)	FORCE
	$(CC) -c -g3 -x assembler-with-cpp \
					$(PATHS) \
					-target aarch64-none-elf \
					$< -o $@
FORCE: ;


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

$(BUILD_DIR)/benchmark.elf: $(addprefix $(BUILD_DIR)/, $(BENCH_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/idle.elf: $(addprefix $(BUILD_DIR)/, $(IDLE_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/vmm.elf: $(addprefix $(BUILD_DIR)/, $(VMM_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/bare_vmm.elf: $(addprefix $(BUILD_DIR)/, $(BARE_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/interf_vmm.elf: $(addprefix $(BUILD_DIR)/, $(INTERF_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(ELFS)) $(SYSTEM_DESCRIPTION) $(IMAGE_DIR)
	$(MICROKIT_TOOL) $(SYSTEM_DESCRIPTION) --search-path $(BUILD_DIR) $(IMAGE_DIR) --board $(BOARD) --config $(CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)
