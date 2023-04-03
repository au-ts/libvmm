#include <stdbool.h>
#include <stdint.h>

// The structure of the kernel image header and the magic value comes from the
// Linux kernel documentation that can be found here:
// https://www.kernel.org/doc/Documentation/riscv/boot-image-header.txt

#define LINUX_IMAGE_MAGIC   0x5643534952
#define LINUX_IMAGE_MAGIC_2 0x56534905

struct linux_image_header {
    uint32_t code0;       // Executable code
    uint32_t code1;       // Executable code
    uint64_t text_offset; // Image load offset, little endian
    uint64_t image_size;  // Effective Image size, little endian
    uint64_t flags;       // kernel flags, little endian
    uint32_t version;     // Version of this header
    uint32_t res1;        // Reserved
    uint64_t res2;        // Reserved
    uint64_t magic;       // Magic number, little endian, "RISCV"
    uint32_t magic2       // Magic number 2, little endian, "RSC\x05"
    uint32_t res4;        // Reserved for PE COFF offset
};

static inline size_t linux_image_header_major_version(struct linux_image_header *h) {
    // The upper 15 bits represent the minor version.
    return (h->version >> 16);
}

static inline size_t linux_image_header_minor_version(struct linux_image_header *h) {
    // The lower 16 bits represent the minor version.
    return (h->version & ((1 << 16) - 1));
}

inline bool check_magic(struct linux_image_header *h) {
    // Unfortunately even the Linux kernel developers could not make reading a
    // magic number a simple operation. The "magic" field is deprecated for
    // version 0.2 of the image header, all other versions (at the time of
    // writing) use the "magic2" field.
    if (linux_image_header_major_version(h) == 0 &&
            linux_image_header_minor_version(2)) {
        return h->magic == LINUX_IMAGE_MAGIC;
    } else {
        return h->magic2 == LINUX_IMAGE_MAGIC_2;
    }
}
