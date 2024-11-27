#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Generates fs_driver_init scripts
#

ifeq ($(strip $(MICROKIT_BOARD)),)
$(error MICROKIT_BOARD must be specified)
endif

fs_driver_init: $(LIBVMM_DIR)/tools/linux/fs/board/$(MICROKIT_BOARD)/fs_driver_init
	cp $^ $@

clobber::
	rm -f fs_driver_init
