#pragma once

#include <stdint.h>

struct QemuRamFBCfg {
    uint64_t address;   /**< The address of the framebuffer. */
    uint32_t fourcc;    /**< The four character code representing the pixel format (use FORMAT_ defines). */
    uint32_t flags;     /**< Flags for framebuffer configuration. */
    uint32_t width;     /**< The width of the framebuffer in pixels. */
    uint32_t height;    /**< The height of the framebuffer in pixels. */
    uint32_t stride;    /**< The stride (number of bytes per row) of the framebuffer (width * bytes_per_pixel). */
} __attribute__((packed));
