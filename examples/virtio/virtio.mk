QEMU := qemu-system-aarch64

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit
SDDF_INCLUDE := $(SDDF)/include/sddf
UTIL := $(SDDF)/util

UART_DRIVER := $(SDDF)/drivers/serial/$(UART_DRIVER)
SERIAL_COMPONENTS := $(SDDF)/serial/components
BLK_COMPONENTS := $(SDDF)/blk/components

SERIAL_NUM_CLIENTS := -DSERIAL_NUM_CLIENTS=3
BLK_NUM_CLIENTS := -DBLK_NUM_CLIENTS=2

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
SYSTEM_DIR := $(VIRTIO_EXAMPLE)/board/$(MICROKIT_BOARD)
SYSTEM_FILE := $(SYSTEM_DIR)/virtio.system
IMAGE_FILE := loader.img
REPORT_FILE := report.txt

vpath %.c $(SDDF) $(LIBVMM) $(VIRTIO_EXAMPLE) $(NETWORK_COMPONENTS)

include $(SDDF)/util/util.mk
include $(UART_DRIVER)/uart_driver.mk
include $(SERIAL_COMPONENTS)/serial_components.mk
include $(BLK_COMPONENTS)/blk_components.mk
include $(LIBVMM)/vmm.mk

IMAGES := client_vmm.elf blk_driver_vmm.elf \
	$(SERIAL_IMAGES) $(BLK_IMAGES) uart_driver.elf

CFLAGS := \
	  -mstrict-align \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -DBOARD_$(MICROKIT_BOARD) \
	  -DCONFIG_$(MICROKIT_CONFIG) \
	  -I$(BOARD_DIR)/include \
	  -I$(SDDF)/include \
	  -I$(LIBVMM)/include \
	  -I$(VIRTIO_EXAMPLE)/include \
	  -I$(SDDF)/$(LWIPDIR)/include \
	  -I$(SDDF)/$(LWIPDIR)/include/ipv4 \
	  -MD \
	  -MP \
	  -target $(TARGET)

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld libsddf_util_debug.a libvmm.a --end-group

CHECK_FLAGS_BOARD_MD5:=.board_cflags-$(shell echo -- $(CFLAGS) $(BOARD) $(MICROKIT_CONFIG) | shasum | sed 's/ *-//')

$(CHECK_FLAGS_BOARD_MD5):
	-rm -f .board_cflags-*
	touch $@

%_vmm.elf: %_vm/vmm.o %_vm/images.o
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

all: loader.img

-include vmm.d

$(IMAGES): libsddf_util_debug.a libvmm.a

$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

%_vm:
	mkdir -p $@

# TODO: sort out userlevel scripts & build userlevel processes
client_vm/rootfs.cpio.gz: client_vm $(SYSTEM_DIR)/client_vm/rootfs.cpio.gz
	$(LIBVMM)/tools/packrootfs $(SYSTEM_DIR)/client_vm/rootfs.cpio.gz client_vm/rootfs -o $@ \
		--startup $(CLIENT_VM_USERLEVEL) \
		--home $(CLIENT_VM_USERLEVEL)

# %_vm/rootfs.cpio.gz: %_vm $(SYSTEM_DIR)/%_vm/rootfs.cpio.gz
# 	$(LIBVMM)/tools/packrootfs $(SYSTEM_DIR)/$</rootfs.cpio.gz $</rootfs -o $@

%_vm/vm.dts: $(SYSTEM_DIR)/%_vm/dts/linux.dts $(shell find $(SYSTEM_DIR)/%_vm -name "*.dts" -not -name "linux.dts")
	$(LIBVMM)/tools/dtscat $^ > $@

%_vm/vm.dtb: %_vm/vm.dts %_vm
	$(DTC) -q -I dts -O dtb $< > $@

%_vm/vmm.o: $(VIRTIO_EXAMPLE)/%_vmm.c $(CHECK_FLAGS_BOARD_MD5) %_vm
	$(CC) $(CFLAGS) -c -o $@ $<

%_vm/images.o: %_vm $(LIBVMM)/tools/package_guest_images.S $(SYSTEM_DIR)/%_vm/linux %_vm/vm.dtb %_vm/rootfs.cpio.gz
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(SYSTEM_DIR)/$</linux\" \
					-DGUEST_DTB_IMAGE_PATH=\"$</vm.dtb\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$</rootfs.cpio.gz\" \
					-target $(TARGET) \
					$(LIBVMM)/tools/package_guest_images.S -o $@

# Stop make from deleting intermediate files
client_vm_files:: client_vm client_vm/vm.dts client_vm/vm.dtb client_vm/rootfs.cpio.gz client_vm/images.o client_vm/vmm.o
blk_driver_vm_files:: blk_driver_vm blk_driver_vm/vm.dts blk_driver_vm/vm.dtb blk_driver_vm/rootfs.cpio.gz blk_driver_vm/images.o blk_driver_vm/vmm.o

qemu: $(IMAGE_FILE)
	$(QEMU) -machine virt,virtualization=on \
			-cpu cortex-a53 \
			-serial mon:stdio \
			-device loader,file=$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
			-m size=2G \
			-nographic \
			-device virtio-net-device,netdev=netdev0 \
			-netdev user,id=netdev0,hostfwd=tcp::1236-:1236,hostfwd=udp::1235-:1235 \
			-global virtio-mmio.force-legacy=false \
			-d guest_errors

clean::
	$(RM) -f *.elf .depend* $
	find . -name \*.[do] |xargs --no-run-if-empty rm

clobber:: clean
	rm -f *.a
	rm -f $(IMAGE_FILE) $(REPORT_FILE)
