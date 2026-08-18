#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
PYTHON ?= python3
IASL ?= iasl

LIBVMM_DOWNLOADS := https://trustworthy.systems/Downloads/libvmm/images/

LIBVMM_TOOLS := $(LIBVMM)/tools
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
SDDF_INCLUDE := $(SDDF)/include/sddf
UTIL := $(SDDF)/util

TIMER_DRIVER := $(SDDF)/drivers/timer/$(TIMER_DRIVER)
SERIAL_DRIVER := $(SDDF)/drivers/serial/$(UART_DRIVER)
BLK_DRIVER := $(SDDF)/drivers/blk/$(BLK_DRIVER)
ETH_DRIVER := $(SDDF)/drivers/network/$(ETH_DRIVER)
SERIAL_COMPONENTS := $(SDDF)/serial/components
BLK_COMPONENTS := $(SDDF)/blk/components
NET_COMPONENTS := $(SDDF)/network/components

CLIENT_VM = $(WINDOWS_EXAMPLE)/client_vm/$(ARCH)
CLIENT_DTB := client_vm/vm.dtb
METAPROGRAM := $(WINDOWS_EXAMPLE)/meta.py

SDDF_CUSTOM_LIBC := 1

TOOLCHAIN ?= clang
SUPPORTED_BOARDS := x86_64_generic_vtx

SYSTEM_FILE := windows.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt

include ${SDDF}/tools/make/board/common.mk

CLIENT_VM_USERLEVEL_INIT := blk_client_init net_client_init
CLIENT_VM_USERLEVEL_HOME := $(LIBVMM_TOOLS)/linux/blk/blk_integration_tests.sh $(LIBVMM_TOOLS)/linux/blk/blk_bench.sh

ifeq ($(ARCH),x86_64)
	OVMF ?= a80a6dbe981a74c48d237c30885dbfa07f9c6225-OVMF.fd

	ARCH_FLAGS := -target x86_64-unknown-elf -march=x86-64-v2

	QEMU_ARCH_ARGS := -accel kvm -cpu host,+fsgsbase,+pdpe1gb,+xsaves,+vmx,+vme,+enforce \
					  -kernel sel4_32.elf -initrd $(IMAGE_FILE) -vga none
else
$(error Unsupported architecture $(ARCH))
endif

vpath %.c $(SDDF) $(LIBVMM) $(WINDOWS_EXAMPLE)

CFLAGS += \
	  -Wall \
	  -Wno-unused-function \
	  -DBOARD_$(MICROKIT_BOARD) \
	  -DSDDF_VIRTIO_PCI_TRANSPORT_SKIP_BUS_CHECK \
	  -I$(BOARD_DIR)/include \
	  -I$(SDDF)/include \
	  -I$(SDDF)/include/microkit \
	  -I$(LIBVMM)/include \
	  -I$(WINDOWS_EXAMPLE) \
	  -DAPIC_VIRT_LEVEL=2 \
	  $(ARCH_FLAGS)

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld libsddf_util_debug.a --end-group

include $(SDDF)/util/util.mk
include ${SDDF}/drivers/timer/${TIMER_DRIV_DIR}/timer_driver.mk
include ${SDDF}/drivers/serial/${UART_DRIV_DIR}/serial_driver.mk
include $(SERIAL_COMPONENTS)/serial_components.mk
include ${SDDF}/drivers/blk/${BLK_DRIV_DIR}/blk_driver.mk
include $(BLK_COMPONENTS)/blk_components.mk
include ${SDDF}/drivers/network/${NET_DRIV_DIR}/eth_driver.mk
include $(NET_COMPONENTS)/network_components.mk
include $(LIBVMM)/vmm.mk
include $(LIBVMM_TOOLS)/linux/uio/uio.mk
include $(LIBVMM_TOOLS)/linux/blk/blk_init.mk
include $(LIBVMM_TOOLS)/linux/net/net_init.mk

IMAGES := client_vmm.elf timer_driver.elf blk_driver.elf blk_virt.elf \
	network_virt_rx.elf network_virt_tx.elf eth_driver.elf network_copy.elf

CHECK_FLAGS_BOARD_MD5 := .board_cflags-$(shell echo -- $(CFLAGS) $(BOARD) $(MICROKIT_CONFIG) | shasum | sed 's/ *-//')

$(CHECK_FLAGS_BOARD_MD5):
	-rm -f .board_cflags-*
	touch $@

all: ${IMAGE_FILE}

-include vmm.d

$(IMAGES): libsddf_util_debug.a

$(SYSTEM_FILE): $(METAPROGRAM) $(IMAGES)
	PYTHONPATH=${SDDF}/tools/meta:$$PYTHONPATH $(PYTHON) $(METAPROGRAM) --sddf $(SDDF) --board $(MICROKIT_BOARD) --output . --sdf $(SYSTEM_FILE) $(PARTITION_ARG)
	$(OBJCOPY) --update-section .device_resources=timer_driver_device_resources.data timer_driver.elf
	$(OBJCOPY) --update-section .timer_client_config=timer_client_CLIENT_VMM.data client_vmm.elf
	$(OBJCOPY) --update-section .device_resources=blk_driver_device_resources.data blk_driver.elf
	$(OBJCOPY) --update-section .blk_driver_config=blk_driver.data blk_driver.elf
	$(OBJCOPY) --update-section .blk_virt_config=blk_virt.data blk_virt.elf
	$(OBJCOPY) --update-section .blk_client_config=blk_client_CLIENT_VMM.data client_vmm.elf
	$(OBJCOPY) --update-section .vmm_config=vmm_CLIENT_VMM.data client_vmm.elf
	$(OBJCOPY) --update-section .device_resources=eth_driver_device_resources.data eth_driver.elf
	$(OBJCOPY) --update-section .net_driver_config=net_driver.data eth_driver.elf
	$(OBJCOPY) --update-section .net_virt_rx_config=net_virt_rx.data network_virt_rx.elf
	$(OBJCOPY) --update-section .net_virt_tx_config=net_virt_tx.data network_virt_tx.elf
	$(OBJCOPY) --update-section .net_copy_config=net_copy_client0_net_copier.data network_copy.elf network_copy.elf
	$(OBJCOPY) --update-section .net_client_config=net_client_CLIENT_VMM.data client_vmm.elf

$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) \
		--config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

.PHONY: client_vm
client_vm:
	mkdir -p client_vm

${OVMF}:
	curl -L https://trustworthy.systems/Downloads/libvmm/images/${OVMF}.tar.gz -o $@.tar.gz
	mkdir -p ovmf_download_dir
	tar xf $@.tar.gz -C ovmf_download_dir
	cp ovmf_download_dir/${OVMF}/OVMF.fd ${OVMF}


client_vm/vm_dsdt.aml: $(CLIENT_VM)/windows_dsdt.dsl |client_vm
	$(IASL) -p $@ $^

client_vm/vmm.o: $(WINDOWS_EXAMPLE)/client_vmm.c $(CHECK_FLAGS_BOARD_MD5) |client_vm
	$(CC) $(CFLAGS) -c -o $@ $<

client_vm/guest_arch_init.o: $(CLIENT_VM)/guest_arch_init.c $(CHECK_FLAGS_BOARD_MD5) |client_vm
	$(CC) $(CFLAGS) -c -o $@ $<

client_vm/images.o: $(LIBVMM)/tools/package_guest_images.S ${OVMF} $(CHECK_FLAGS_BOARD_MD5) \
	                client_vm/vm_dsdt.aml
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_DSDT_AML_PATH=\"client_vm/vm_dsdt.aml\" \
					-DGUEST_FIRMWARE_PATH=\"${OVMF}\" \
					-target $(TARGET) \
					$(LIBVMM)/tools/package_guest_images.S -o $@

client_vmm.elf: client_vm/vmm.o client_vm/guest_arch_init.o client_vm/images.o libvmm.a |client_vm
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# Stop make from deleting intermediate files
.PRECIOUS: client_vm client_vm/images.o client_vm/vmm.o

# In my experience, SDL have dramatically higher performance and
# does not apply fractional scaling so it give a pixel perfect output.
qemu: $(IMAGE_FILE) ${GUEST_OS_DISK}
	[ "${MICROKIT_BOARD}" = "x86_64_generic_vtx" ]
	taskset -c 0-3 $(QEMU) $(QEMU_ARCH_ARGS) \
			-display sdl \
			-serial mon:stdio \
			-m size=9G \
			-global virtio-mmio.force-legacy=false \
			-drive file=${GUEST_OS_DISK},format=raw,if=none,id=hd \
			$(QEMU_BLK_ARGS) \
			$(QEMU_NET_ARGS) \
			-netdev user,id=netdev0,hostfwd=tcp::1236-:1236,hostfwd=tcp::1237-:1237,hostfwd=udp::1235-:1235 \
			-device bochs-display,addr=0x4.0,xres=1920,yres=1080

clean::
	$(RM) -f *.elf .depend* $
	find . -name \*.[do] -type f |xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f $(IMAGE_FILE) $(REPORT_FILE)
