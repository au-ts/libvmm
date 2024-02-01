/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sddf/blk/shared_queue.h>

#include "libuio.h"
#include "block.h"

#define BLK_CH 3

blk_queue_handle_t h;
uintptr_t blk_data;
blk_storage_info_t *blk_config;

/* as much as we don't want to expose the UIO interface to the guest driver,
 * over engineering is still not a good idea. For not I think the guest driver
 * should be aware of the Linux UIO interface
 */
void init(int fd)
{    
    blk_req_queue_t *req_queue;
    if ((req_queue = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == (void *) -1) {
        printf("req_queue: mmap failed, errno: %d\n", errno);
        close(fd);
        exit(1);
    }

    blk_resp_queue_t *resp_queue;
    if ((resp_queue = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 1 * getpagesize())) == (void *) -1) {
        printf("resp_queue: mmap failed, errno: %d\n", errno);
        close(fd);
        exit(2);
    }

    void *data;
    if ((data = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 2 * getpagesize())) == (void *) -1) {
        printf("blk_data: mmap failed, errno: %d\n", errno);
        close(fd);
        exit(3);
    }
    blk_data = (uintptr_t)data;

    if ((blk_config = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 3 * getpagesize())) == (void *) -1) {
        printf("blk_config: mmap failed, errno: %d\n", errno);
        close(fd);
        exit(4);
    }

    blk_queue_init(&h, req_queue, resp_queue, false, BLK_REQ_QUEUE_SIZE, BLK_RESP_QUEUE_SIZE);
}

void notified(microkit_channel ch)
{
    if (ch == BLK_CH) {
        printf("Received notification on channel %d\n", ch);

        blk_request_code_t ret_code;
        uintptr_t ret_addr;
        uint32_t ret_sector;
        uint16_t ret_count;
        uint32_t ret_id;

        while (!blk_req_queue_empty(&h)) {
            blk_dequeue_req(&h, &ret_code, &ret_addr, &ret_sector, &ret_count, &ret_id);
            printf("Received command: code=%d, addr=%p, sector=%d, count=%d, id=%d\n", ret_code, (void *)ret_addr, ret_sector, ret_count, ret_id);

            blk_response_status_t status = SUCCESS;
            uintptr_t addr = ret_addr;
            uint16_t count = ret_count;
            uint16_t success_count = ret_count;
            uint32_t id = ret_id;
            //@ericc: what happens if this response is dropped? should we have a timeout in client?
            if (blk_resp_queue_full(&h)) {
                printf("Response ring is full, dropping response\n");
                continue;
            }
            blk_enqueue_resp(&h, status, addr, count, success_count, id);    
        }

        microkit_notify(BLK_CH);
    }
}
