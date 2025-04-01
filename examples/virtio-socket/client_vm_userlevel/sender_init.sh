#!/bin/sh

# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause

CID=3
# Sleep for a bit to make sure the receiver is ready
sleep 5 && /root/vsock_send.elf "$CID"