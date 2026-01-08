#
# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#


MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
ARCH := ${shell grep 'CONFIG_SEL4_ARCH  ' $(BOARD_DIR)/include/kernel/gen_config.h | cut -d' ' -f4}
SYSTEM_DIR := $(EXAMPLE_DIR)/board/$(MICROKIT_BOARD)
SYSTEM_FILE := $(SYSTEM_DIR)/uefi.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt

SDDF_CUSTOM_LIBC := 1

vpath %.c $(LIBVMM) $(EXAMPLE_DIR)

ifeq ($(ARCH),x86_64)
	FIRM ?= $(SYSTEM_DIR)/OVMF.fd
	ARCH_FLAGS := -target x86_64-unknown-elf
	IMAGES = vmm_x86_64.elf timer_driver.elf
else
$(error Unsupported ARCH given)
endif

ifeq ($(strip $(MICROKIT_BOARD)), x86_64_generic_vtx)
	KERNEL = sel4.elf
	KERNEL32 := sel4_32.elf
	QEMU := qemu-system-x86_64
	QEMU_ARCH_ARGS := -accel kvm -cpu host,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme -kernel $(KERNEL32) -initrd $(IMAGE_FILE)
	TIMER_DRIVER_DIR := hpet
else
$(error Unsupported MICROKIT_BOARD given)
endif

CFLAGS := \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -DBOARD_$(MICROKIT_BOARD) \
	  -I$(BOARD_DIR)/include \
	  -I$(LIBVMM)/include \
	  -I$(SDDF)/include \
	  -I$(SDDF)/include/sddf/util/custom_libc \
	  -I$(SDDF)/include/microkit \
	  -MD \
	  -MP \
	  $(ARCH_FLAGS)

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld libvmm.a libsddf_util_debug.a --end-group

CHECK_FLAGS_BOARD_MD5 := .board_cflags-$(shell echo -- $(CFLAGS) $(BOARD) $(MICROKIT_CONFIG) | shasum | sed 's/ *-//')

$(CHECK_FLAGS_BOARD_MD5):
	-rm -f .board_cflags-*
	touch $@

vmm_x86_64.elf: vmm.o images.o
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

all: loader.img

-include vmm.d

$(IMAGES): libvmm.a libsddf_util_debug.a

$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

vmm.o: $(EXAMPLE_DIR)/vmm_x86_64.c $(CHECK_FLAGS_BOARD_MD5)
	$(CC) $(CFLAGS) -c -o $@ $<

images.o: $(LIBVMM)/tools/package_guest_images.S $(FIRM)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_FIRMWARE_IMAGE_PATH=\"$(FIRM)\" \
					$(ARCH_FLAGS) \
					$(LIBVMM)/tools/package_guest_images.S -o $@

include $(LIBVMM)/vmm.mk
include ${SDDF}/util/util.mk

TIMER_DRIVER := $(SDDF)/drivers/timer/$(TIMER_DRIVER_DIR)
include ${TIMER_DRIVER}/timer_driver.mk

qemu: $(IMAGE_FILE)
	if ! command -v $(QEMU) > /dev/null 2>&1; then echo "Could not find dependency: $(QEMU)"; exit 1; fi
	$(QEMU) $(QEMU_ARCH_ARGS) \
			-serial mon:stdio \
			-m size=2G -device ramfb -vga none \
			-cdrom $(ISO)

clean::
	$(RM) -f *.elf .depend* $
	find . -name \*.[do] |xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f $(IMAGE_FILE) $(REPORT_FILE)
