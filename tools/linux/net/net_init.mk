#
# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Generates net_client_init or net_driver_init scripts
#

LINUX_NET_DIR := $(abs $(dir $(last ${MAKEFILES_LIST})))
LIBVMM ?= $(realpath ${LINUX_BLK_DIR}/../../../)

net_client_init: $(LIBVMM)/tools/linux/net/net_client_init
	cp $^ $@

net_driver_init: $(LIBVMM)/tools/linux/net/net_driver_init
	cp $^ $@

clobber::
	rm -f net_client_init net_driver_init
