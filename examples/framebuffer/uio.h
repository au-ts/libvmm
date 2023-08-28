#pragma once

#define UIO_INIT_ADDRESS 0x300000

typedef struct fb_config {
    uint32_t xres;
    uint32_t yres;
    uint32_t bpp;
} fb_config_t;
