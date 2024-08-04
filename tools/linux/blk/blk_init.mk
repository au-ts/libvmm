#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(strip $(MICROKIT_BOARD)),)
$(error MICROKIT_BOARD must be specified)
endif

LINUX_BLK_DIR := $(abs $(dir $(last ${MAKEFILES_LIST})))
LIBVMM ?= $(realpath ${LINUX_BLK_DIR}/../../../)

blk_client_init: $(LIBVMM)/tools/linux/blk/blk_client_init
	cp $^ $@

blk_driver_init: $(LIBVMM)/tools/linux/blk/board/$(MICROKIT_BOARD)/blk_driver_init
	cp $^ $@

clobber::
	rm -f blk_client_init blk_driver_init
