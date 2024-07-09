#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(strip $(LIBVMM)),)
$(error LIBVMM must be specified)
endif
ifeq ($(strip $(MICROKIT_BOARD)),)
$(error MICROKIT_BOARD must be specified)
endif

blk_client_init: $(LIBVMM)/tools/linux/blk/blk_client_init
	cp $^ $@

blk_driver_init: $(LIBVMM)/tools/linux/blk/board/$(MICROKIT_BOARD)/blk_driver_init
	cp $^ $@

clobber::
	rm -f blk_client_init blk_driver_init
