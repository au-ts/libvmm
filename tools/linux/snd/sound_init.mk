#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Generates asound.conf, snd_driver_init
#

ifeq ($(strip $(MICROKIT_BOARD)),)
$(error MICROKIT_BOARD must be specified)
endif

LINUX_BLK_DIR := $(abs $(dir $(last ${MAKEFILES_LIST})))
LIBVMM ?= $(realpath ${LINUX_BLK_DIR}/../../../)

asound.conf: $(LIBVMM)/tools/linux/snd/board/$(MICROKIT_BOARD)/asound.conf
	cp $^ $@

snd_driver_init: $(LIBVMM)/tools/linux/snd/snd_driver_init
	cp $^ $@

clobber::
	rm -f asound.conf snd_driver_init
