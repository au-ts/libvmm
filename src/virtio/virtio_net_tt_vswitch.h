/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4cp.h>

#include "virtio_mmio.h"
#include "virtio_net_emul.h"
#include "virtio_net_interface.h"

#define VSWITCH_CONN_CH_1  2

void virtio_net_tt_vswitch_init(void);
