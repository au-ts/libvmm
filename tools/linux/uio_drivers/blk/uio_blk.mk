#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
# This Makefile snippet builds block UIO driver
#

ifeq ($(strip $(VMM_TOOLS)),)
$(error VMM_TOOLS must be specified)
endif
ifeq ($(strip $(SDDF)),)
$(error SDDF must be specified)
endif
ifeq ($(strip $(CC_USERLEVEL)),)
$(error CC_USERLEVEL must be specified)
endif

UIO_BLK_IMAGES := uio_blk_driver

CFLAGS_uio_blk_driver := -I$(SDDF)/include -I$(VMM_TOOLS)/linux/include

CHECK_UIO_BLK_DRIVER_FLAGS_MD5:=.uio_blk_driver_cflags-$(shell echo -- $(CFLAGS_USERLEVEL) $(CFLAGS_uio_blk_driver) | shasum | sed 's/ *-//')

$(CHECK_UIO_BLK_DRIVER_FLAGS_MD5):
	-rm -f .uio_blk_driver_cflags-*
	touch $@


# This shouldn't have a -c but for some reason it only works with one
uio_blk_driver: uio_blk_driver.o
	$(CC_USERLEVEL) $(CFLAGS_USERLEVEL) $(CFLAGS_uio_blk_driver) -o $@ -c $<

uio_blk_driver.o: $(CHECK_UIO_BLK_DRIVER_FLAGS_MD5)
uio_blk_driver.o: $(VMM_TOOLS)/linux/uio_drivers/blk/blk.c
	$(CC_USERLEVEL) $(CFLAGS_USERLEVEL) $(CFLAGS_uio_blk_driver) -o $@ -c $<

clean::
	rm -f uio_blk_driver.[od] .uio_blk_driver_cflags-*

clobber::
	rm -f $(UIO_BLK_IMAGES)

-include uio_blk_driver.d
