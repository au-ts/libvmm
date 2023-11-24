#include <microkit.h>
#include <block/libblocksharedringbuffer/include/sddf_blk_shared_ringbuffer.h>
#include "util/util.h"

/* Uncomment this to enable debug logging */
#define DEBUG_BLKTEST

#if defined(DEBUG_BLKTEST)
#define LOG_BLKTEST(...) do{ printf("VIRTIO(BLK_TEST): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_BLKTEST(...) do{}while(0)
#endif

#define LOG_BLKTEST_ERR(...) do{ printf("VIRTIO(BLK_TEST)|ERROR: "); printf(__VA_ARGS__); }while(0)

uintptr_t blk_cmd_ring;
uintptr_t blk_resp_ring;
uintptr_t blk_desc_handle;
uintptr_t blk_data;
sddf_blk_ring_handle_t blk_ring_handle;

#define BLK_CH 1

void init(void) 
{
    LOG_BLKTEST("Starting blk_test\n");
    sddf_blk_ring_init(&blk_ring_handle,
                    (sddf_blk_cmd_ring_buffer_t *)blk_cmd_ring,
                    (sddf_blk_resp_ring_buffer_t *)blk_resp_ring,
                    false, 0, 0);
}

void notified(microkit_channel ch)
{
    if (ch == BLK_CH) {
        LOG_BLKTEST("Received notification on channel %d\n", ch);

        sddf_blk_command_code_t ret_code;
        uintptr_t ret_addr;
        uint32_t ret_sector;
        uint16_t ret_count;
        uint32_t ret_id;

        while (sddf_blk_dequeue_cmd(&blk_ring_handle, &ret_code, &ret_addr, &ret_sector, &ret_count, &ret_id) != -1) {
            LOG_BLKTEST("Received command: code=%d, desc=%d, sector=%d, count=%d, id=%d\n", ret_code, ret_addr, ret_sector, ret_count, ret_id);
        }
    }
}