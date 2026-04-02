/*
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/util/util.h>
#include <libvmm/util/printf.h>

/* This is required to use the printf library we brought in, it is
   simply for convenience since there's a lot of logging/debug printing
   in the VMM. */
void _putchar(char character)
{
    microkit_dbg_putc(character);
}

void print_mem_hex(uintptr_t addr, size_t size)
{
#ifdef CONFIG_DEBUG_BUILD
    for (size_t i = 0; i < size; i++) {
        printf("%02X ", *((uint8_t *)addr + i));
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
#endif
}

bool is_dhcp_frame(uintptr_t addr)
{
    uint16_t ethtype = htons(addr + 0xC);
    if (ethtype == 0x0800) {
        uint8_t type = addr + 0x17;
        if (type == 17) {
            uint16_t srcp = htons(addr + 0x22);
            uint16_t dstp = htons(addr + 0x24);
            if (srcp == 67 && dstp == 68) {
                return true;
            }
        }
    }
    return false;
}
