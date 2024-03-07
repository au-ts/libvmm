#
# Copyright 2024, UNSW (ABN 57 195 873 179)
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Snippet to build libvmm.a, to be included in a full-system Makefile.
# Needs the variable LionsOS to point to the top of the LionsOS tree.
#
V3_BOARDS := BOARD_imx8mm_evk
ifeq ($(filter ${MICROKIT_BOARD},${V3_BOARDS}),)
	VGIC := GIC_V2
	VGIC_FILES := src/arch/aarch64/vgic/vgic_v2.c
else
	VGIC := GIC_V3
	VGIC_FILES := src/arch/aarch64/vgic/vgic_v3.c
endif

AARCH64_FILES := src/arch/aarch64/fault.c \
		 src/arch/aarch64/linux.c \
		 src/arch/aarch64/linux.c \
		 src/arch/aarch64/psci.c \
		 src/arch/aarch64/smc.c \
		 src/arch/aarch64/tcb.c \
		 src/arch/aarch64/vcpu.c \
		 src/arch/aarch64/virq.c \
		 src/arch/aarch64/vgic/vgic.c \
		 ${VGIC_FILES}

# VIRTIO MMIO depends on sddf
ifeq ($(strip $(SDDF)),)
  ifneq (,$(wildcard ${LionsOS}/sddf))
    SDDF := ${LionsOS}/sddf
  else
    $(error libvmm needs the location of the SDDF to build virtIO components)
  endif
endif

CFLAGS += -I${SDDF}/include

ARCH_INDEP_FILES := src/util/printf.c \
		    src/util/util.c \
		    src/virtio/console.c \
		    src/virtio/mmio.c \
		    src/virtio/virtio.c \
		    src/guest.c

CFILES := ${AARCH64_FILES} ${ARCH_INDEP_FILES}

# Generate dependencies automatically
CFLAGS += -MD

src/arch/aarch64/vgic/stamp:
	mkdir -p src/arch/aarch64/vgic/
	mkdir -p src/util
	mkdir -p src/virtio
	touch $@

libvmm.a: ${CFILES:.c=.o}
	ar rv $@ ${CFILES:.c=.o}

${CFILES:.c=.o}: src/arch/aarch64/vgic/stamp

-include ${CFILES:.c=.d}
