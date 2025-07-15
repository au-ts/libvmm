#
# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
# Generates net_client_init script
#

LINUX_NET_DIR := $(abs $(dir $(last ${MAKEFILES_LIST})))
LIBVMM ?= $(realpath ${LINUX_BLK_DIR}/../../../)

net_client_init: $(LIBVMM)/tools/linux/net/net_client_init
	cp $^ $@

clobber::
	rm -f net_client_init
