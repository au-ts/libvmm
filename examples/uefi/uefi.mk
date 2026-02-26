#
# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
PYTHON ?= python3

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
LIBVMM_TOOLS := $(LIBVMM)/tools

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
ARCH := ${shell grep 'CONFIG_SEL4_ARCH  ' $(BOARD_DIR)/include/kernel/gen_config.h | cut -d' ' -f4}
SYSTEM_DIR := $(EXAMPLE_DIR)/board/$(MICROKIT_BOARD)
SYSTEM_FILE := uefi.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt

TIMER_DRIVER := $(SDDF)/drivers/timer/$(TIMER_DRIVER)
SERIAL_DRIVER := $(SDDF)/drivers/serial/$(UART_DRIVER)
BLK_DRIVER := $(SDDF)/drivers/blk/$(BLK_DRIVER)
ETH_DRIVER := $(SDDF)/drivers/network/$(ETH_DRIVER)
SERIAL_COMPONENTS := $(SDDF)/serial/components
BLK_COMPONENTS := $(SDDF)/blk/components
NET_COMPONENTS := $(SDDF)/network/components
METAPROGRAM := $(EXAMPLE_DIR)/meta.py

SDDF_CUSTOM_LIBC := 1

vpath %.c $(LIBVMM) $(EXAMPLE_DIR) $(NET_COMPONENTS)

ifeq ($(ARCH),x86_64)
	FIRM ?= $(SYSTEM_DIR)/OVMF.fd
	ARCH_FLAGS := -target x86_64-unknown-elf
	IMAGES = vmm_x86_64.elf timer_driver.elf blk_driver.elf blk_virt.elf serial_driver.elf serial_virt_tx.elf serial_virt_rx.elf \
	         network_virt_rx.elf network_virt_tx.elf eth_driver.elf network_copy.elf eth_driver_virtio.elf
else
$(error Unsupported ARCH given)
endif

ifeq ($(strip $(MICROKIT_BOARD)), x86_64_generic_vtx)
	KERNEL = sel4.elf
	KERNEL32 := sel4_32.elf
	QEMU := qemu-system-x86_64
	QEMU_ARCH_ARGS := -accel kvm -cpu host,+sse,+sse2,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme \
	                  -kernel $(KERNEL32) -initrd $(IMAGE_FILE) \
					  -device virtio-net-pci,netdev=netdev0,addr=0x2.0 \
					  -device virtio-blk-pci,drive=drive0,id=virtblk0,num-queues=1,addr=0x3.0
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

include $(SDDF)/util/util.mk
include $(TIMER_DRIVER)/timer_driver.mk
include $(SERIAL_DRIVER)/serial_driver.mk
include $(SERIAL_COMPONENTS)/serial_components.mk
include ${BLK_DRIVER}/blk_driver.mk
include $(BLK_COMPONENTS)/blk_components.mk
include ${ETH_DRIVER}/eth_driver.mk
include $(NET_COMPONENTS)/network_components.mk
include $(LIBVMM)/vmm.mk

eth_driver.elf: eth_driver_virtio.elf
	cp eth_driver_virtio.elf eth_driver.elf

blk_storage:
	$(LIBVMM_TOOLS)/mkvirtdisk $@ $(BLK_NUM_PART) $(BLK_SIZE) $(BLK_MEM)

$(SYSTEM_FILE): $(METAPROGRAM) $(IMAGES)
	$(PYTHON) $(METAPROGRAM) --sddf $(SDDF) --board $(MICROKIT_BOARD) --output . --sdf $(SYSTEM_FILE) $(PARTITION_ARG)
	$(OBJCOPY) --update-section .device_resources=blk_driver_device_resources.data blk_driver.elf
	$(OBJCOPY) --update-section .blk_driver_config=blk_driver.data blk_driver.elf
	$(OBJCOPY) --update-section .blk_virt_config=blk_virt.data blk_virt.elf
	$(OBJCOPY) --update-section .blk_client_config=blk_client_CLIENT_VMM.data vmm_x86_64.elf
	$(OBJCOPY) --update-section .device_resources=serial_driver_device_resources.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_driver_config=serial_driver_config.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_virt_rx_config=serial_virt_rx.data serial_virt_rx.elf
	$(OBJCOPY) --update-section .serial_virt_tx_config=serial_virt_tx.data serial_virt_tx.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_CLIENT_VMM.data vmm_x86_64.elf
	$(OBJCOPY) --update-section .vmm_config=vmm_CLIENT_VMM.data vmm_x86_64.elf
	$(OBJCOPY) --update-section .device_resources=eth_driver_device_resources.data eth_driver.elf
	$(OBJCOPY) --update-section .net_driver_config=net_driver.data eth_driver.elf
	$(OBJCOPY) --update-section .net_virt_rx_config=net_virt_rx.data network_virt_rx.elf
	$(OBJCOPY) --update-section .net_virt_tx_config=net_virt_tx.data network_virt_tx.elf
	$(OBJCOPY) --update-section .net_copy_config=net_copy_client0_net_copier.data network_copy.elf network_copy.elf
	$(OBJCOPY) --update-section .net_client_config=net_client_CLIENT_VMM.data vmm_x86_64.elf

# 							  -drive file=/home/billn/Downloads/julia/disk.img,format=raw,if=none,id=drive0
# 							  -cdrom /home/billn/Downloads/nixos-graphical-25.11.4270.77ef7a29d276-x86_64-linux.iso
# -drive file=/home/billn/Downloads/windbg_server/Windows10Installed_clean.guest,format=raw,if=none,id=drive0
# 							  -cdrom '/home/dreamliner787-9/Downloads/nixos-graphical-25.11.5198.e576e3c9cf9b-x86_64-linux.iso'
# -cdrom '/home/dreamliner787-9/Downloads/Win10_22H2_EnglishInternational_x64v1.iso'
# /home/billn/Downloads/windbg_client/win10_debugged_com2_no_recovery.guest
# /home/billn/Downloads/Win10_22H2_English_x64v1.iso

# 							  -drive file=/home/billn/Downloads/windbg_server/Windows10Installed_clean.guest,format=raw,if=none,id=drive0 \
# hack for virtio driver installation
# 							  -drive id=disk,file=/home/billn/Downloads/virtio-win-0.1.285.iso,format=raw,if=none \
# 							  -device ide-hd,drive=disk,bus=ide.0 \
# 							  -drive file=/home/billn/Downloads/windbg_client/win10_debugged_com2_no_recovery.guest,format=raw,if=none,id=drive0 \

# 							  -serial tcp:127.0.0.1:4445

qemu: $(IMAGE_FILE) blk_storage
	if ! command -v $(QEMU) > /dev/null 2>&1; then echo "Could not find dependency: $(QEMU)"; exit 1; fi
	$(QEMU) $(QEMU_ARCH_ARGS) -serial mon:stdio \
							  -m size=9G \
							  -d guest_errors \
							  -device ramfb -vga none \
							  -drive file=$(EXAMPLE_DIR)/disk.img,format=raw,if=none,id=drive0 \
							  -netdev user,id=netdev0,hostfwd=tcp::1236-:1236,hostfwd=tcp::1237-:1237,hostfwd=udp::1235-:1235


clean::
	$(RM) -f *.elf .depend* $
	find . -name \*.[do] |xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f $(IMAGE_FILE) $(REPORT_FILE)
