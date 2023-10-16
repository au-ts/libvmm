/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include "util/util.h"
#include <stddef.h>
#include "uio.h"

#define VMM_CH 0

uint8_t *uio_map0;

void init() {
    // Initialising rectangle PD
    microkit_dbg_puts("SEND RECTANGLE|INFO: starting!\n");
}

fb_config_t *config;
uint8_t *fb_base;
uint64_t line_len;

size_t num_draws = 0;

void fbwrite() {
    // Draw triangle
    // int start = 100 + (num_draws * 2);
    // int end = 300 + (num_draws * 2);
    // for (int y = start; y < end; y++) {
    //     for (int x = start; x < end; x++) {

    //         uint64_t location = (x * (config->bpp/8)) + (y * line_len);

    //         *(fb_base + location) = 100;        // Some blue
    //         *(fb_base + location + 1) = 15+(x-100)/2;     // A little green
    //         *(fb_base + location + 2) = 200-(y-100)/5;    // A lot of red
    //         *(fb_base + location + 3) = 0;      // No transparency
    //     }
    // }
    if (num_draws == config->xres * config->yres - 1) {
        printf("END\n");
    }

    *(fb_base + num_draws * 4) = 255;
    num_draws += 1;
}

bool readconfig() {
    config = get_fb_config(uio_map0);
    if (config == NULL) {
        printf("RECT|ERROR: config is NULL\n");
        return false;
    }
    get_fb_base_addr(uio_map0, (void **)&fb_base);
    if (fb_base == NULL) {
        printf("RECT|ERROR: fb_base is NULL\n");
        return false;
    }

    printf("RECT|INFO: xres: %d, yres: %d, bpp: %d\n", config->xres, config->yres, config->bpp);
    line_len = config->xres * (config->bpp/8);
    return true;
}

void notified(microkit_channel ch) {
    switch (ch) {
        case VMM_CH: {
            // Draw triangle
            printf("RECT|INFO: reading device configuration\n");
            if (!readconfig()) {
                printf("RECT|ERROR: failed to read device configuration\n");
                break;
            }
            printf("RECT|INFO: writing to uio_map0\n");
            fbwrite();
            microkit_notify(VMM_CH);
            break;
        }
        default:
            break;
    }
}

