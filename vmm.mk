#
# Copyright 2024, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Snippet to build libvmm.a, to be included in a full-system Makefile.
#

LIBVMM_DIR :=  $(abspath $(dir $(lastword ${MAKEFILE_LIST})))

AARCH64_FILES := src/arch/aarch64/fault.c \
		 src/arch/aarch64/linux.c \
		 src/arch/aarch64/cpuif.c \
		 src/arch/aarch64/psci.c \
		 src/arch/aarch64/smc.c \
		 src/arch/aarch64/tcb.c \
		 src/arch/aarch64/vcpu.c \
		 src/arch/aarch64/virq.c \
		 src/arch/aarch64/guest.c \
		 src/arch/aarch64/vgic/vgic.c \
		 src/arch/aarch64/vgic/vgic_v2.c \
		 src/arch/aarch64/vgic/vgic_v3.c \
		 src/arch/aarch64/vgic/vgic_v3_cpuif.c \
		 src/virtio/mmio.c \
		 ${VGIC_FILES}

X86_64_FILES = src/arch/x86_64/fault.c \
			   src/arch/x86_64/guest.c \
			   src/arch/x86_64/linux.c \
			   src/arch/x86_64/vcpu.c \
			   src/arch/x86_64/virq.c \
			   src/arch/x86_64/cpuid.c \
			   src/arch/x86_64/msr.c \
			   src/arch/x86_64/acpi.c \
			   src/arch/x86_64/apic.c \
			   src/arch/x86_64/instruction.c \
			   src/arch/x86_64/ioports.c \
			   src/arch/x86_64/hpet.c \
			   src/arch/x86_64/pci.c \
			   src/arch/x86_64/util.c \
			   src/arch/x86_64/cmos.c \
			   src/arch/x86_64/com.c \
			   src/arch/x86_64/tsc.c

# VIRTIO MMIO and PCI depends on sddf
ifeq ($(strip $(SDDF)),)
    $(error libvmm needs the location of the SDDF to build virtIO components)
endif

# we need ${SDDF} for virtIO; we need ${LIBVMM} for all
# the libvmm api interfaces
ifeq ($(findstring ${SDDF}/include, ${CFLAGS}),)
CFLAGS += -I${SDDF}/include -I${SDDF}/include/microkit
endif
ifeq ($(findstring ${LIBVMM_DIR}/include,${CFLAGS}),)
CFLAGS += -I${LIBVMM_DIR}/include
endif

ARCH_INDEP_FILES := src/util/printf.c \
		    src/util/util.c \
		    src/guest.c

VIRTIO_FILES := src/virtio/block.c \
		    src/virtio/console.c \
		    src/virtio/pci.c \
		    src/virtio/net.c \
		    src/virtio/sound.c \
		    src/virtio/virtio.c \
		    src/guest.c

ifeq ($(ARCH),aarch64)
CFILES := ${AARCH64_FILES} ${VIRTIO_FILES}
else ifeq ($(ARCH),x86_64)
CFILES := ${X86_64_FILES}
endif

CFILES += ${ARCH_INDEP_FILES}
OBJECTS := $(subst src,libvmm,${CFILES:.c=.o})

# Enable LLVM UBSAN to trap on detected undefined behaviour
CFLAGS += -fsanitize=undefined -fsanitize-trap=undefined

# Generate dependencies automatically
CFLAGS += -MD -Wall -Werror -Wno-unused-function

# Force rebuid if CFLAGS changes.
# This will pick up (among other things} changes
# to Microkit BOARD and CONFIG.
CHECK_LIBVMM_CFLAGS:=.libvmm_cflags.$(shell echo ${CFLAGS} | shasum | sed 's/ *-$$//')
.libvmm_cflags.%:
	rm -f .libvmm_cflags.*
	echo ${CFLAGS} > $@

# This is ugly, but needed to distinguish  directories in the BUILD area
# from directories in the source area.
.PHONY: directories
directories:
ifeq ($(ARCH),aarch64)
	mkdir -p libvmm/arch/aarch64/vgic/
else ifeq ($(ARCH),x86_64)
	mkdir -p libvmm/arch/x86_64/qemu/
endif
	mkdir -p libvmm/util
	mkdir -p libvmm/virtio

libvmm.a: ${OBJECTS}
	${AR} crv $@ $^

${OBJECTS}: directories ${SDDF}/include
${OBJECTS}: ${CHECK_LIBVMM_CFLAGS} | $(LIBVMM_LIBC_INCLUDE)

libvmm/%.o: src/%.c
	${CC} ${CFLAGS} -c -o $@ $<

-include ${OBJECTS:.o=.d}

clean::
	rm -f ${OBJECTS} ${OBJECTS:.c=.d}

clobber:: clean
	rmdir src/arch/aarch64/vgic
	rmdir src/util
	rmdir src/virtio
