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
#include <sddf/blk/queue.h>
#include <sddf/util/fsmalloc.h>
#include <sddf/util/ialloc.h>

#if defined(CONFIG_ARCH_X86_64)
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/util.h>
#endif

#define SECTORS_IN_TRANSFER_WINDOW (BLK_TRANSFER_SIZE / VIRTIO_BLK_SECTOR_SIZE)

/* Uncomment this to enable debug logging */
// #define DEBUG_BLOCK

#if defined(DEBUG_BLOCK)
#define LOG_BLOCK(...)             \
    do                             \
    {                              \
        printf("VIRTIO(BLOCK): "); \
        printf(__VA_ARGS__);       \
    } while (0)
#else
#define LOG_BLOCK(...) \
    do                 \
    {                  \
    } while (0)
#endif

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

static inline void virtio_blk_reset(struct virtio_device *dev)
{
    LOG_VMM("block device reset!\n");
    for (int i = 0; i < dev->num_vqs; i++) {
        dev->vqs[i].virtq.avail = 0;
        dev->vqs[i].virtq.used = 0;
        dev->vqs[i].virtq.desc = 0;
        dev->vqs[i].last_idx = 0;
        dev->vqs[i].ready = false;
    }
    assert(blk_queue_empty_req(&device_state(dev)->queue_h));
    assert(blk_queue_empty_resp(&device_state(dev)->queue_h));
    memset(&dev->regs, 0, sizeof(virtio_device_regs_t));
    dev->regs.DeviceID = VIRTIO_PCI_MODERN_BASE_DEVICE_ID + VIRTIO_DEVICE_ID_BLOCK;
    dev->regs.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;

    memset(device_state(dev)->reqsbk, 0, sizeof((device_state(dev)->reqsbk)));
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
        *features |= BIT_LOW(VIRTIO_BLK_F_BLK_SIZE);
        *features |= BIT_LOW(VIRTIO_BLK_F_SIZE_MAX);
        *features |= BIT_LOW(VIRTIO_BLK_F_SEG_MAX);
        *features |= BIT_LOW(VIRTIO_BLK_F_TOPOLOGY);
        break;
    /* features bits 32 to 63 */
    case 1:
        *features = BIT_HIGH(VIRTIO_F_VERSION_1);
        break;
    default:
        *features = 0;
        LOG_BLOCK_ERR("driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", dev->regs.DeviceFeaturesSel);
        return true;
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
        LOG_VMM("device_features = 0x%x, features = 0x%x\n", device_features, features);
        success = (device_features & features) == features;
        break;
    /* features bits 32 to 63 */
    case 1:
        success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
        break;
    default:
        LOG_BLOCK_ERR("driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", dev->regs.DriverFeaturesSel);
        success = true;
    }

    if (success) {
        // TODO: must be done for all other virtIO impls
        dev->regs.DriverFeatures = features;
        dev->features_happy = 1;
    } else {
        LOG_VMM("set dev features not happy\n");
    }

    return success;
}

static inline bool virtio_blk_get_device_config(struct virtio_device *dev, uint32_t offset, uint32_t *ret_val)
{
    struct virtio_blk_device *state = device_state(dev);

    uintptr_t config_base_addr = (uintptr_t)&state->config;
    memcpy((char *)ret_val, (char *)(config_base_addr + offset), 4);

    LOG_BLOCK("get device config with base_addr 0x%x and offset 0x%x has "
              "value %d\n",
              config_base_addr, offset, *ret_val);

    return true;
}

static inline bool virtio_blk_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t val)
{
    struct virtio_blk_device *state = device_state(dev);

    uintptr_t config_base_addr = (uintptr_t)&state->config;
    memcpy((char *)(config_base_addr + offset), (char *)&val, 4);
    LOG_BLOCK("set device config with base_addr 0x%x and offset 0x%x with "
              "value %d\n",
              config_base_addr, offset, val);

    return true;
}

static inline void virtio_blk_used_buffer(struct virtio_device *dev, uint16_t desc)
{
    struct virtq *virtq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].virtq;
    struct virtq_used_elem used_elem = { desc, 0 };

    virtq->used->ring[virtq->used->idx % virtq->num] = used_elem;
    virtq->used->idx++;
}

// TODO: this function is not specific to block at all but rather all virtIO devices,
// should be made generic instead.
static inline bool virtio_blk_virq_inject(struct virtio_device *dev)
{
    assert(dev->regs.InterruptStatus);
    bool success;
#if defined(CONFIG_ARCH_AARCH64)
    success = virq_inject(dev->virq);
#elif defined(CONFIG_ARCH_X86_64)
    success = inject_ioapic_irq(0, dev->virq);
#endif
    return success;
}

// TODO: this function is not specific to block at all but rather all virtIO devices,
// should be made generic instead.
static inline void virtio_blk_set_interrupt_status(struct virtio_device *dev, bool used_buffer, bool config_change)
{
    /* Set the reason of the irq.
       bit 0: used buffer
       bit 1: configuration change */
    dev->regs.InterruptStatus = 0;
    if (used_buffer) {
        dev->regs.InterruptStatus |= 0x1;
    }
    if (config_change) {
        dev->regs.InterruptStatus |= 0x2;
    }
}

static inline bool virtio_blk_respond(struct virtio_device *dev) {
    if (dev->transport_type == VIRTIO_TRANSPORT_PCI && dev->regs.InterruptStatus) {
        // Don't inject the IRQ if InterruptStatus is already set on PCI since reading
        // from InterruptStatus will clear it.
        return false;
    }

    virtio_blk_set_interrupt_status(dev, true, false);
    return virtio_blk_virq_inject(dev);
}

static inline void virtio_blk_set_req_fail(struct virtio_device *dev, uint16_t desc)
{
    struct virtq *virtq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].virtq;

    /* Loop to the final byte of the final descriptor and write response code
     * there */
    uint16_t curr_desc = desc;
    while (virtq->desc[curr_desc].flags & VIRTQ_DESC_F_NEXT) {
        curr_desc = virtq->desc[curr_desc].next;
    }
    assert(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_WRITE);
    *(uint8_t *)(gpa_to_vaddr(virtq->desc[curr_desc].addr) + virtq->desc[curr_desc].len - 1) = VIRTIO_BLK_S_IOERR;
}

static inline void virtio_blk_set_req_success(struct virtio_device *dev, uint16_t desc)
{
    struct virtq *virtq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].virtq;

    uint16_t curr_desc = desc;
    while (virtq->desc[curr_desc].flags & VIRTQ_DESC_F_NEXT) {
        curr_desc = virtq->desc[curr_desc].next;
    }
    assert(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_WRITE);
    *(uint8_t *)(gpa_to_vaddr(virtq->desc[curr_desc].addr) + virtq->desc[curr_desc].len - 1) = VIRTIO_BLK_S_OK;
}

static inline bool sddf_make_req_check(struct virtio_blk_device *state, uint16_t sddf_count)
{
    /* Check if ialloc is full, if data region is full, if req queue is full.
       If these all pass then this request can be handled successfully */

    /* A sanity check that all the maths checks out. <=2 because we have negotiated
       with the driver to only send 1 4k segment at any given time. And 2 because such segment
       can sit between 2 sDDF block transfer window. */
    // TODO revisit
       // if (sddf_count > 2) {
    //     LOG_BLOCK_ERR("sddf_count %d > 2\n", sddf_count);
    //     return false;
    // }

    if (ialloc_full(&state->ialloc)) {
        LOG_VMM_ERR("Request bookkeeping array is full\n");
        return false;
    }

    if (blk_queue_full_req(&state->queue_h)) {
        LOG_VMM_ERR("Request queue is full\n");
        return false;
    }

    if (fsmalloc_full(&state->fsmalloc, sddf_count)) {
        LOG_VMM_ERR("Data region is full, trying to allocate %d sddf transfer windows,\n", sddf_count);
        return false;
    }

    return true;
}

/* Returns true if both requests hit the same block */
bool do_requests_overlap(reqbk_t *req1, reqbk_t *req2)
{
    uint32_t start1 = req1->sddf_block_number;
    uint32_t end1 = start1 + req1->sddf_count - 1;
    uint32_t start2 = req2->sddf_block_number;
    uint32_t end2 = start2 + req2->sddf_count - 1;
    return (start1 <= end2 && start2 <= end1);
}

bool request_is_write(reqbk_t *req)
{
    return req->state >= STATE_WRITING_ALIGNED;
}

static bool handle_client_requests(struct virtio_device *dev, int *num_reqs_consumed)
{
    assert(dev->regs.QueueSel == VIRTIO_BLK_DEFAULT_VIRTQ);
    int err = 0;
    /* If multiqueue feature bit negotiated, should read which queue from
    dev->QueueNotify, but for now we just assume it's the one and only default
    queue */
    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ];
    struct virtq *virtq = &vq->virtq;
    if (!(vq->ready)) {
        LOG_VMM_ERR("handle_client_requests() called when vq is NOT ready\n");
        return true;
    }

    struct virtio_blk_device *state = device_state(dev);

    /* If any request has to be dropped due to any number of reasons, such as an invalid req, this becomes
     * true */
    bool has_dropped = false;
    int nums_consumed = 0;

    /* Handle available requests beginning from the last handled request */
    uint16_t last_handled_avail_idx = vq->last_idx;

    LOG_BLOCK("------------- handle_client_requests start loop -------------\n");
    for (; last_handled_avail_idx != virtq->avail->idx; last_handled_avail_idx++) {
        uint16_t desc_head = virtq->avail->ring[last_handled_avail_idx % virtq->num];
        uint16_t curr_desc = desc_head;
        uint32_t curr_desc_bytes_read = 0;

        /* There are three parts with each block request. The header, body (which
         * contains the data) and reply. */
        uint32_t header_bytes_read = 0;
        struct virtio_blk_outhdr virtio_req_header;
        for (; header_bytes_read < sizeof(struct virtio_blk_outhdr); curr_desc = virtq->desc[curr_desc].next) {
            /* Header is device read only */
            assert(!(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_WRITE));
            /* We can guarantee existence of next descriptor as footer is write only
             */
            assert(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_NEXT);
            if (header_bytes_read + virtq->desc[curr_desc].len > sizeof(struct virtio_blk_outhdr)) {
                void *src_addr = (void *)gpa_to_vaddr(virtq->desc[curr_desc].addr);
                void *dst_addr = (void *)&virtio_req_header;
                uint32_t copy_sz = sizeof(struct virtio_blk_outhdr) - header_bytes_read;
                memcpy(dst_addr, src_addr, copy_sz);
                curr_desc_bytes_read = sizeof(struct virtio_blk_outhdr) - header_bytes_read;
                header_bytes_read += sizeof(struct virtio_blk_outhdr) - header_bytes_read;
                /* Don't go to the next descriptor yet, we're not done processing with
                 * current one */
                break;
            } else {
                void *src_addr = (void *)gpa_to_vaddr(virtq->desc[curr_desc].addr);
                void *dst_addr = (void *)&virtio_req_header;
                uint32_t copy_sz = virtq->desc[curr_desc].len;
                memcpy(dst_addr, src_addr, copy_sz);
                header_bytes_read += virtq->desc[curr_desc].len;
            }
        }

        LOG_BLOCK("----- Request type is 0x%x -----\n", virtio_req_header.type);

        switch (virtio_req_header.type) {
        case VIRTIO_BLK_T_IN: {
            LOG_BLOCK("Request type is VIRTIO_BLK_T_IN\n");

            /* Converting virtio sector number to sddf block number, we are rounding
            * down */
            uint32_t sddf_block_number = (virtio_req_header.sector * VIRTIO_BLK_SECTOR_SIZE) / BLK_TRANSFER_SIZE;

            /* Figure out how many bytes are in the body of the request */
            uint32_t body_size_bytes = 0;
            uint32_t tmp_curr_desc_bytes_read = curr_desc_bytes_read;
            for (uint16_t tmp_curr_desc = curr_desc; virtq->desc[tmp_curr_desc].flags & VIRTQ_DESC_F_NEXT;
                 tmp_curr_desc = virtq->desc[tmp_curr_desc].next) {
                if (tmp_curr_desc_bytes_read != 0) {
                    body_size_bytes += virtq->desc[tmp_curr_desc].len - tmp_curr_desc_bytes_read;
                    tmp_curr_desc_bytes_read = 0;
                } else {
                    body_size_bytes += virtq->desc[tmp_curr_desc].len;
                }
                if (!(virtq->desc[tmp_curr_desc].flags & VIRTQ_DESC_F_WRITE)
                    || virtq->desc[tmp_curr_desc].len < VIRTIO_BLK_SECTOR_SIZE) {
                    break;
                }
            }

            /* Figure out whether the guest's read request spills over to the next 4k transfer window */
            uint32_t num_sectors = body_size_bytes / VIRTIO_BLK_SECTOR_SIZE;
            uint32_t sddf_count = (body_size_bytes + BLK_TRANSFER_SIZE - 1) / BLK_TRANSFER_SIZE;
            if (((virtio_req_header.sector % SECTORS_IN_TRANSFER_WINDOW) + num_sectors) > SECTORS_IN_TRANSFER_WINDOW) {
                sddf_count++;
            }

            LOG_BLOCK("Sector (read/write offset) is %d, num sddf blocks %d\n", virtio_req_header.sector, sddf_count);

            if (!sddf_make_req_check(state, sddf_count)) {
                /* One of the book-keeping structure or the data region is full */
                virtio_blk_set_req_fail(dev, curr_desc);
                virtio_blk_used_buffer(dev, curr_desc);
                nums_consumed += 1;
                break;
            }

            /* Allocate data cells from sddf data region based on sddf_count */
            uintptr_t sddf_data_cell_base;
            fsmalloc_alloc(&state->fsmalloc, &sddf_data_cell_base, sddf_count);

            /* Find address within the data cells for reading/writing virtio data */
            uintptr_t sddf_data = sddf_data_cell_base
                                + (virtio_req_header.sector * VIRTIO_BLK_SECTOR_SIZE) % BLK_TRANSFER_SIZE;

            /* Generate sddf request id and bookkeep the request */
            uint32_t req_id;
            err = ialloc_alloc(&state->ialloc, &req_id);
            assert(!err);
            assert(!state->reqsbk[req_id].valid);

            state->reqsbk[req_id] =
                (reqbk_t) { true,      desc_head,       sddf_data_cell_base, sddf_count, sddf_block_number,
                            sddf_data, body_size_bytes, STATE_READING };

            uintptr_t sddf_offset = sddf_data_cell_base - ((struct virtio_blk_device *)dev->device_data)->data_region;
            err = blk_enqueue_req(&state->queue_h, BLK_REQ_READ, sddf_offset, sddf_block_number, sddf_count, req_id);
            nums_consumed += 1;
            assert(!err);

            LOG_BLOCK("send read sector %u, sddf block %u, body size %u, data off %u, nums block %u, virtio desc %u\n",
                      virtio_req_header.sector, sddf_block_number, body_size_bytes,
                      (virtio_req_header.sector * VIRTIO_BLK_SECTOR_SIZE) % BLK_TRANSFER_SIZE, sddf_count, curr_desc);

            break;
        }
        case VIRTIO_BLK_T_OUT: {
            LOG_BLOCK("Request type is VIRTIO_BLK_T_OUT\n");
            LOG_BLOCK("Sector (read/write offset) is %d, curr_desc is %u\n", virtio_req_header.sector, curr_desc);

            /* Converting virtio sector number to sddf block number, we are rounding
             * down */
            uint32_t sddf_block_number = (virtio_req_header.sector * VIRTIO_BLK_SECTOR_SIZE) / BLK_TRANSFER_SIZE;
            LOG_BLOCK("sddf_block_number is %u\n", sddf_block_number);

            /* Figure out how many bytes are in the body of the request */
            uint32_t body_size_bytes = 0;
            uint32_t tmp_curr_desc_bytes_read = curr_desc_bytes_read;
            for (uint16_t tmp_curr_desc = curr_desc; virtq->desc[tmp_curr_desc].flags & VIRTQ_DESC_F_NEXT;
                 tmp_curr_desc = virtq->desc[tmp_curr_desc].next) {
                if (tmp_curr_desc_bytes_read != 0) {
                    body_size_bytes += virtq->desc[tmp_curr_desc].len - tmp_curr_desc_bytes_read;
                    tmp_curr_desc_bytes_read = 0;
                } else {
                    body_size_bytes += virtq->desc[tmp_curr_desc].len;
                }
                if (!(virtq->desc[tmp_curr_desc].flags & VIRTQ_DESC_F_WRITE)
                    || virtq->desc[tmp_curr_desc].len < VIRTIO_BLK_SECTOR_SIZE) {
                    break;
                }
            }
            LOG_BLOCK("body_size_bytes is %u\n", body_size_bytes);

            /* Figure out whether the guest's write request spills over to the next 4k transfer window */
            uint32_t num_sectors = body_size_bytes / VIRTIO_BLK_SECTOR_SIZE;
            uint32_t sddf_count = (body_size_bytes + BLK_TRANSFER_SIZE - 1) / BLK_TRANSFER_SIZE;
            if (((virtio_req_header.sector % SECTORS_IN_TRANSFER_WINDOW) + num_sectors) > SECTORS_IN_TRANSFER_WINDOW) {
                sddf_count++;
            }

            LOG_BLOCK("sddf_count is %u\n", sddf_count);

            if (!sddf_make_req_check(state, sddf_count)) {
                LOG_BLOCK("write: data region full at sector %u, body bytes %u, sddf count %u\n",
                          virtio_req_header.sector, body_size_bytes, sddf_count);
                virtio_blk_set_req_fail(dev, curr_desc);
                virtio_blk_used_buffer(dev, curr_desc);
                nums_consumed += 1;
                break;
            }

            /* If the write request is not aligned on the sddf transfer window, we need
             * to do a read-modify-write: we need to first read the surrounding
             * memory, overwrite the memory on the unaligned areas, and then write the
             * entire memory back to disk.
             */
            bool aligned = true;
            if (body_size_bytes % BLK_TRANSFER_SIZE != 0) {
                aligned = false;
            } else {
                aligned = ((virtio_req_header.sector % (BLK_TRANSFER_SIZE / VIRTIO_BLK_SECTOR_SIZE)) == 0);
            }

            LOG_BLOCK("aligned is %d, \n", aligned);

            if (!aligned) {
                /* Allocate data buffer from data region based on sddf_count */
                uintptr_t sddf_data_cell_base;
                assert(fsmalloc_alloc(&state->fsmalloc, &sddf_data_cell_base, sddf_count) == 0);

                /* Find address within the data cells for reading/writing virtio data */
                uintptr_t sddf_data = sddf_data_cell_base
                                    + (virtio_req_header.sector * VIRTIO_BLK_SECTOR_SIZE) % BLK_TRANSFER_SIZE;
                LOG_BLOCK("not aligned, sddf_data offset is %u\n",
                          (virtio_req_header.sector * VIRTIO_BLK_SECTOR_SIZE) % BLK_TRANSFER_SIZE);

                /* Generate sddf request id and bookkeep the request */
                uint32_t req_id;
                assert(!ialloc_alloc(&state->ialloc, &req_id));
                assert(!state->reqsbk[req_id].valid);
                state->reqsbk[req_id] =
                    (reqbk_t) { true,      desc_head,       sddf_data_cell_base, sddf_count, sddf_block_number,
                                sddf_data, body_size_bytes, STATE_RMW_READING };

                /* Before we actually do anything, double check whether we are already handling
                   an unaligned write op on that sDDF block to prevent data race. */
                bool process = true;
                for (int i = 0; i < SDDF_MAX_QUEUE_CAPACITY; i++) {
                    if (i != req_id && state->reqsbk[i].valid
                        && do_requests_overlap(&state->reqsbk[i], &state->reqsbk[req_id])
                        && request_is_write(&state->reqsbk[i])) {

                        LOG_BLOCK("not aligned and found another req inflight, queueing, this req id is %u\n", req_id);
                        process = false;
                        break;
                    }
                }

                if (process) {
                    LOG_BLOCK("not aligned and NOT found another unaligned req inflight, sending read\n");
                    uintptr_t sddf_offset = sddf_data_cell_base
                                          - ((struct virtio_blk_device *)dev->device_data)->data_region;
                    err = blk_enqueue_req(&state->queue_h, BLK_REQ_READ, sddf_offset, sddf_block_number, sddf_count,
                                          req_id);
                    nums_consumed += 1;
                    assert(!err);
                } else {
                    state->reqsbk[req_id].state = STATE_RMW_QUEUEING;
                }
            } else {
                /* Handle normal write request */
                /* Allocate data buffer from data region based on sddf_count */
                uintptr_t sddf_data_cell_base;
                assert(fsmalloc_alloc(&state->fsmalloc, &sddf_data_cell_base, sddf_count) == 0);
                /* Find address within the data cells for reading/writing virtio data */
                uintptr_t sddf_data = sddf_data_cell_base
                                    + (virtio_req_header.sector * VIRTIO_BLK_SECTOR_SIZE) % BLK_TRANSFER_SIZE;

                /* Copy data from virtio buffer to sddf buffer */
                uint32_t body_bytes_read = 0;
                for (; body_bytes_read < body_size_bytes; curr_desc = virtq->desc[curr_desc].next) {
                    /* For write requests, the body is a read descriptor, and the footer
                     * is a write descriptor, we know there must be a descriptor cut-off
                     * at the end.
                     */
                    assert(body_bytes_read + virtq->desc[curr_desc].len <= body_size_bytes);
                    assert(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_NEXT);
                    if (curr_desc_bytes_read != 0) {
                        void *src_addr = (void *)gpa_to_vaddr(virtq->desc[curr_desc].addr) + curr_desc_bytes_read;
                        void *dst_addr = (void *)sddf_data + body_bytes_read;
                        uint32_t copy_sz = virtq->desc[curr_desc].len - curr_desc_bytes_read;
                        memcpy(dst_addr, src_addr, copy_sz);
                        body_bytes_read += virtq->desc[curr_desc].len - curr_desc_bytes_read;
                        curr_desc_bytes_read = 0;
                    } else {
                        void *src_addr = (void *)gpa_to_vaddr(virtq->desc[curr_desc].addr);
                        void *dst_addr = (void *)sddf_data + body_bytes_read;
                        uint32_t copy_sz = virtq->desc[curr_desc].len;
                        memcpy(dst_addr, src_addr, copy_sz);
                        body_bytes_read += virtq->desc[curr_desc].len;
                    }
                }

                /* Generate sddf request id and bookkeep the request */
                uint32_t req_id;
                assert(!ialloc_alloc(&state->ialloc, &req_id));
                assert(!state->reqsbk[req_id].valid);
                state->reqsbk[req_id] =
                    (reqbk_t) { true,      desc_head,       sddf_data_cell_base,  sddf_count, sddf_block_number,
                                sddf_data, body_size_bytes, STATE_WRITING_ALIGNED };

                uintptr_t sddf_offset = sddf_data_cell_base
                                      - ((struct virtio_blk_device *)dev->device_data)->data_region;
                err = blk_enqueue_req(&state->queue_h, BLK_REQ_WRITE, sddf_offset, sddf_block_number, sddf_count,
                                      req_id);
                nums_consumed += 1;
                assert(!err);
                LOG_BLOCK("send normal write sector %u, sddf block %u, body size %u, data off %u, nums block %u, "
                          "virtio desc %u\n",
                          virtio_req_header.sector, sddf_block_number, body_size_bytes,
                          (virtio_req_header.sector * VIRTIO_BLK_SECTOR_SIZE) % BLK_TRANSFER_SIZE, sddf_count,
                          curr_desc);
            }
            break;
        }
        case VIRTIO_BLK_T_FLUSH: {
            LOG_BLOCK("Request type is VIRTIO_BLK_T_FLUSH\n");
            if (!sddf_make_req_check(state, 0)) {
                virtio_blk_set_req_fail(dev, curr_desc);
                virtio_blk_used_buffer(dev, desc_head);
                nums_consumed += 1;
                break;
            }

            /* Bookkeep the request */
            uint32_t req_id;
            ialloc_alloc(&state->ialloc, &req_id);
            /* except for virtio desc, nothing else needs to be retrieved later
             * so leave as 0 */
            state->reqsbk[req_id] = (reqbk_t) { true, desc_head, 0, 0, 0, 0, 0, STATE_FLUSHING };

            err = blk_enqueue_req(&state->queue_h, BLK_REQ_FLUSH, 0, 0, 0, req_id);
            nums_consumed += 1;
            break;
        }
        case VIRTIO_BLK_T_GET_ID: {
            LOG_BLOCK("Request type is VIRTIO_BLK_T_GET_ID\n");
            assert(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_NEXT);
            uint16_t next_desc = virtq->desc[curr_desc].next;
            assert(virtq->desc[next_desc].flags & VIRTQ_DESC_F_WRITE);
            char *dst_addr = (void *)gpa_to_vaddr(virtq->desc[next_desc].addr);
            LOG_BLOCK("buf len is 0x%x bytes, GPA 0x%lx\n", virtq->desc[curr_desc].len, virtq->desc[next_desc].addr);
            strcpy(dst_addr, "libvmm");
            dst_addr[6] = 0;
            nums_consumed += 1;
            virtio_blk_set_req_success(dev, curr_desc);
            virtio_blk_used_buffer(dev, desc_head);
            has_dropped = true;
            break;
        }
        default: {
            LOG_BLOCK_ERR("Handling VirtIO block request, but virtIO request type is "
                          "not recognised: %d\n",
                          virtio_req_header.type);
            virtio_blk_set_req_fail(dev, curr_desc);
            has_dropped = true;
            break;
        }
        }
    }

// TODO: handle unused label
// stop_processing:
    *num_reqs_consumed = nums_consumed;

    /* Update virtq index to the next available request to be handled */
    vq->last_idx = last_handled_avail_idx;

    return !has_dropped;
}

static bool virtio_blk_queue_notify(struct virtio_device *dev)
{
    int nums_consumed = 0;
    LOG_BLOCK("virtio_blk_queue_notify calling handle_client_requests\n");
    bool consumption_status = handle_client_requests(dev, &nums_consumed);

    // TODO: handle unused variable
    // bool virq_inject_success = true;
    if (!consumption_status) {
        LOG_BLOCK("virtio_blk_queue_notify dropped requests\n");
        // virtio_blk_set_interrupt_status(dev, true, false);
        // virq_inject_success = virtio_blk_virq_inject(dev);
        // virtio_blk_virq_inject(dev);
        virtio_blk_respond(dev);
    }

    struct virtio_blk_device *state = device_state(dev);
    if (nums_consumed && !blk_queue_plugged_req(&state->queue_h)) {
        LOG_BLOCK("virtio_blk_queue_notify notified virt\n");
        microkit_notify(state->server_ch);
    }
    return true;
}

bool virtio_blk_handle_resp(struct virtio_blk_device *state)
{
    LOG_BLOCK(" ----------- Blk virt notified VMM ----------- \n");

    int err = 0;
    struct virtio_device *dev = &state->virtio_device;

    assert(dev->regs.QueueSel == VIRTIO_BLK_DEFAULT_VIRTQ);

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

        struct virtq *virtq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].virtq;

        uint16_t curr_desc = reqbk->virtio_desc_head;
        uint32_t curr_desc_bytes_read = 0;

        uint32_t header_bytes_read = 0;
        struct virtio_blk_outhdr virtio_req_header;
        for (; header_bytes_read < sizeof(struct virtio_blk_outhdr); curr_desc = virtq->desc[curr_desc].next) {
            /* Header is device read only */
            assert(!(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_WRITE));
            /* We can always guarantee existence of next descriptor as footer is write
             * only */
            assert(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_NEXT);
            if (header_bytes_read + virtq->desc[curr_desc].len > sizeof(struct virtio_blk_outhdr)) {
                void *src_addr = (void *)gpa_to_vaddr(virtq->desc[curr_desc].addr);
                void *dst_addr = (void *)&virtio_req_header;
                uint32_t copy_sz = sizeof(struct virtio_blk_outhdr) - header_bytes_read;
                memcpy(dst_addr, src_addr, copy_sz);
                curr_desc_bytes_read = sizeof(struct virtio_blk_outhdr) - header_bytes_read;
                header_bytes_read += sizeof(struct virtio_blk_outhdr) - header_bytes_read;
                /* Don't go to the next descriptor yet, we're not done processing with
                 * current one */
                break;
            } else {
                void *src_addr = (void *)gpa_to_vaddr(virtq->desc[curr_desc].addr);
                void *dst_addr = (void *)&virtio_req_header;
                uint32_t copy_sz = virtq->desc[curr_desc].len;
                memcpy(dst_addr, src_addr, copy_sz);
                header_bytes_read += virtq->desc[curr_desc].len;
            }
        }

        bool resp_success = false;
        if (sddf_ret_status == BLK_RESP_OK) {
            resp_success = true;
            switch (virtio_req_header.type) {
            case VIRTIO_BLK_T_IN: {
                uint32_t data_off = reqbk->sddf_data - reqbk->sddf_data_cell_base;
                LOG_BLOCK("resp read sector %u, sddf block %u, body size %u, data off %u\n", virtio_req_header.sector,
                          reqbk->sddf_block_number, reqbk->virtio_body_size_bytes, data_off);
                assert(reqbk->sddf_data_cell_base + data_off == reqbk->sddf_data);

                /* Going from read (header) to write (body) descriptor, there should be
                 * a descriptor cut-off at the beginning. */
                assert(curr_desc_bytes_read == 0);
                uint32_t body_bytes_read = 0;
                for (; body_bytes_read < reqbk->virtio_body_size_bytes; curr_desc = virtq->desc[curr_desc].next) {
                    if (body_bytes_read + virtq->desc[curr_desc].len > reqbk->virtio_body_size_bytes) {
                        void *src_addr = (void *)reqbk->sddf_data + body_bytes_read;
                        void *dst_addr = (void *)gpa_to_vaddr(virtq->desc[curr_desc].addr);
                        uint32_t copy_sz = reqbk->virtio_body_size_bytes - body_bytes_read;
                        memcpy(dst_addr, src_addr, copy_sz);
                        body_bytes_read += reqbk->virtio_body_size_bytes - body_bytes_read;
                        /* This is the final descriptor if we get into this condition, don't
                        //  * go to next descriptor */
                        LOG_BLOCK("virtq->desc[curr_desc].len: %d\n", virtq->desc[curr_desc].len);
                        assert(!(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_NEXT));
                        break;
                    } else {
                        void *src_addr = (void *)reqbk->sddf_data + body_bytes_read;
                        void *dst_addr = (void *)gpa_to_vaddr(virtq->desc[curr_desc].addr);
                        uint32_t copy_sz = virtq->desc[curr_desc].len;
                        memcpy(dst_addr, src_addr, copy_sz);

                        body_bytes_read += virtq->desc[curr_desc].len;
                        /* Because there is still the footer, we are guaranteed next
                         * descriptor exists */
                        assert(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_NEXT);
                    }
                }
                break;
            }
            case VIRTIO_BLK_T_OUT: {
                if (reqbk->state == STATE_RMW_READING) {
                    LOG_BLOCK("resp read-to-write sector %u, sddf block %u, body size %u, data off %u, sddf_cell_base "
                              "0x%x, virtio desc %u\n",
                              virtio_req_header.sector, reqbk->sddf_block_number, reqbk->virtio_body_size_bytes,
                              reqbk->sddf_data - reqbk->sddf_data_cell_base, reqbk->sddf_data_cell_base, curr_desc);
                    /* Handling read-modify-write procedure, copy virtio write data to the
                     * correct offset in the same sddf data region allocated to do the
                     * surrounding read.
                     */
                    uint32_t body_bytes_read = 0;
                    for (; body_bytes_read < reqbk->virtio_body_size_bytes; curr_desc = virtq->desc[curr_desc].next) {
                        /* For write requests, the body is a read descriptor and the footer
                         * is a write descriptor, there must be a descriptor cut-off at the
                         * end
                         */
                        assert(body_bytes_read + virtq->desc[curr_desc].len <= reqbk->virtio_body_size_bytes);
                        assert(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_NEXT);
                        if (curr_desc_bytes_read != 0) {
                            void *src_addr = (void *)gpa_to_vaddr(virtq->desc[curr_desc].addr) + curr_desc_bytes_read;
                            void *dst_addr = (void *)reqbk->sddf_data + body_bytes_read;
                            uint32_t copy_sz = virtq->desc[curr_desc].len - curr_desc_bytes_read;
                            memcpy(dst_addr, src_addr, copy_sz);
                            body_bytes_read += virtq->desc[curr_desc].len - curr_desc_bytes_read;
                            curr_desc_bytes_read = 0;
                        } else {
                            void *src_addr = (void *)gpa_to_vaddr(virtq->desc[curr_desc].addr);
                            void *dst_addr = (void *)reqbk->sddf_data + body_bytes_read;
                            uint32_t copy_sz = virtq->desc[curr_desc].len;
                            memcpy(dst_addr, src_addr, copy_sz);
                            body_bytes_read += virtq->desc[curr_desc].len;
                        }
                    }

                    state->reqsbk[sddf_ret_id].state = STATE_RMW_WRITING;
                    err = blk_enqueue_req(&state->queue_h, BLK_REQ_WRITE,
                                          reqbk->sddf_data_cell_base
                                              - ((struct virtio_blk_device *)dev->device_data)->data_region,
                                          reqbk->sddf_block_number, reqbk->sddf_count, sddf_ret_id);
                    assert(!err);
                    virt_notify = true;
                    read_write_modify_inflight = true;
                    /* The virtIO request is not complete yet so we don't tell the driver
                     * (just skip over to next request) */

                    continue;
                } else {
                    LOG_BLOCK(
                        "resp write complete sector %u, sddf block %u, body size %u, data off %u, virtio desc %u\n",
                        virtio_req_header.sector, reqbk->sddf_block_number, reqbk->virtio_body_size_bytes,
                        reqbk->sddf_data - reqbk->sddf_data_cell_base, curr_desc);
                }
                break;
            }
            case VIRTIO_BLK_T_FLUSH:
                break;
            default: {
                LOG_BLOCK_ERR("Retrieving sDDF block response, but virtIO request type "
                              "is not recognised: %d\n",
                              virtio_req_header.type);
                resp_success = false;
                break;
            }
            }

            if (reqbk->state == STATE_WRITING_ALIGNED || reqbk->state == STATE_RMW_WRITING) {
                /* If we get here, we've just finished processing a normal or unaligned write. Now check
                   which request is queueing on the same sDDF block we touched and process it. */
                for (int i = 0; i < SDDF_MAX_QUEUE_CAPACITY; i++) {
                    if (state->reqsbk[i].valid && state->reqsbk[i].state == STATE_RMW_QUEUEING
                        && do_requests_overlap(&(state->reqsbk[i]), reqbk)) {

                        state->reqsbk[i].state = STATE_RMW_READING;
                        uintptr_t next_sddf_offset = state->reqsbk[i].sddf_data_cell_base
                                                   - ((struct virtio_blk_device *)dev->device_data)->data_region;

                        err = blk_enqueue_req(&state->queue_h, BLK_REQ_READ, next_sddf_offset,
                                              state->reqsbk[i].sddf_block_number, state->reqsbk[i].sddf_count, i);
                        assert(!err);

                        virt_notify = true;
                        read_write_modify_inflight = true;
                        break;
                    }
                }
            }
        }

        if (resp_success) {
            virtio_blk_set_req_success(dev, reqbk->virtio_desc_head);
        } else {
            virtio_blk_set_req_fail(dev, reqbk->virtio_desc_head);
        }

        /* Free corresponding bookkeeping structures regardless of the request's
         * success status.
         */
        if (virtio_req_header.type == VIRTIO_BLK_T_IN || virtio_req_header.type == VIRTIO_BLK_T_OUT) {

            LOG_BLOCK("freeing fs buff for sector %u\n", virtio_req_header.sector);
            fsmalloc_free(&state->fsmalloc, reqbk->sddf_data_cell_base, reqbk->sddf_count);
        }

        virtio_blk_used_buffer(dev, reqbk->virtio_desc_head);

        reqbk->valid = false;
        err = ialloc_free(&state->ialloc, sddf_ret_id);
        assert(!err);

        resp_handled = true;
    }

    int nums_pending_cmds_consumed = 0;
    if (!read_write_modify_inflight) {
        LOG_BLOCK("virtio_blk_handle_resp calling handle_client_requests\n");
        handle_client_requests(dev, &nums_pending_cmds_consumed);
        LOG_BLOCK("handle_client_requests consumed %d reqs\n", nums_pending_cmds_consumed);
        if (nums_pending_cmds_consumed) {
            virt_notify = true;
        }
    }

    /* We need to know if we've finished handling all the requests in the previous cycle, if we did, we inject an
     * interrupt, if we didn't we don't inject.
     */
    bool virq_inject_success = true;
    if (resp_handled && !read_write_modify_inflight && !virt_notify) {
        virq_inject_success = virtio_blk_respond(dev);
    }

    if (virt_notify) {
        LOG_BLOCK("virtio_blk_handle_resp virt notify\n");
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

    /* Restrict the guest driver to only send 1x 4K segment per request at any given time.
    This is to prevent internal fragmentation within the data region, leading to a deadlock
    where we can't handle large requests when the free cells in the data region isn't contiguous. */
    blk_dev->config.size_max = BLK_TRANSFER_SIZE;
    blk_dev->config.seg_max = 1;

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

static struct virtio_device *virtio_blk_init(struct virtio_blk_device *blk_dev, size_t virq, uintptr_t data_region,
                                             size_t data_region_size, blk_storage_info_t *storage_info,
                                             blk_queue_handle_t *queue_h, uint32_t queue_capacity, int server_ch)
{
    struct virtio_device *dev = &blk_dev->virtio_device;
    dev->regs.DeviceID = VIRTIO_PCI_MODERN_BASE_DEVICE_ID + VIRTIO_DEVICE_ID_BLOCK;
    dev->regs.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = blk_dev->vqs;
    dev->num_vqs = VIRTIO_BLK_NUM_VIRTQ;
    dev->virq = virq;
    dev->device_data = blk_dev;

    blk_dev->storage_info = storage_info;
    blk_dev->queue_h = *queue_h;
    blk_dev->data_region = data_region;
    blk_dev->queue_capacity = queue_capacity;
    blk_dev->server_ch = server_ch;

    size_t num_sddf_cells = (data_region_size / BLK_TRANSFER_SIZE) < SDDF_MAX_DATA_CELLS
                              ? (data_region_size / BLK_TRANSFER_SIZE)
                              : SDDF_MAX_DATA_CELLS;

    // assert(num_sddf_cells == queue_capacity);

    virtio_blk_config_init(blk_dev);

    fsmalloc_init(&blk_dev->fsmalloc, data_region, BLK_TRANSFER_SIZE, num_sddf_cells, &blk_dev->fsmalloc_avail_bitarr,
                  blk_dev->fsmalloc_avail_bitarr_words, roundup_bits2words64(num_sddf_cells));

    ialloc_init(&blk_dev->ialloc, blk_dev->ialloc_idxlist, queue_capacity);

    return dev;
}

#ifndef CONFIG_ARCH_X86_64
bool virtio_mmio_blk_init(struct virtio_blk_device *blk_dev, uintptr_t region_base, uintptr_t region_size, size_t virq,
                          uintptr_t data_region, size_t data_region_size, blk_storage_info_t *storage_info,
                          blk_queue_handle_t *queue_h, uint32_t queue_capacity, int server_ch)
{
    struct virtio_device *dev = virtio_blk_init(blk_dev, virq, data_region, data_region_size, storage_info, queue_h,
                                                queue_capacity, server_ch);

    return virtio_mmio_register_device(dev, region_base, region_size, virq);
}
#endif

bool virtio_pci_blk_init(struct virtio_blk_device *blk_dev, uint32_t dev_slot, size_t virq, uintptr_t data_region,
                         size_t data_region_size, blk_storage_info_t *storage_info, blk_queue_handle_t *queue_h,
                         uint32_t queue_capacity, int server_ch)
{
    struct virtio_device *dev = virtio_blk_init(blk_dev, virq, data_region, data_region_size, storage_info, queue_h,
                                                queue_capacity, server_ch);

    dev->transport_type = VIRTIO_TRANSPORT_PCI;
    dev->transport.pci.device_id = VIRTIO_PCI_MODERN_BASE_DEVICE_ID + VIRTIO_DEVICE_ID_BLOCK;
    dev->transport.pci.vendor_id = VIRTIO_PCI_VENDOR_ID;
    dev->transport.pci.device_class = PCI_CLASS_STORAGE_SCSI;

    bool success = virtio_pci_alloc_dev_cfg_space(dev, dev_slot);
    assert(success);

    assert(virtio_pci_alloc_memory_bar(dev, 0, VIRTIO_PCI_DEFAULT_BAR_SIZE));

    return pci_register_virtio_device(dev, virq);
}
