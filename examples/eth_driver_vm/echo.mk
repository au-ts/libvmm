#
# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
# This Makefile is copied into the build directory
# and operated on from there.
#

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif

ifeq ($(strip $(SDDF)),)
$(error SDDF must be specified)
endif

LIBVMM_DOWNLOADS := https://trustworthy.systems/Downloads/libvmm/images/

BUILD_DIR ?= build
MICROKIT_CONFIG ?= debug
IMAGE_FILE = loader.img
REPORT_FILE = report.txt

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
ARCH := ${shell grep 'CONFIG_SEL4_ARCH  ' $(BOARD_DIR)/include/kernel/gen_config.h | cut -d' ' -f4}
SDDF_CUSTOM_LIBC := 1

ifeq ($(strip $(TOOLCHAIN)),)
	TOOLCHAIN := clang
endif

ifeq ($(strip $(TOOLCHAIN)), clang)
	CC := clang
	LD := ld.lld
	AR := llvm-ar
	RANLIB := llvm-ranlib
	OBJCOPY := llvm-objcopy
else
	CC := $(TOOLCHAIN)-gcc
	LD := $(TOOLCHAIN)-ld
	AS := $(TOOLCHAIN)-as
	AR := $(TOOLCHAIN)-ar
	RANLIB := $(TOOLCHAIN)-ranlib
	OBJCOPY := $(TOOLCHAIN)-objcopy
endif

DTC := dtc
PYTHON ?= python3

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

ifeq ($(strip $(MICROKIT_BOARD)), odroidc4)
	LINUX := dbc9444d8184b6203a62b9925df81fdf8ccb8a0e-linux
	INITRD := 3b2853f5c6e81c7b9c910d2092cf1a65c20cbfb4-rootfs.cpio.gz
	SERIAL_DRIV_DIR := meson
	TIMER_DRV_DIR := meson
	CPU := cortex-a55
else
$(error Unsupported MICROKIT_BOARD given)
endif


TOP := ${SDDF}/examples/echo_server
METAPROGRAM := $(EXAMPLE_DIR)/meta.py

ECHO_SERVER := ${SDDF}/examples/echo_server
LWIPDIR := network/ipstacks/lwip/src
BENCHMARK := $(SDDF)/benchmark
UTIL := $(SDDF)/util
SERIAL_COMPONENTS := $(SDDF)/serial/components
SERIAL_DRIVER := $(SDDF)/drivers/serial/$(SERIAL_DRIV_DIR)
TIMER_DRIVER := $(SDDF)/drivers/timer/$(TIMER_DRV_DIR)
NETWORK_COMPONENTS := $(SDDF)/network/components
SYSTEM_FILE := echo_server.system
DTS := $(SDDF)/dts/$(MICROKIT_BOARD).dts
DTB := $(MICROKIT_BOARD).dtb

vpath %.c ${SDDF} ${ECHO_SERVER} $(LIBVMM)

IMAGES := eth_driver_vmm.elf echo0.elf echo1.elf benchmark.elf idle.elf network_virt_rx.elf\
	  network_virt_tx.elf network_copy.elf network_copy0.elf network_copy1.elf timer_driver.elf\
	  serial_driver.elf serial_virt_tx.elf serial_virt_rx.elf

ifeq ($(ARCH),aarch64)
	CFLAGS_ARCH := -mcpu=$(CPU)
	TARGET := aarch64-none-elf
else ifeq ($(ARCH),riscv64)
	CFLAGS_ARCH := -march=rv64imafdc
	TARGET := riscv64-none-elf
else
$(error Unsupported ARCH given)
endif

ifeq ($(strip $(TOOLCHAIN)), clang)
	CFLAGS_ARCH += -target $(TARGET)
endif

CFLAGS := $(CFLAGS_ARCH) \
	  -nostdlib \
	  -ffreestanding \
	  -g3 -O3 -Wall \
	  -Wno-unused-function \
	  -DMICROKIT_CONFIG_$(MICROKIT_CONFIG) \
	  -I$(BOARD_DIR)/include \
	  -I$(SDDF)/include/microkit \
	  -I$(SDDF)/include \
	  -I$(ECHO_INCLUDE)/lwip \
	  -I$(SDDF)/$(LWIPDIR)/include \
	  -I$(SDDF)/$(LWIPDIR)/include/ipv4 \
	  -I$(ECHO_SERVER)/include \
	  -I$(LIBVMM_TOOLS)/linux/include \
	  -I$(EXAMPLE_DIR)/include \
	  -MD \
	  -MP

CFLAGS += -Wno-tautological-constant-out-of-range-compare # Suppress warning from lwIP

LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld libsddf_util_debug.a --end-group

CHECK_FLAGS_BOARD_MD5 := .board_cflags-$(shell echo -- ${CFLAGS} ${MICROKIT_SDK} ${MICROKIT_BOARD} ${MICROKIT_CONFIG} | shasum | sed 's/ *-//')

${CHECK_FLAGS_BOARD_MD5}:
	-rm -f .board_cflags-*
	touch $@

ECHO_OBJS := echo.o utilization_socket.o \
	     udp_echo_socket.o tcp_echo_socket.o

DEPS := $(ECHO_OBJS:.o=.d)

all: loader.img

echo0.elf echo1.elf: $(ECHO_OBJS) libsddf_util.a lib_sddf_lwip_echo.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

network_copy0.elf network_copy1.elf: network_copy.elf
	cp $< $@

NET_DRIVER_VM_USERLEVEL := uio_net_driver
NET_DRIVER_VM_USERLEVEL_INIT := net_driver_init

CFLAGS_USERLEVEL := \
	-g3 \
	-fno-vectorize -fno-slp-vectorize \
	-mgeneral-regs-only \
	-fno-builtin-memcpy -fno-builtin-memmove -fno-builtin-memset \
	-mstrict-align \
	-Wno-unused-command-line-argument \
	-Wall -Wno-unused-function \
	-D_GNU_SOURCE \
	-target aarch64-linux-gnu \
	-I$(BOARD_DIR)/include \
	-I$(SDDF)/include/microkit \
	-I$(SDDF)/include

NET_DRIVER_VM_BOARD_DIR := ${EXAMPLE_DIR}/board/${MICROKIT_BOARD}
NET_DRIVER_VM_DTS := $(NET_DRIVER_VM_BOARD_DIR)/linux.dts
NET_DRIVER_VM_DTS_OVERLAY := $(NET_DRIVER_VM_BOARD_DIR)/overlay.dts
NET_DRIVER_VM_DTS_FINAL := linux_overlayed.dts
NET_DRIVER_VM_DTB := linux_overlayed.dtb
NET_DRIVER_VM_INITRD_FINAL := ethernet_driver_vm_rootfs.cpio.gz

${LINUX}:
	curl -L ${LIBVMM_DOWNLOADS}/$(LINUX).tar.gz -o $(LINUX).tar.gz
	mkdir -p linux_download_dir
	tar -xf $@.tar.gz -C linux_download_dir
	cp linux_download_dir/${LINUX}/linux ${LINUX}

${INITRD}:
	curl -L ${LIBVMM_DOWNLOADS}/$(INITRD).tar.gz -o $(INITRD).tar.gz
	mkdir -p initrd_download_dir
	tar xf $@.tar.gz -C initrd_download_dir
	cp initrd_download_dir/${INITRD}/rootfs.cpio.gz ${INITRD}

${NET_DRIVER_VM_DTS_FINAL}: $(NET_DRIVER_VM_DTS) $(NET_DRIVER_VM_DTS_OVERLAY)
	$(LIBVMM)/tools/dtscat $^ > $@

$(NET_DRIVER_VM_DTB): $(NET_DRIVER_VM_DTS_FINAL)
	$(DTC) -q -I dts -O dtb $< > $@

${NET_DRIVER_VM_INITRD_FINAL}: ${INITRD} $(NET_DRIVER_VM_USERLEVEL) $(NET_DRIVER_VM_USERLEVEL_INIT)
	$(LIBVMM)/tools/packrootfs ${INITRD} rootfs_staging -o $@ \
		--startup $(NET_DRIVER_VM_USERLEVEL_INIT) \
		--home $(NET_DRIVER_VM_USERLEVEL)

net_driver_vm_guest_images.o: $(LIBVMM)/tools/package_guest_images.S \
			$(LINUX) $(NET_DRIVER_VM_INITRD_FINAL) $(NET_DRIVER_VM_DTB)
	$(CC) -c -g3 -x assembler-with-cpp \
					-DGUEST_KERNEL_IMAGE_PATH=\"$(LINUX)\" \
					-DGUEST_DTB_IMAGE_PATH=\"$(NET_DRIVER_VM_DTB)\" \
					-DGUEST_INITRD_IMAGE_PATH=\"$(NET_DRIVER_VM_INITRD_FINAL)\" \
					-target $(TARGET) \
					$(LIBVMM_DIR)/tools/package_guest_images.S -o $@

eth_driver_vmm.o: ${EXAMPLE_DIR}/ethernet_driver_vmm.c
	$(CC) $(CFLAGS) -c $^ -o $@

eth_driver_vmm.elf: eth_driver_vmm.o net_driver_vm_guest_images.o libvmm.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# Need to build libsddf_util_debug.a because it's included in LIBS
# for the unimplemented libc dependencies
${IMAGES}: libsddf_util_debug.a ${CHECK_FLAGS_BOARD_MD5}

$(DTB): $(DTS)
	dtc -q -I dts -O dtb $(DTS) > $(DTB)

$(SYSTEM_FILE): $(METAPROGRAM) $(IMAGES) $(DTB) $(NET_DRIVER_VM_DTB)
	$(PYTHON) $(METAPROGRAM) --sddf $(SDDF) --board $(MICROKIT_BOARD) --dtb_native $(DTB) --dtb_guest $(NET_DRIVER_VM_DTB) --output . --sdf $(SYSTEM_FILE)
	$(OBJCOPY) --update-section .device_resources=serial_driver_device_resources.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_driver_config=serial_driver_config.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_virt_tx_config=serial_virt_tx.data serial_virt_tx.elf
	$(OBJCOPY) --update-section .serial_virt_rx_config=serial_virt_rx.data serial_virt_rx.elf

	$(OBJCOPY) --update-section .device_resources=net_driver_vm_device_resources.data eth_driver_vmm.elf
	$(OBJCOPY) --update-section .net_driver_config=net_driver.data eth_driver_vmm.elf
	$(OBJCOPY) --update-section .vmm_config=vmm_net_driver_vm.data eth_driver_vmm.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_net_driver_vm.data eth_driver_vmm.elf

	$(OBJCOPY) --update-section .net_virt_rx_config=net_virt_rx.data eth_driver_vmm.elf
	$(OBJCOPY) --update-section .net_virt_tx_config=net_virt_tx.data eth_driver_vmm.elf

	$(OBJCOPY) --update-section .net_virt_rx_config=net_virt_rx.data network_virt_rx.elf
	$(OBJCOPY) --update-section .net_virt_tx_config=net_virt_tx.data network_virt_tx.elf
	$(OBJCOPY) --update-section .net_copy_config=net_copy_client0_net_copier.data network_copy.elf network_copy0.elf
	$(OBJCOPY) --update-section .net_copy_config=net_copy_client1_net_copier.data network_copy.elf network_copy1.elf
	$(OBJCOPY) --update-section .device_resources=timer_driver_device_resources.data timer_driver.elf
	$(OBJCOPY) --update-section .timer_client_config=timer_client_client0.data echo0.elf
	$(OBJCOPY) --update-section .net_client_config=net_client_client0.data echo0.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_client0.data echo0.elf
	$(OBJCOPY) --update-section .timer_client_config=timer_client_client1.data echo1.elf
	$(OBJCOPY) --update-section .net_client_config=net_client_client1.data echo1.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_client1.data echo1.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_bench.data benchmark.elf
	$(OBJCOPY) --update-section .benchmark_config=benchmark_config.data benchmark.elf
	$(OBJCOPY) --update-section .benchmark_client_config=benchmark_client_config.data echo0.elf
	$(OBJCOPY) --update-section .benchmark_config=benchmark_idle_config.data idle.elf
	$(OBJCOPY) --update-section .lib_sddf_lwip_config=lib_sddf_lwip_config_client0.data echo0.elf
	$(OBJCOPY) --update-section .lib_sddf_lwip_config=lib_sddf_lwip_config_client1.data echo1.elf

${IMAGE_FILE} $(REPORT_FILE): $(IMAGES) $(SYSTEM_FILE)
	$(MICROKIT_TOOL) $(SYSTEM_FILE) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)


include ${SDDF}/util/util.mk
include ${SDDF}/network/components/network_components.mk
include ${SDDF}/network/lib_sddf_lwip/lib_sddf_lwip.mk
include ${BENCHMARK}/benchmark.mk
include ${TIMER_DRIVER}/timer_driver.mk
include ${SERIAL_DRIVER}/serial_driver.mk
include ${SERIAL_COMPONENTS}/serial_components.mk
include $(LIBVMM)/vmm.mk
include $(LIBVMM_TOOLS)/linux/uio_drivers/net/uio_net.mk
include $(LIBVMM_TOOLS)/linux/net/net_init.mk

qemu: $(IMAGE_FILE)
	$(QEMU) $(QEMU_ARCH_ARGS) \
			-m size=2G \
			-nographic \
			-device virtio-net-device,netdev=netdev0 \
			-netdev user,id=netdev0,hostfwd=tcp::1236-:1236,hostfwd=tcp::1237-:1237,hostfwd=udp::1235-:1235 \
			-global virtio-mmio.force-legacy=false \
			-d guest_errors

clean::
	${RM} -f *.elf .depend* $
	find . -name \*.[do] |xargs --no-run-if-empty rm

clobber:: clean
	rm -f ${IMAGE_FILE} ${REPORT_FILE}

-include $(DEPS)
