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

/* Uncomment this to enable debug logging */
#define DEBUG_UIO_BLOCK

#if defined(DEBUG_UIO_BLOCK)
#define LOG_UIO_BLOCK(...) do{ printf("UIO_DRIVER(BLOCK): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_UIO_BLOCK(...) do{}while(0)
#endif

#define LOG_UIO_BLOCK_ERR(...) do{ printf("UIO_DRIVER(BLOCK)|ERROR: "); printf(__VA_ARGS__); }while(0)

#define BLK_CH 3

blk_queue_handle_t h;
uintptr_t blk_data;
blk_storage_info_t *blk_config;

int driver_init(void **maps, int num_maps)
{    
    if (num_maps != 4) {
        LOG_UIO_BLOCK_ERR("Expecting 4 maps, got %d\n", num_maps);
        return -1;
    }
    
    blk_req_queue_t *req_queue = (blk_req_queue_t *)maps[0];
    blk_resp_queue_t *resp_queue = (blk_resp_queue_t *)maps[1];
    blk_data = (uintptr_t)maps[2];
    blk_config = (blk_storage_info_t *)maps[3];

    blk_queue_init(&h, req_queue, resp_queue, false, BLK_REQ_QUEUE_SIZE, BLK_RESP_QUEUE_SIZE);
    
    // @TODO, @ericc: Query the block device we have and fill in the blk_config
    // just random numbers I've chosen for now
    blk_config->blocksize = 1024;
    blk_config->size = 1024 * 1024 * 1024;
    blk_config->read_only = false;
    blk_config->ready = true;

    // Notify the other side that driver is ready
    uio_notify();
    
    return 0;
}

void driver_notified()
{
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

    uio_notify();
}
