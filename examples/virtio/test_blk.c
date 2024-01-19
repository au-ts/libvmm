#include <microkit.h>
#include <sddf/blk/shared_queue.h>
#include "util/util.h"

/* Uncomment this to enable debug logging */
#define DEBUG_BLKTEST

#if defined(DEBUG_BLKTEST)
#define LOG_BLKTEST(...) do{ printf("VIRTIO(BLK_TEST): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_BLKTEST(...) do{}while(0)
#endif

#define LOG_BLKTEST_ERR(...) do{ printf("VIRTIO(BLK_TEST)|ERROR: "); printf(__VA_ARGS__); }while(0)

uintptr_t blk_req_queue;
uintptr_t blk_resp_queue;
blk_queue_handle_t blk_queue_handle;

#define BLK_CH 1

void init(void) 
{
    LOG_BLKTEST("Starting blk_test\n");
    blk_queue_init(&blk_queue_handle,
                    (blk_req_queue_t *)blk_req_queue,
                    (blk_resp_queue_t *)blk_resp_queue,
                    false, 0, 0);
}

void notified(microkit_channel ch)
{
    if (ch == BLK_CH) {
        LOG_BLKTEST("Received notification on channel %d\n", ch);

        blk_request_code_t ret_code;
        uintptr_t ret_addr;
        uint32_t ret_sector;
        uint16_t ret_count;
        uint32_t ret_id;

        while (!blk_req_queue_empty(&blk_queue_handle)) {
            blk_dequeue_req(&blk_queue_handle, &ret_code, &ret_addr, &ret_sector, &ret_count, &ret_id);
            LOG_BLKTEST("Received command: code=%d, addr=%p, sector=%d, count=%d, id=%d\n", ret_code, ret_addr, ret_sector, ret_count, ret_id);

            blk_response_status_t status = SUCCESS;
            uintptr_t addr = ret_addr;
            uint16_t count = ret_count;
            uint16_t success_count = ret_count;
            uint32_t id = ret_id;
            //@ericc: what happens if this response is dropped? should we have a timeout in client?
            if (blk_resp_queue_full(&blk_queue_handle)) {
                LOG_BLKTEST_ERR("Response ring is full, dropping response\n");
                continue;
            }
            blk_enqueue_resp(&blk_queue_handle, status, addr, count, success_count, id);    
        }

        microkit_notify(BLK_CH);
    }
}