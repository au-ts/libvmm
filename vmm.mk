#
# Copyright 2024, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Snippet to build libvmm.a, to be included in a full-system Makefile.
#

LIBVMM_DIR :=  $(abspath $(dir $(lastword ${MAKEFILE_LIST})))

GIC_V3_BOARDS := imx8mm_evk maaxboard
ifeq ($(filter ${MICROKIT_BOARD},${GIC_V3_BOARDS}),)
	VGIC := GIC_V2
	VGIC_FILES := src/arch/aarch64/vgic/vgic_v2.c
else
	VGIC := GIC_V3
	VGIC_FILES := src/arch/aarch64/vgic/vgic_v3.c
endif

AARCH64_FILES := src/arch/aarch64/fault.c \
		 src/arch/aarch64/linux.c \
		 src/arch/aarch64/psci.c \
		 src/arch/aarch64/smc.c \
		 src/arch/aarch64/tcb.c \
		 src/arch/aarch64/vcpu.c \
		 src/arch/aarch64/virq.c \
		 src/arch/aarch64/vgic/vgic.c \
		 ${VGIC_FILES}

RISCV_FILES := src/arch/riscv/fault.c \
		 src/arch/riscv/linux.c \
		 src/arch/riscv/plic.c \
		 src/arch/riscv/sbi.c \
		 src/arch/riscv/tcb.c \
		 src/arch/riscv/vcpu.c \
		 src/arch/riscv/virq.c

# VIRTIO MMIO depends on sddf
ifeq ($(strip $(SDDF)),)
    $(error libvmm needs the location of the SDDF to build virtIO components)
endif

ifeq ($(strip $(ARCH)),)
    $(error libvmm needs to know what architecture to target set to ARCH)
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
		    src/virtio/block.c \
		    src/virtio/console.c \
		    src/virtio/mmio.c \
		    src/virtio/net.c \
		    src/virtio/sound.c \
		    src/guest.c \
		    src/fault.c \
		    src/linux.c \
		    src/dtb.c \
		    src/virq.c

CFILES := ${ARCH_INDEP_FILES}

ifeq ($(ARCH),aarch64)
	CFILES += $(AARCH64_FILES)
else ifeq ($(ARCH),riscv64)
	CFILES += $(RISCV_FILES)
else
	$(error Unsupported ARCH given)
endif

OBJECTS := $(subst src,libvmm,${CFILES:.c=.o})

# Generate dependencies automatically
CFLAGS += -MD

# Force rebuid if CFLAGS changes.
# This will pick up (among other things} changes
# to Microkit BOARD and CONFIG.
CHECK_LIBVMM_CFLAGS:=.libvmm_cflags.$(shell echo ${CFLAGS} | shasum | sed 's/ *-$$//')
.libvmm_cflags.%:
	rm -f .libvmm_cflags.*
	echo ${CFLAGS} > $@

# This is ugly, but needed to distinguish  directories in the BUILD area
# from directories in the source area.
libvmm/arch/aarch64/vgic:
	mkdir -p libvmm/arch/aarch64/vgic/
	mkdir -p libvmm/util
	mkdir -p libvmm/virtio

libvmm/arch/riscv:
	mkdir -p libvmm/arch/riscv
	mkdir -p libvmm/util
	mkdir -p libvmm/virtio

libvmm.a: ${OBJECTS}
	${AR} crv $@ $^

${OBJECTS}: ${SDDF}/include
${OBJECTS}: ${CHECK_LIBVMM_CFLAGS} |libvmm/arch/aarch64/vgic libvmm/arch/riscv

libvmm/%.o: src/%.c
	${CC} ${CFLAGS} -c -o $@ $<

-include ${OBJECTS:.o=.d}

clean::
	rm -f ${OBJECTS} ${OBJECTS:.c=.d}

clobber:: clean
	rmdir src/arch/riscv
	rmdir src/arch/aarch64/vgic
	rmdir src/util
	rmdir src/virtio
