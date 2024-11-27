#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
# This Makefile snippet builds filesystem UIO driver
#

ifeq ($(strip $(LIBVMM_TOOLS)),)
$(error LIBVMM_TOOLS must be specified)
endif
ifeq ($(strip $(SDDF)),)
$(error SDDF must be specified)
endif
ifeq ($(strip $(CC_USERLEVEL)),)
$(error CC_USERLEVEL must be specified)
endif

UIO_FS_IMAGES := uio_fs_driver

CFLAGS_uio_fs_driver := -I$(SDDF)/include -I$(LIBVMM_TOOLS)/linux/include

CHECK_UIO_FS_DRIVER_FLAGS_MD5:=.uio_fs_driver_cflags-$(shell echo -- $(CFLAGS_USERLEVEL) $(CFLAGS_uio_fs_driver) | shasum | sed 's/ *-//')

$(CHECK_UIO_FS_DRIVER_FLAGS_MD5):
	-rm -f .uio_fs_driver_cflags-*
	touch $@

uio_fs_driver: uio_fs_driver.o
	$(CC_USERLEVEL) -static $(CFLAGS_USERLEVEL) $(CFLAGS_uio_fs_driver) $^ -o $@

uio_fs_driver.o: $(CHECK_UIO_FS_DRIVER_FLAGS_MD5)
uio_fs_driver.o: $(LIBVMM_TOOLS)/linux/uio_drivers/fs/main.c
	$(CC_USERLEVEL) $(CFLAGS_USERLEVEL) $(CFLAGS_uio_fs_driver) -o $@ -c $<

clean::
	rm -f uio_fs_driver.[od] .uio_fs_driver_cflags-*

clobber::
	rm -f $(UIO_FS_IMAGES)

-include uio_fs_driver.d
