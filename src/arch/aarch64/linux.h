#include <stdint.h>

// The structure of the kernel image header and the magic value comes from the
// Linux kernel documentation that can be found here:
// https://www.kernel.org/doc/Documentation/arm64/booting.txt

#define LINUX_IMAGE_MAGIC 0x644d5241

struct linux_image_header {
    uint32_t code0;       // Executable code
    uint32_t code1;       // Executable code
    uint64_t text_offset; // Image load offset, little endian
    uint64_t image_size;  // Effective Image size, little endian
    uint64_t flags;       // kernel flags, little endian
    uint64_t res2;        // reserved
    uint64_t res3;        // reserved
    uint64_t res4;        // reserved
    uint32_t magic;       // Magic number, little endian, "ARM\x64"
    uint32_t res5;        // reserved (used for PE COFF offset)
};
