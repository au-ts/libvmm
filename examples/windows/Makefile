#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

BUILD_DIR ?= build
export MICROKIT_CONFIG ?= debug

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif
override MICROKIT_SDK := $(abspath $(MICROKIT_SDK))

# Allow to user to specify a custom partition
PARTITION :=
ifdef PARTITION
export PARTITION_ARG := --partition $(PARTITION)
endif

BUILD_DIR ?= build
export BUILD_DIR := $(abspath ${BUILD_DIR})
export WINDOWS_EXAMPLE := $(abspath .)

export CC_USERLEVEL := clang
export MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
export SDDF := $(abspath ../../dep/sddf)
export LIBVMM := $(abspath ../../)

IMAGE_FILE := $(BUILD_DIR)/loader.img
REPORT_FILE := $(BUILD_DIR)/report.txt

all: $(IMAGE_FILE)

qemu $(IMAGE_FILE) $(REPORT_FILE) clean clobber: $(BUILD_DIR)/Makefile FORCE
	$(MAKE) -C $(BUILD_DIR) MICROKIT_SDK=$(MICROKIT_SDK) $(notdir $@)

$(BUILD_DIR)/Makefile: windows.mk
	mkdir -p $(BUILD_DIR)
	cp windows.mk $@

FORCE:
