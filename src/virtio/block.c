/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <microkit.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <libvmm/guest.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <libvmm/virtio/config.h>
#include <libvmm/virtio/virtq.h>
#include <libvmm/virtio/virtio.h>
#include <libvmm/virtio/block.h>
#include <libvmm/virtio/virtio.h>
#include <sddf/blk/queue.h>
#include <sddf/util/fsmalloc.h>
#include <sddf/util/ialloc.h>

/* This file implements a guest-visible "virtio block" device. It translate standardised
 * virtio block requests into sDDF block requests. A virtio block request is always paired
 * with a virtio block response. But it is possible for one virtio block request to be translated
 * into multiple sDDF block requests internally within the device for reasons explained below.
 *
 * Note that while the sDDF block protocol operates in 4 KiB blocks, virtio block operates on
 * 512-bytes sectors. For example, reading sector 8 or 9 means reading block number 1. This difference
 * in protocol is what we are trying to reconcile in this code. The phrase "transfer window" and "blocks" will
 * be used interchangably but refers to the same concept, a 4 KiB block in sDDF block protocol.
 *
 * Due to the difference in transfer size, a read or write request may be "aigned" or "unaligned".
 * In this case, aligned means that the number of sectors in the request is divisible by the number
 * of sectors that can fit within one transfer window, and the starting sector is aligned on the start
 * of a transfer window. The diagram below illustrates the alignment concept:
 *
 * |Sector|Sector|Sector|Sector|Sector|Sector|Sector|Sector|
 * |         sDDF 4KiB Block / Transfer window             |
 *
 * For example:
 * - Write request starting at sector 8, of size 4096 bytes is aligned.
 * - Write request starting at sector 8, of size 8192 bytes is aligned.
 * - Write request starting at sector 8, of size 512 bytes is unaligned.
 * - Write request starting at sector 9, of size 3584 bytes is unaligned.
 *
 * A virtio block request has the following structure:
 * struct virtio_blk_req {
 *     le32 type;
 *     le32 ioprio;
 *     le64 sector;
 *     u8 data[];
 *     u8 status;
 * };
 * This is represented as a linked list of buffers in guest RAM, formally called a "descriptor chain".
 * Briefly, when a guest wants to send us (the device) a request, it enqueues a descriptor chain
 * into the "available" ring and then notify us via a fault. We then dequeue the descriptor chain and
 * "decode" it. Which entails copying the list of buffers' pointers and inspecting the header. The header is
 * all struct members before `data`.
 *
 * The header and the sum of the length of all the desciptor chain's buffers tell us:
 * 1. What type of virtio block request? (Read, Write, Get ID, Flush...),
 * 2. Which 512-bytes sector on the disk is the request starting from, and
 * 3. How much data the guest want to transfer, which is divisible by sector size.
 *
 * Once a virtio block request has been "decoded", we begin to process it immediately.
 *
 * For a read request, a corresponding sDDF block request will be dispatched whether it is
 * aligned or not.
 *
 * For a write request, there are 3 cases we must consider:
 * 1. Aligned and does not overlap with other requests.
 * In this case, it is safe to dispatch it to sDDF block right away, as there is no
 * synchronisation needed. Thus, a sDDF block write request will be generated.
 *
 * 2. Not aligned and does not overlap with other requests.
 * In this case we have to perform a Read-Modify-Write (RMW) procedure. RMW is needed because
 * there are data in the transfer window that we don't want to clobber. For example,
 * writing 1024 bytes to sector 10 will look like this:
 *
 * <-no clobber--><---write----><-------no clobber--------->
 * |Sector|Sector|Sector|Sector|Sector|Sector|Sector|Sector|
 * |         sDDF 4KiB Block / Transfer window             |
 *
 * We have to read in the entire 4KiB block, modify the range of data requested by the guest,
 * then write out the entire block to disk.
 *
 * 3. Does overlap with other requests
 * In this case, it is necessary to queue up the request for synchronisation. Because in the
 * guest's perspective, issuing 2 requests for writing to sector 8 and 9, each 512 bytes won't
 * cause any overlap. But internally in our code, both requests map to sDDF block 1. Thus we have
 * to dispatch RMW for the first request, wait asynchronously for that to finish. Then dispatch
 * RMW for the second request.
 *
 * When the guest did not negotiate VIRTIO_BLK_F_SIZE_MAX or VIRTIO_BLK_F_SEG_MAX, it is possible
 * that it will give us a request with a data size that exceeds our sDDF block data region size.
 * Meaning we will never be able to satisfy the request and leading to the guest deadlocking. To
 * solve this problem the device will divide the request up into multiple smaller chunk and process
 * them sequentially, then only consider the request completed when it have finished with all the chunks.
 *
 * Therefore each virtio block request has an attached state machine. A request can be in one of the
 * following states at any given time:
 * 1. Flushing
 * 2. Reading
 * 3. Writing aligned
 * 4. Queued for RMW
 * 5. Read phase of RMW
 * 6. Write phase of RMW
 * 7. Completed (this is represented as an invalid state machine in the code)
 *
 * Once a virtio block request is decoded and initially processed, it can only be in states 1, 2, 3, 4 or 5.
 * For a state transition to occur in a request's state machine, a notification from the block virtualiser
 * must be received and a sDDF block response with that request's identifier dequeued.
 * It is only possible for states 1, 2 and 3 to transition to state 7.
 * Once a request is in state 7, a virtio block response is generated and it cannot transition to another state.
 *
 * Therefore the only possible state transitions are:
 * A. Flushing (1) -> Completed (7)
 * B. Reading (2) -> Completed (7)
 * C. Writing aligned (3) -> Completed (7)
 * D. Read phase of RMW (5) -> Write phase of RMW (6) -> Completed (7)
 * E. Queued for RMW (4) -> Read phase of RMW (5) -> Write phase of RMW (6) -> Completed (7)
 *
 * In E, it is only possible for a request in state 4 to transition to state 5 if another write request
 * completely perform transitions C or D.
 *
 */

#define VIRTIO_BLK_DEV_ID "libvmm"
#define SECTORS_IN_TRANSFER_WINDOW (BLK_TRANSFER_SIZE / VIRTIO_BLK_SECTOR_SIZE)

/* If we need to split up the guest's request into multiple chunks for reason explained above then
 * make the chunk equal to a quarter of the data region. This is arbitrarily chosen, can be increased
 * or decreased safely up to the data region size. */
#define CHUNK_SIZE_BYTES ((SDDF_MAX_DATA_CELLS / 4) * BLK_TRANSFER_SIZE)

#define LOG_BLOCK_WARN(...)               \
    do                                   \
    {                                    \
        printf("VIRTIO(BLOCK)|WARN: "); \
        printf(__VA_ARGS__);             \
    } while (0)

#define LOG_BLOCK_ERR(...)               \
    do                                   \
    {                                    \
        printf("VIRTIO(BLOCK)|ERROR: "); \
        printf(__VA_ARGS__);             \
    } while (0)

static inline struct virtio_blk_device *device_state(struct virtio_device *dev)
{
    return (struct virtio_blk_device *)dev->device_data;
}

static void virtio_blk_regs_init(struct virtio_device *dev)
{
    dev->regs.DeviceID = VIRTIO_DEVICE_ID_BLOCK;
    dev->regs.VendorID = VIRTIO_DEV_VENDOR_ID;
}

static inline void virtio_blk_reset(struct virtio_device *dev)
{
    for (int i = 0; i < dev->num_vqs; i++) {
        dev->vqs[i].virtq.avail_gpa = 0;
        dev->vqs[i].virtq.used_gpa = 0;
        dev->vqs[i].virtq.desc_gpa = 0;
        dev->vqs[i].last_idx = 0;
        dev->vqs[i].ready = false;
    }
    assert(blk_queue_empty_req(&device_state(dev)->queue_h));
    assert(blk_queue_empty_resp(&device_state(dev)->queue_h));
    memset(&dev->regs, 0, sizeof(virtio_device_regs_t));
    memset(device_state(dev)->reqsbk, 0, sizeof((device_state(dev)->reqsbk)));

    virtio_blk_regs_init(dev);
}

static inline bool virtio_blk_get_device_features(struct virtio_device *dev, uint32_t *features)
{
    if (dev->regs.Status & VIRTIO_CONFIG_S_FEATURES_OK) {
        LOG_BLOCK_ERR("driver somehow wants to read device features after FEATURES_OK\n");
    }

    switch (dev->regs.DeviceFeaturesSel) {
    /* feature bits 0 to 31 */
    case 0:
        *features = BIT_LOW(VIRTIO_BLK_F_FLUSH);
        *features = *features | BIT_LOW(VIRTIO_BLK_F_BLK_SIZE);
        *features = *features | BIT_LOW(VIRTIO_BLK_F_SIZE_MAX);
        *features = *features | BIT_LOW(VIRTIO_BLK_F_SEG_MAX);
        *features = *features | BIT_LOW(VIRTIO_BLK_F_TOPOLOGY);
        break;
    /* features bits 32 to 63 */
    case 1:
        *features = BIT_HIGH(VIRTIO_F_VERSION_1);
        break;
    default:
        *features = 0;
        break;
    }

    return true;
}

static inline bool virtio_blk_set_driver_features(struct virtio_device *dev, uint32_t features)
{
    /* According to virtio initialisation protocol,
       this should check what device features were set, and return the subset of
       features understood by the driver. */
    bool success = false;

    uint32_t device_features = 0;
    device_features |= BIT_LOW(VIRTIO_BLK_F_FLUSH);
    device_features |= BIT_LOW(VIRTIO_BLK_F_BLK_SIZE);
    device_features |= BIT_LOW(VIRTIO_BLK_F_SIZE_MAX);
    device_features |= BIT_LOW(VIRTIO_BLK_F_SEG_MAX);
    device_features |= BIT_LOW(VIRTIO_BLK_F_TOPOLOGY);

    switch (dev->regs.DriverFeaturesSel) {
    /* feature bits 0 to 31 */
    case 0:
        success = (device_features & features) == features;
        break;
    /* features bits 32 to 63 */
    case 1:
        success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
        break;
    default:
        success = true;
        break;
    }

    if (success) {
        dev->regs.DriverFeatures = features;
        dev->features_happy = 1;
    }

    return success;
}

static inline bool virtio_blk_get_device_config(struct virtio_device *dev, uint32_t offset, uint32_t *ret_val)
{
    struct virtio_blk_device *state = device_state(dev);
    uintptr_t config_base_addr = (uintptr_t)&state->config;
    memcpy((void *)ret_val, (void *)(config_base_addr + offset), 4);

    return true;
}

static inline bool virtio_blk_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t val)
{
    struct virtio_blk_device *state = device_state(dev);
    uintptr_t config_base_addr = (uintptr_t)&state->config;
    memcpy((void *)(config_base_addr + offset), (void *)&val, 4);
    return true;
}

/* What is the number of bytes that the guest want to read from/write to the disk? */
static uint64_t reqbk_to_body_bytes(reqbk_t *reqbk)
{
    return reqbk->total_req_size - sizeof(struct virtio_blk_outhdr) - 1;
}

/* Convert the virtio sector (512b) number into sDDF block (4k) number. */
static uint64_t reqbk_to_sddf_block_num(reqbk_t *reqbk)
{
    return ((reqbk->virtio_sector * VIRTIO_BLK_SECTOR_SIZE) + reqbk->bytes_completed) / BLK_TRANSFER_SIZE;
}

/* What is the distance from the sDDF block 4k boundary to the first 512b sector in the request? */
static uint64_t reqbk_to_sddf_data_offset(reqbk_t *reqbk)
{
    return ((reqbk->virtio_sector * VIRTIO_BLK_SECTOR_SIZE) + reqbk->bytes_completed) % BLK_TRANSFER_SIZE;
}

/* How many sDDF 4k blocks do we need to read from or write to for the given request's sector and number
 * of requesting bytes? */
static uint64_t reqbk_to_sddf_num_blocks(reqbk_t *reqbk, uint64_t proposed_bytes)
{
    assert(proposed_bytes);

    /* Offset within the data cells for reading/writing virtio data */
    uint64_t data_offset = reqbk_to_sddf_data_offset(reqbk);
    uint64_t sddf_count = (data_offset + proposed_bytes) / BLK_TRANSFER_SIZE;
    if ((data_offset + proposed_bytes) % BLK_TRANSFER_SIZE) {
        /* Request will cross a transfer window or needs to be rounded up to transfer window. */
        sddf_count++;
    }

    /* Never returns 0. */
    assert(sddf_count);
    return sddf_count;
}

/* Returns true if both requests hit the same block */
static bool do_requests_overlap(reqbk_t *req1, reqbk_t *req2)
{
    if (req1->bytes_in_flight == 0 || req2->bytes_in_flight == 0) {
        return false;
    }

    uint64_t start1 = reqbk_to_sddf_block_num(req1);
    uint64_t end1 = start1 + reqbk_to_sddf_num_blocks(req1, req1->bytes_in_flight) - 1;
    uint64_t start2 = reqbk_to_sddf_block_num(req2);
    uint64_t end2 = start2 + reqbk_to_sddf_num_blocks(req2, req2->bytes_in_flight) - 1;
    return (start1 <= end2 && start2 <= end1);
}

static bool request_is_write(reqbk_t *req)
{
    return req->state >= VIRTIO_BLK_REQ_STATE_WRITING_ALIGNED;
}

static inline void virtio_blk_set_req_fail(virtio_queue_handler_t *vq_handler, reqbk_t *reqbk)
{
    char fail_byte = VIRTIO_BLK_S_IOERR;
    assert(
        virtio_write_data_to_desc_chain(vq_handler, reqbk->virtio_desc_head, 1, reqbk->total_req_size - 1, &fail_byte));
}

static inline void virtio_blk_set_req_success(virtio_queue_handler_t *vq_handler, reqbk_t *reqbk)
{
    char ok_byte = VIRTIO_BLK_S_OK;
    assert(
        virtio_write_data_to_desc_chain(vq_handler, reqbk->virtio_desc_head, 1, reqbk->total_req_size - 1, &ok_byte));
}

bool decode_virtio_block_request(virtio_queue_handler_t *vq_handler, uint16_t desc_head, reqbk_t *ret)
{
    /* A virtio block request looks like this:
       struct virtio_blk_req {
            le32 type;
            le32 ioprio;
            le64 sector;
            u8 data[];
            u8 status;
       };
       We just need to walk the scatter-gather list, and re-assemble the data
     */
    uint64_t payload_len = virtio_desc_chain_payload_len(vq_handler, desc_head);
    if (payload_len < sizeof(struct virtio_blk_outhdr) + 1) {
        /* Malicious guest driver */
        LOG_BLOCK_ERR("decode_virtio_block_request(): desc head %u, payload length %lu bytes too short\n", desc_head,
                      payload_len);
        return false;
    }

    ret->virtio_desc_head = desc_head;
    ret->total_req_size = payload_len;

    /* Step 2: read the request header from the scatter-gather list */
    struct virtio_blk_outhdr header;
    assert(
        virtio_read_data_from_desc_chain(vq_handler, desc_head, sizeof(struct virtio_blk_outhdr), 0, (char *)&header));

    ret->virtio_req_type = header.type;
    ret->virtio_sector = header.sector;
    ret->bytes_remaining = reqbk_to_body_bytes(ret);

    return true;
}

/* Returns false if request can't be process *right now*. Try again later. */
static bool dispatch_request(struct virtio_device *dev, reqbk_t *reqbk, uint32_t req_id)
{
    /* Should only be called for virtio blk read or write requests. */
    assert(reqbk->state != VIRTIO_BLK_REQ_STATE_INVALID);
    assert(reqbk->virtio_req_type == VIRTIO_BLK_T_IN || reqbk->virtio_req_type == VIRTIO_BLK_T_OUT);
    assert(reqbk->bytes_remaining);

    int err;
    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ];
    struct virtio_blk_device *state = device_state(dev);

    bool resources_ok = true;

    /* Allocate data cells from sddf data region for the entire request. This may fail since
     * there might be fragmentation in the data region or a few requests are using a lot of
     * space.
     *
     * But if the entire request can't fit in the data region then we will have to split it up.
     *
     * We check against `reqbk_to_body_bytes(reqbk)` rather than reqbk->bytes_remaining here
     * because this will be called again in `virtio_blk_handle_resp` and the situation might
     * change such that the remaining chunks will be under SDDF_MAX_DATA_CELLS. But we might
     * not have enough resources for them and fail. We cannot fail in `virtio_blk_handle_resp`
     * so once the request is chunked it stays chunked. */
    uint64_t proposed_bytes = reqbk->bytes_remaining;
    uint64_t sddf_num_blocks = reqbk_to_sddf_num_blocks(reqbk, proposed_bytes);
    if (reqbk_to_sddf_num_blocks(reqbk, reqbk_to_body_bytes(reqbk)) > SDDF_MAX_DATA_CELLS) {
        proposed_bytes = MIN(CHUNK_SIZE_BYTES, reqbk->bytes_remaining);
        sddf_num_blocks = reqbk_to_sddf_num_blocks(reqbk, proposed_bytes);
    }

    if (fsmalloc_alloc(&state->fsmalloc, &reqbk->sddf_data_cell_base, sddf_num_blocks) == -1) {
        /* Data region is full. Eventually we will be able to service this request.
         * We should only get here if this is a fresh request and this function was called from
         * handle_client_requests(). */
        assert(reqbk->bytes_completed == 0);
        LOG_BLOCK_WARN("Data region is full\n");
        resources_ok = false;
    }

    if (!resources_ok) {
        LOG_BLOCK_WARN("virtio block: out of resource for request at sector %lu, body bytes %lu, sddf count %lu\n",
                       reqbk->virtio_sector, reqbk_to_body_bytes(reqbk), sddf_num_blocks);
        return resources_ok;
    }

    uintptr_t sddf_offset = reqbk->sddf_data_cell_base - ((struct virtio_blk_device *)dev->device_data)->data_region;
    uint64_t sddf_block = reqbk_to_sddf_block_num(reqbk);

    reqbk->sddf_data_offset = reqbk_to_sddf_data_offset(reqbk);
    reqbk->sddf_count_in_flight = sddf_num_blocks;
    reqbk->bytes_remaining -= proposed_bytes;
    reqbk->bytes_in_flight += proposed_bytes;
    assert(reqbk->bytes_completed + reqbk->bytes_in_flight + reqbk->bytes_remaining == reqbk_to_body_bytes(reqbk));

    if (reqbk->virtio_req_type == VIRTIO_BLK_T_IN) {
        err = blk_enqueue_req(&state->queue_h, BLK_REQ_READ, sddf_offset, sddf_block, sddf_num_blocks, req_id);
        assert(!err);
        reqbk->state = VIRTIO_BLK_REQ_STATE_READING;
    } else if (reqbk->virtio_req_type == VIRTIO_BLK_T_OUT) {
        /* If the write request is not aligned on the sddf transfer window, we need
         * to do a read-modify-write: we need to first read the surrounding
         * memory, overwrite the memory on the unaligned areas, and then write the
         * entire memory back to disk.
         */
        bool aligned_on_transfer_window = true;
        if (proposed_bytes % BLK_TRANSFER_SIZE != 0 || reqbk->sddf_data_offset != 0) {

            aligned_on_transfer_window = false;
        }

        /* Check if this request overlap with other requests, if so, also perform read modify write.
         * But we queue it up. */
        bool overlap_with_other_requests = false;
        for (int i = 0; i < SDDF_MAX_QUEUE_CAPACITY; i++) {
            if (i != req_id && state->reqsbk[i].state != VIRTIO_BLK_REQ_STATE_INVALID
                && do_requests_overlap(&state->reqsbk[i], reqbk) && request_is_write(&state->reqsbk[i])) {

                overlap_with_other_requests = true;
                break;
            }
        }

        if (aligned_on_transfer_window && !overlap_with_other_requests) {
            /* Normal case, just send a normal write and we are done. */
            /* Copy data from virtio buffer to sddf buffer */
            assert(reqbk->sddf_data_offset == 0); /* Should be aligned */
            assert(virtio_read_data_from_desc_chain(vq, reqbk->virtio_desc_head, proposed_bytes,
                                                    sizeof(struct virtio_blk_outhdr) + reqbk->bytes_completed,
                                                    (char *)reqbk->sddf_data_cell_base));

            err = blk_enqueue_req(&state->queue_h, BLK_REQ_WRITE, sddf_offset, sddf_block, sddf_num_blocks, req_id);
            assert(!err);

            reqbk->state = VIRTIO_BLK_REQ_STATE_WRITING_ALIGNED;
        } else if (!aligned_on_transfer_window && !overlap_with_other_requests) {
            /* Read modify write as described above */
            err = blk_enqueue_req(&state->queue_h, BLK_REQ_READ, sddf_offset, sddf_block, sddf_num_blocks, req_id);
            assert(!err);

            reqbk->state = VIRTIO_BLK_REQ_STATE_RMW_READING;
        } else if (overlap_with_other_requests) {
            reqbk->state = VIRTIO_BLK_REQ_STATE_RMW_QUEUEING;
        }
    }

    return true;
}

/* Returns true if there are responses ready for the guest */
static bool handle_client_requests(struct virtio_device *dev, int *num_reqs_consumed)
{
    /* If multiqueue feature bit negotiated, should read which queue from
       dev->QueueNotify, but for now we just assume it's the one and only default
       queue */
    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ];

    struct virtio_blk_device *state = device_state(dev);

    bool have_responses = false;
    int nums_consumed = 0;

    uint16_t desc_head;
    while (virtio_virtq_peek_avail(vq, &desc_head)) {
        /* Generate sddf request id and bookkeep the request */
        uint32_t req_id;
        if (ialloc_alloc(&state->ialloc, &req_id) == -1) {
            goto stop_processing;
        }
        if (blk_queue_full_req(&state->queue_h)) {
            ialloc_free(&state->ialloc, req_id);
            goto stop_processing;
        }
        memset(&state->reqsbk[req_id], 0, sizeof(reqbk_t));

        assert(decode_virtio_block_request(vq, desc_head, &state->reqsbk[req_id]));

        switch (state->reqsbk[req_id].virtio_req_type) {
        case VIRTIO_BLK_T_IN:
        case VIRTIO_BLK_T_OUT: {
            /* Quick sanity check, the body must be of multiple sector size,
             * if it is not then something is wrong with the decoding process
             * or the guest was malicious. The former is more likely so we
             * will keep the assert for now to catch such issue for further
             * investigations. */
            assert(state->reqsbk[req_id].bytes_remaining % VIRTIO_BLK_SECTOR_SIZE == 0);

            if (!dispatch_request(dev, &state->reqsbk[req_id], req_id)) {
                /* Create backpressure, don't consume this request until the block virtualiser gives us
                 * responses to free up resources */
                state->reqsbk[req_id].state = VIRTIO_BLK_REQ_STATE_INVALID;
                ialloc_free(&state->ialloc, req_id);
                goto stop_processing;
            } else {
                nums_consumed += 1;
                assert(virtio_virtq_pop_avail(vq, &desc_head));
                break;
            }
        }
        case VIRTIO_BLK_T_FLUSH: {
            state->reqsbk[req_id].state = VIRTIO_BLK_REQ_STATE_FLUSHING;

            int err = blk_enqueue_req(&state->queue_h, BLK_REQ_FLUSH, 0, 0, 0, req_id);
            assert(!err);
            nums_consumed += 1;
            assert(virtio_virtq_pop_avail(vq, &desc_head));
            break;
        }
        case VIRTIO_BLK_T_GET_ID: {
            uint32_t body_bytes = reqbk_to_body_bytes(&state->reqsbk[req_id]);
            uint64_t bytes_to_write = MIN(body_bytes, sizeof(VIRTIO_BLK_DEV_ID));
            assert(virtio_write_data_to_desc_chain(vq, state->reqsbk[req_id].virtio_desc_head, bytes_to_write,
                                                   sizeof(struct virtio_blk_outhdr), VIRTIO_BLK_DEV_ID));
            nums_consumed += 1;
            assert(virtio_virtq_pop_avail(vq, &desc_head));
            virtio_virtq_add_used(vq, desc_head, 0);
            virtio_blk_set_req_success(vq, &state->reqsbk[req_id]);
            have_responses = true;
            break;
        }
        default: {
            LOG_BLOCK_ERR("Handling VirtIO block request, but virtIO request type is "
                          "not recognised: %d\n",
                          state->reqsbk[req_id].virtio_req_type);
            virtio_blk_set_req_fail(vq, &state->reqsbk[req_id]);
            virtio_virtq_add_used(vq, state->reqsbk[req_id].virtio_desc_head, 0);
            ialloc_free(&state->ialloc, req_id);
            state->reqsbk[req_id].state = VIRTIO_BLK_REQ_STATE_INVALID;
            have_responses = true;
            break;
        }
        }
    }

stop_processing:
    *num_reqs_consumed = nums_consumed;

    return have_responses;
}

static bool virtio_blk_queue_notify(struct virtio_device *dev)
{
    int nums_consumed = 0;
    bool have_responses = handle_client_requests(dev, &nums_consumed);

    bool virq_inject_success = true;
    if (have_responses) {
        virtio_set_interrupt_status(dev, true, false);
        virq_inject_success = virtio_inject_interrupt(dev);
    }

    struct virtio_blk_device *state = device_state(dev);
    if (nums_consumed && !blk_queue_plugged_req(&state->queue_h)) {
        microkit_notify(state->server_ch);
    }

    return virq_inject_success;
}

bool virtio_blk_handle_resp(struct virtio_blk_device *state)
{
    int err = 0;
    struct virtio_device *dev = &state->virtio_device;

    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ];

    blk_resp_status_t sddf_ret_status;
    uint16_t sddf_ret_success_count;
    uint32_t sddf_ret_id;

    bool virt_notify = false;
    bool resp_handled = false;
    bool read_write_modify_inflight = false;
    while (!blk_queue_empty_resp(&state->queue_h)) {
        err = blk_dequeue_resp(&state->queue_h, &sddf_ret_status, &sddf_ret_success_count, &sddf_ret_id);
        assert(!err);

        /* Retrieve request bookkeep information */
        reqbk_t *reqbk = &state->reqsbk[sddf_ret_id];
        assert(reqbk->state != VIRTIO_BLK_REQ_STATE_INVALID);

        bool resp_success = false;
        if (sddf_ret_status == BLK_RESP_OK) {
            resp_success = true;
            switch (reqbk->virtio_req_type) {
            case VIRTIO_BLK_T_IN: {
                /* Copy data into guest RAM */
                assert(virtio_write_data_to_desc_chain(vq, reqbk->virtio_desc_head, reqbk->bytes_in_flight,
                                                       sizeof(struct virtio_blk_outhdr) + reqbk->bytes_completed,
                                                       (char *)(reqbk->sddf_data_cell_base + reqbk->sddf_data_offset)));
                reqbk->bytes_completed += reqbk->bytes_in_flight;
                reqbk->bytes_in_flight = 0;
                break;
            }
            case VIRTIO_BLK_T_OUT: {
                if (reqbk->state == VIRTIO_BLK_REQ_STATE_RMW_READING) {
                    /* Handling read-modify-write procedure, copy virtio write data to the
                     * correct offset in the same sddf data region allocated to do the
                     * surrounding read.
                     */
                    assert(virtio_read_data_from_desc_chain(
                        vq, reqbk->virtio_desc_head, reqbk->bytes_in_flight,
                        sizeof(struct virtio_blk_outhdr) + reqbk->bytes_completed,
                        (char *)(reqbk->sddf_data_cell_base + reqbk->sddf_data_offset)));

                    state->reqsbk[sddf_ret_id].state = VIRTIO_BLK_REQ_STATE_RMW_WRITING;
                    err = blk_enqueue_req(&state->queue_h, BLK_REQ_WRITE,
                                          reqbk->sddf_data_cell_base
                                              - ((struct virtio_blk_device *)dev->device_data)->data_region,
                                          reqbk_to_sddf_block_num(reqbk), reqbk->sddf_count_in_flight, sddf_ret_id);
                    assert(!err);
                    virt_notify = true;
                    read_write_modify_inflight = true;
                    /* The virtIO request is not complete yet so we don't tell the driver
                     * (just skip over to next request) */
                    continue;
                } else if (reqbk->state == VIRTIO_BLK_REQ_STATE_WRITING_ALIGNED
                           || reqbk->state == VIRTIO_BLK_REQ_STATE_RMW_WRITING) {
                    reqbk->bytes_completed += reqbk->bytes_in_flight;
                    reqbk->bytes_in_flight = 0;
                } else {
                    /* unreachable unless bugs */
                    assert(false);
                }
                break;
            }
            case VIRTIO_BLK_T_FLUSH:
                break;
            default: {
                LOG_BLOCK_ERR("Retrieving sDDF block response, but virtIO request type "
                              "is not recognised: %d\n",
                              reqbk->virtio_req_type);
                resp_success = false;
                break;
            }
            }

            assert(reqbk->bytes_completed + reqbk->bytes_in_flight + reqbk->bytes_remaining
                   == reqbk_to_body_bytes(reqbk));
            assert(!reqbk->bytes_in_flight);
            /* If we get here then the current chunk in the request have been completed in full.
             * Process the next chunk if we still have work to do for this request. */
            if (reqbk->bytes_remaining) {
                fsmalloc_free(&state->fsmalloc, reqbk->sddf_data_cell_base, reqbk->sddf_count_in_flight);
                reqbk->sddf_data_cell_base = 0;
                reqbk->sddf_count_in_flight = 0;

                /* This should not fail since we have freed resources of the previous chunk and all the
                 * chunks are the same size if a request is chunked.
                 * i.e. fsmalloc_free have already made contiguous free hole for the next chunk. */
                assert(dispatch_request(dev, reqbk, sddf_ret_id));
                virt_notify = true;

                /* Skip over to the next request, this request have not finished yet. */
                continue;
            }

            if (reqbk->state == VIRTIO_BLK_REQ_STATE_WRITING_ALIGNED
                || reqbk->state == VIRTIO_BLK_REQ_STATE_RMW_WRITING) {
                /* If we get here, we've just finished processing a normal or unaligned write. Now check
                   which requests have been blocked on this request's completion and process them. */
                int active_write_reqs[SDDF_MAX_QUEUE_CAPACITY];
                int num_active_write_reqs = 0;
                for (int i = 0; i < SDDF_MAX_QUEUE_CAPACITY; i++) {
                    if (i != sddf_ret_id
                        && (state->reqsbk[i].state == VIRTIO_BLK_REQ_STATE_WRITING_ALIGNED
                            || state->reqsbk[i].state == VIRTIO_BLK_REQ_STATE_RMW_READING
                            || state->reqsbk[i].state == VIRTIO_BLK_REQ_STATE_RMW_WRITING)) {
                        active_write_reqs[num_active_write_reqs] = i;
                        num_active_write_reqs++;
                    }
                }

                for (int i = 0; i < SDDF_MAX_QUEUE_CAPACITY; i++) {
                    if (state->reqsbk[i].state == VIRTIO_BLK_REQ_STATE_RMW_QUEUEING) {
                        bool i_req_safe_to_dispatch = true;
                        for (int j = 0; j < num_active_write_reqs; j++) {
                            if (do_requests_overlap(&state->reqsbk[i], &state->reqsbk[active_write_reqs[j]])) {

                                i_req_safe_to_dispatch = false;
                                break;
                            }
                        }

                        if (i_req_safe_to_dispatch) {
                            state->reqsbk[i].state = VIRTIO_BLK_REQ_STATE_RMW_READING;
                            uintptr_t next_sddf_offset = state->reqsbk[i].sddf_data_cell_base
                                                       - ((struct virtio_blk_device *)dev->device_data)->data_region;

                            err = blk_enqueue_req(&state->queue_h, BLK_REQ_READ, next_sddf_offset,
                                                  reqbk_to_sddf_block_num(&state->reqsbk[i]),
                                                  state->reqsbk[i].sddf_count_in_flight, i);
                            assert(!err);

                            virt_notify = true;
                            read_write_modify_inflight = true;

                            active_write_reqs[num_active_write_reqs] = i;
                            num_active_write_reqs++;
                        }
                    }
                }
            }
        }

        if (resp_success) {
            virtio_blk_set_req_success(vq, reqbk);
        } else {
            virtio_blk_set_req_fail(vq, reqbk);
        }

        /* Free corresponding bookkeeping structures regardless of the request's
         * success status.
         */
        if (reqbk->virtio_req_type == VIRTIO_BLK_T_IN || reqbk->virtio_req_type == VIRTIO_BLK_T_OUT) {
            fsmalloc_free(&state->fsmalloc, reqbk->sddf_data_cell_base, reqbk->sddf_count_in_flight);
        }

        virtio_virtq_add_used(vq, reqbk->virtio_desc_head, 0);

        reqbk->state = VIRTIO_BLK_REQ_STATE_INVALID;
        err = ialloc_free(&state->ialloc, sddf_ret_id);
        assert(!err);

        resp_handled = true;
    }

    int nums_pending_cmds_consumed = 0;
    if (!read_write_modify_inflight) {
        handle_client_requests(dev, &nums_pending_cmds_consumed);
        if (nums_pending_cmds_consumed) {
            virt_notify = true;
        }
    }

    /* We need to know if we've finished handling all the requests in the previous cycle, if we did, we inject an
     * interrupt, if we didn't we don't inject.
     */
    bool virq_inject_success = true;
    if (resp_handled && !read_write_modify_inflight && !virt_notify) {
        virtio_set_interrupt_status(dev, true, false);
        virq_inject_success = virtio_inject_interrupt(dev);
    }

    if (virt_notify) {
        microkit_notify(state->server_ch);
    }

    return virq_inject_success;
}

static inline void virtio_blk_config_init(struct virtio_blk_device *blk_dev)
{
    blk_storage_info_t *storage_info = blk_dev->storage_info;

    blk_dev->config.capacity = (BLK_TRANSFER_SIZE / VIRTIO_BLK_SECTOR_SIZE) * storage_info->capacity;
    if (storage_info->block_size != 0) {
        blk_dev->config.blk_size = storage_info->block_size * BLK_TRANSFER_SIZE;
    } else {
        blk_dev->config.blk_size = storage_info->sector_size;
    }

    blk_dev->config.size_max = VIRTIO_BLK_SIZE_MAX;
    blk_dev->config.seg_max = VIRTIO_BLK_SEG_MAX;

    blk_dev->config.topology.physical_block_exp =
        3; // 2^3 = 8 logical blocks in 1 physical block (sDDF transfer window)
    blk_dev->config.topology.alignment_offset = 0;
    blk_dev->config.topology.min_io_size = 8;
    blk_dev->config.topology.opt_io_size = blk_dev->config.topology.min_io_size;
}

static virtio_device_funs_t functions = {
    .device_reset = virtio_blk_reset,
    .get_device_features = virtio_blk_get_device_features,
    .set_driver_features = virtio_blk_set_driver_features,
    .get_device_config = virtio_blk_get_device_config,
    .set_device_config = virtio_blk_set_device_config,
    .queue_notify = virtio_blk_queue_notify,
};

static struct virtio_device *virtio_blk_init(struct virtio_blk_device *blk_dev, virtio_transport_type_t type,
                                             irq_routing_info_t irq_routing_info, uintptr_t data_region,
                                             size_t data_region_size, blk_storage_info_t *storage_info,
                                             blk_queue_handle_t *queue_h, uint32_t queue_capacity, int server_ch)
{
    struct virtio_device *dev = &blk_dev->virtio_device;

    virtio_blk_regs_init(dev);
    dev->transport_type = type;
    dev->funs = &functions;
    dev->vqs = blk_dev->vqs;
    dev->num_vqs = VIRTIO_BLK_NUM_VIRTQ;
    dev->irq_routing_info = irq_routing_info;
    dev->device_data = blk_dev;

    blk_dev->storage_info = storage_info;
    blk_dev->queue_h = *queue_h;
    blk_dev->data_region = data_region;
    blk_dev->queue_capacity = queue_capacity;
    blk_dev->server_ch = server_ch;

    size_t num_sddf_cells = (data_region_size / BLK_TRANSFER_SIZE) < SDDF_MAX_DATA_CELLS
                              ? (data_region_size / BLK_TRANSFER_SIZE)
                              : SDDF_MAX_DATA_CELLS;

    assert(num_sddf_cells == queue_capacity);

    virtio_blk_config_init(blk_dev);

    fsmalloc_init(&blk_dev->fsmalloc, data_region, BLK_TRANSFER_SIZE, num_sddf_cells, &blk_dev->fsmalloc_avail_bitarr,
                  blk_dev->fsmalloc_avail_bitarr_words, BITS_2_WORDS64(num_sddf_cells));

    ialloc_init(&blk_dev->ialloc, blk_dev->ialloc_idxlist, SDDF_MAX_QUEUE_CAPACITY);

    return dev;
}

#if !defined(CONFIG_ARCH_X86)
bool virtio_mmio_blk_init(struct virtio_blk_device *blk_dev, uintptr_t region_base, uintptr_t region_size,
                          irq_routing_info_t irq_routing_info, uintptr_t data_region, size_t data_region_size,
                          blk_storage_info_t *storage_info, blk_queue_handle_t *queue_h, uint32_t queue_capacity,
                          int server_ch)
{
    struct virtio_device *dev = virtio_blk_init(blk_dev, VIRTIO_TRANSPORT_MMIO, irq_routing_info, data_region,
                                                data_region_size, storage_info, queue_h, queue_capacity, server_ch);

    return virtio_mmio_register_device(dev, region_base, region_size, irq_routing_info);
}
#endif

bool virtio_pci_blk_init(struct virtio_blk_device *blk_dev, uint16_t pci_bus, uint16_t pci_dev,
                         irq_routing_info_t irq_routing_info, uintptr_t data_region, size_t data_region_size,
                         blk_storage_info_t *storage_info, blk_queue_handle_t *queue_h, uint32_t queue_capacity,
                         int server_ch)
{
    struct virtio_device *dev = virtio_blk_init(blk_dev, VIRTIO_TRANSPORT_PCI, irq_routing_info, data_region,
                                                data_region_size, storage_info, queue_h, queue_capacity, server_ch);

    dev->transport.pci.device_id = VIRTIO_PCI_MODERN_BASE_DEVICE_ID + VIRTIO_DEVICE_ID_BLOCK;
    dev->transport.pci.vendor_id = VIRTIO_PCI_VENDOR_ID;
    dev->transport.pci.device_class = PCI_CLASS_STORAGE_SCSI;

    return virtio_pci_register_device(dev, pci_bus, pci_dev, irq_routing_info);
}
