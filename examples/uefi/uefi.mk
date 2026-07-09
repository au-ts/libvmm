#
# Copyright 2024, UNSW
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

IMAGES := vmm.elf

ifeq ($(ARCH),x86_64)
	LINUX ?= be4206493bcc7234a8713319b7c6280fa04f9c5a-bzImage
	INITRD ?= d887a642236a92610a9537ab9f4a4aa1a966ad3a-rootfs.cpio.gz

# -march=x86-64-v2 so that we get extra optimisations without AVX, since seL4 doesnt't enable it by default
	ARCH_FLAGS := -target x86_64-unknown-elf -march=x86-64-v2
	IMAGES += timer_driver.elf

	QEMU := qemu-system-x86_64
	QEMU_ARCH_ARGS := -accel kvm -cpu host,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme -kernel sel4_32.elf -initrd $(IMAGE_FILE)
else
$(error Unsupported ARCH given)
endif

CFLAGS := \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
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

vmm.elf: vmm.o images.o
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

all: loader.img

-include vmm.d

$(IMAGES): libvmm.a libsddf_util_debug.a

$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

${LINUX}:
	curl -L https://trustworthy.systems/Downloads/libvmm/images/${LINUX}.tar.gz -o $@.tar.gz
	mkdir -p linux_download_dir
	tar -xf $@.tar.gz -C linux_download_dir
	cp linux_download_dir/${LINUX}/bzImage ${LINUX}

${INITRD}:
	curl -L https://trustworthy.systems/Downloads/libvmm/images/${INITRD}.tar.gz -o $@.tar.gz
	mkdir -p initrd_download_dir
	tar xf $@.tar.gz -C initrd_download_dir
	cp initrd_download_dir/${INITRD}/rootfs.cpio.gz ${INITRD}

vm.dts: $(SYSTEM_DIR)/linux.dts $(SYSTEM_DIR)/overlay.dts
	$(LIBVMM)/tools/dtscat $^ > $@

vm.dtb: vm.dts
	$(DTC) -q -I dts -O dtb $< > $@

vm_dsdt.aml: $(SYSTEM_DIR)/uefi_dsdt.dsl
	$(IASL) -p $@ $^

vmm.o: $(EXAMPLE_DIR)/vmm.c $(CHECK_FLAGS_BOARD_MD5)
	$(CC) $(CFLAGS) -c -o $@ $<

images.o: $(LIBVMM)/tools/package_guest_images.S $(LINUX) $(INITRD) vm_dsdt.aml
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"${LINUX}\" \
					-DGUEST_INITRD_IMAGE_PATH=\"${INITRD}\" \
					-DGUEST_DSDT_AML_PATH=\"vm_dsdt.aml\" \
					$(ARCH_FLAGS) \
					$(LIBVMM)/tools/package_guest_images.S -o $@

include $(LIBVMM)/vmm.mk
include ${SDDF}/util/util.mk

TIMER_DRIVER_DIR := tsc_hpet
TIMER_DRIVER := $(SDDF)/drivers/timer/$(TIMER_DRIVER_DIR)
include ${TIMER_DRIVER}/timer_driver.mk

qemu: $(IMAGE_FILE)
	if ! command -v $(QEMU) > /dev/null 2>&1; then echo "Could not find dependency: $(QEMU)"; exit 1; fi
	$(QEMU) $(QEMU_ARCH_ARGS) \
			-serial mon:stdio \
			-m size=2G \
			-nographic

clean::
	$(RM) -f *.elf .depend* $
	find . -name \*.[do] |xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f $(IMAGE_FILE) $(REPORT_FILE)
