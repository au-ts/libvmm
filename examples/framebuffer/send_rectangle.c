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

size_t num_draws = 0;

void fbwrite() {
    // Draw triangle
    int start = 100 + (num_draws * 2);
    int end = 300 + (num_draws * 2);
    for (int y = start; y < end; y++) {
        for (int x = start; x < end; x++) {

            uint64_t location = (x * (BPP/8)) + (y * LINE_LEN);
            // printf("UIO location: 0x%lx\n", location);

            if (x == 100 && y == 100) {
                printf("RECT|INFO: first location: 0x%lx\n", location);
            }
            if (x == 299 && y == 299) {
                printf("RECT|INFO: final location: 0x%lx\n", location);
            }

            *(uio_map0 + location) = 100;        // Some blue
            *(uio_map0 + location + 1) = 15+(x-100)/2;     // A little green
            *(uio_map0 + location + 2) = 200-(y-100)/5;    // A lot of red
            *(uio_map0 + location + 3) = 0;      // No transparency
        }
    }
    num_draws += 1;
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

