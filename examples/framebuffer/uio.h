#pragma once

#define UIO_INIT_ADDRESS 0x300000

/*
* Driver VM configured pixel format:
* Each pixel is 4 bytes, with the following format:
* BYTE 1: Blue
* BYTE 2: Green
* Byte 3: Red
* Byte 4: Alpha (transparency)
*/

typedef struct fb_config {
    uint32_t xres;
    uint32_t yres;
    uint32_t bpp;
} fb_config_t;

void get_config_base_addr(void *uio_map, fb_config_t **config_base_addr) {
    if (uio_map == NULL) return;
    *config_base_addr = uio_map;
}

void get_fb_base_addr(void *uio_map, void **fb_base_addr) {
    if (uio_map == NULL) return;
    *fb_base_addr = (uint8_t *)uio_map + sizeof(fb_config_t);
}