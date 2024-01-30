/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sddf/blk/shared_queue.h>

#include "libuio.h"
#include "block.h"

blk_queue_handle_t h;

/* as much as we don't want to expose the UIO interface to the guest driver,
 * over engineering is still not a good idea. For not I think the guest driver
 * should be aware of the Linux UIO interface
 */
void init(int fd)
{
    // if ((rx_ring.free_ring = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 1 * getpagesize())) == (void *) -1) {
    //     printf("mmap failed, errno: %d\n", errno);
    //     close(fd);
	//     return 1;
    // }

    // if ((rx_ring.used_ring = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 2 * getpagesize())) == (void *) -1) {
    //     printf("mmap failed, errno: %d\n", errno);
    //     close(fd);
	//     return 1;
    // }

    // if ((tx_ring.free_ring = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 3 * getpagesize())) == (void *) -1) {
    //     printf("mmap failed, errno: %d\n", errno);
    //     close(fd);
	//     return 1;
    // }

    // if ((tx_ring.used_ring = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 4 * getpagesize())) == (void *) -1) {
    //     printf("mmap failed, errno: %d\n", errno);
    //     close(fd);
	//     return 1;
    // }
}

void notified(microkit_channel ch)
{
    assert(!"should not be here!");
}
