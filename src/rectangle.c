/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4cp.h>
#include "util/util.h"
#include <stddef.h>

#define RECT_CH 55
#define INIT_CH 57

#define XRES 1074
#define YRES 1074
#define LINE_LEN 1074
#define BPP 32

uint8_t *uio_map0;

void init() {
    // Initialising rectangle PD
    LOG_VMM("\"%s\" starting up \n", sel4cp_name);
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
    printf("notified on channel %d\n", ch);
    switch (ch) {
        case INIT_CH: {
            // Draw triangle
            printf("rectangle notified! writing to uio_map0\n");
            fbwrite();
            sel4cp_notify(RECT_CH);
            break;
        }
        default:
            break;
    }
}

