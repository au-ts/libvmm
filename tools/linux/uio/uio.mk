#
# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
# This Makefile snippet builds block UIO driver
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

LIBUIO_IMAGES := libuio.a

CFLAGS_libuio := -I$(SDDF)/include -I$(LIBVMM_TOOLS)/linux/include

CHECK_LIBUIO_FLAGS_MD5:=.libuio_cflags-$(shell echo -- $(CFLAGS_USERLEVEL) $(CFLAGS_libuio) | shasum | sed 's/ *-//')

$(CHECK_LIBUIO_FLAGS_MD5):
	-rm -f .libuio_cflags-*
	touch $@


libuio.a: libuio.o
	ar rv $@ $^

libuio.o: $(CHECK_LIBUIO_FLAGS_MD5)
libuio.o: $(LIBVMM_TOOLS)/linux/uio/libuio.c
	$(CC_USERLEVEL) $(CFLAGS_USERLEVEL) $(CFLAGS_libuio) -o $@ -c $<

clean::
	rm -f libuio.[od] .libuio_cflags-*

clobber::
	rm -f $(UIO_BLK_IMAGES)

-include libuio.d
