/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4cp.h>
#include "util/util.h"
#include <stddef.h>
#include "uio.h"

#define VMM_CH 0

uint8_t *uio_map0;

void init() {
    // Initialising rectangle PD
    sel4cp_dbg_puts("SEND RECTANGLE|INFO: starting!\n");
}

void fbwrite() {
    // Draw triangle
    for (int y = 100; y < 300; y++) {
        for (int x = 100; x < 300; x++) {

            uint64_t location = (x * (BPP/8)) + (y * LINE_LEN);

            *(uio_map0 + location) = 100;        // Some blue
            *(uio_map0 + location + 1) = 15+(x-100)/2;     // A little green
            *(uio_map0 + location + 2) = 200-(y-100)/5;    // A lot of red
            *(uio_map0 + location + 3) = 0;      // No transparency
        }
    }
}

void notified(sel4cp_channel ch) {
    switch (ch) {
        case VMM_CH: {
            // Draw triangle
            sel4cp_dbg_puts("rectangle notified! writing to uio_map0\n");
            fbwrite();
            sel4cp_notify(VMM_CH);
            break;
        }
        default:
            break;
    }
}

