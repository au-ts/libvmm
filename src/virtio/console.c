#include "virtio/config.h"
#include "virtio/mmio.h"
#include "virtio/console.h"
#include "virq.h"
#include "util.h"
#include <sddf/serial/queue.h>

/* Uncomment this to enable debug logging */
// #define DEBUG_CONSOLE

#if defined(DEBUG_CONSOLE)
#define LOG_CONSOLE(...) do{ printf("VIRTIO(CONSOLE): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_CONSOLE(...) do{}while(0)
#endif

#define LOG_CONSOLE_ERR(...) do{ printf("VIRTIO(CONSOLE)|ERROR: "); printf(__VA_ARGS__); }while(0)

static void virtio_console_features_print(uint32_t features)
{
    /* Dump the features given in a human-readable format */
    LOG_CONSOLE("Dumping features (0x%lx):\n", features);
    LOG_CONSOLE("feature VIRTIO_CONSOLE_F_SIZE set to %s\n",
                BIT_LOW(VIRTIO_CONSOLE_F_SIZE) & features ? "true" : "false");
    LOG_CONSOLE("feature VIRTIO_CONSOLE_F_MULTIPORT set to %s\n",
                BIT_LOW(VIRTIO_CONSOLE_F_MULTIPORT) & features ? "true" : "false");
    LOG_CONSOLE("feature VIRTIO_CONSOLE_F_EMERG_WRITE set to %s\n",
                BIT_LOW(VIRTIO_CONSOLE_F_EMERG_WRITE) & features ? "true" : "false");
}

static void virtio_console_reset(struct virtio_device *dev)
{
    LOG_CONSOLE("operation: reset\n");
    LOG_CONSOLE_ERR("virtio_console_reset is not implemented!\n");

    // @ivanv reset vqs?
}

static int virtio_console_get_device_features(struct virtio_device *dev, uint32_t *features)
{
    LOG_CONSOLE("operation: get device features\n");

    switch (dev->data.DeviceFeaturesSel) {
    case 0:
        *features = 0;
        break;
    case 1:
        *features = BIT_HIGH(VIRTIO_F_VERSION_1);
        break;
    default:
        // @ivanv: audit
        LOG_CONSOLE_ERR("driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DeviceFeaturesSel);
        return 0;
    }

    return 1;
}

static int virtio_console_set_driver_features(struct virtio_device *dev, uint32_t features)
{
    LOG_CONSOLE("operation: set driver features\n");
    virtio_console_features_print(features);

    int success = 1;

    switch (dev->data.DriverFeaturesSel) {
    // feature bits 0 to 31
    case 0:
        // The device initialisation protocol says the driver should read device feature bits,
        // and write the subset of feature bits understood by the OS and driver to the device.
        // Currently we only have one feature to check.
        // success = (features == BIT_LOW(VIRTIO_NET_F_MAC));
        break;
    // features bits 32 to 63
    case 1:
        success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
        break;
    default:
        LOG_CONSOLE_ERR("driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DriverFeaturesSel);
        return false;
    }

    if (success) {
        dev->data.features_happy = 1;
        LOG_CONSOLE("device is feature happy\n");
    }

    return success;
}

static int virtio_console_get_device_config(struct virtio_device *dev, uint32_t offset, uint32_t *config)
{
    LOG_CONSOLE("operation: get device config\n");
    return -1;
}

static int virtio_console_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t config)
{
    LOG_CONSOLE("operation: set device config\n");
    return -1;
}

/* The guest has notified us that it has placed something in the transmit queue. */
static int virtio_console_handle_tx(struct virtio_device *dev)
{
    LOG_CONSOLE("operation: handle transmit\n");

    // @ivanv: we need to check the pre-conditions before doing anything. e.g check
    // TX_QUEUE is ready?

    assert(dev->num_vqs > TX_QUEUE);
    struct virtio_queue_handler *tx_queue = &dev->vqs[TX_QUEUE];
    struct virtq *virtq = &tx_queue->virtq;
    uint16_t guest_idx = virtq->avail->idx;
    size_t idx = tx_queue->last_idx;

    serial_queue_handle_t *sddf_tx_queue = (serial_queue_handle_t *)dev->sddf_handlers[SDDF_SERIAL_TX_HANDLE].queue_h;
    while (idx != guest_idx) {
        LOG_CONSOLE("processing available buffers from index [0x%lx..0x%lx)\n", idx, guest_idx);
        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

        /* Continue traversing the chained buffers */
        struct virtq_desc desc;
        uint16_t desc_idx = desc_head;
        do {
            desc = virtq->desc[desc_idx];
            LOG_CONSOLE("processing descriptor (0x%lx) with buffer [0x%lx..0x%lx)\n", desc_idx, desc.addr, desc.addr + desc.len);
            // @ivanv: to the debug logging, we should actually print out teh buffer contents

            /* Now that we have a buffer, we can transfer the data to the sDDF multiplexor */
            /* We first need a free buffer from the TX queue */
            uintptr_t sddf_buffer = 0;
            unsigned int sddf_buffer_len = 0;
            LOG_CONSOLE("tx queue free size: 0x%lx, tx queue active size: 0x%lx\n", serial_queue_size(sddf_tx_queue->free),
                        serial_queue_size(sddf_tx_queue->active));
            assert(!serial_queue_empty(sddf_tx_queue->free));
            int ret = serial_dequeue_free(sddf_tx_queue, &sddf_buffer, &sddf_buffer_len);
            assert(!ret);
            if (ret != 0) {
                LOG_CONSOLE_ERR("could not dequeue from the TX free queue\n");
                // @ivanv: todo, handle this properly
            }

            // @ivanv: handle this in release mode
            if (desc.len > sddf_buffer_len) {
                LOG_CONSOLE_ERR("%s expected sddf_buffer_len (0x%lx) <= desc.len (0x%lx)\n", microkit_name, sddf_buffer_len, desc.len);
            }
            assert(desc.len <= sddf_buffer_len);

            /* Copy the data over, these buffers are in the guests's RAM and hence inaccessible
             * by the multiplexor. */
            memcpy((char *) sddf_buffer, (char *) desc.addr, desc.len);

            bool is_empty = serial_queue_empty(sddf_tx_queue->active);
            /* Now we can enqueue our buffer into the active TX queue */
            ret = serial_enqueue_active(sddf_tx_queue, sddf_buffer, desc.len);
            // @ivanv: handle case in release made
            assert(ret == 0);

            if (is_empty) {
                // @ivanv: should we be using the notify_reader/notify_writer API?
                microkit_notify(dev->sddf_handlers[SDDF_SERIAL_TX_HANDLE].ch);
            }

            /* Lastly, move to the next descriptor in the chain */
            desc_idx = desc.next;
        } while (desc.flags & VIRTQ_DESC_F_NEXT);

        /* Our final job is to move the available virtq into the used virtq */
        struct virtq_used_elem used_elem;
        used_elem.id = desc_head;
        /* We did not write to any of the buffers, so len is zero. */
        used_elem.len = 0;
        virtq->used->ring[guest_idx % virtq->num] = used_elem;
        virtq->used->idx++;

        idx++;
    }

    /* Move the available index past every virtq we've processed. */
    // @ivanv: not sure about this line
    virtq->avail->idx = idx;

    tx_queue->last_idx = idx;

    dev->data.InterruptStatus = BIT_LOW(0);
    // @ivanv: The virq_inject API is poor as it expects a vCPU ID even though
    // it doesn't matter for the case of SPIs, which is what virtIO devices use.
    bool success = virq_inject(GUEST_VCPU_ID, dev->virq);
    assert(success);

    return success;
}

int virtio_console_handle_rx(struct virtio_device *dev)
{
    // @ivanv: revisit this whole function, it works but is very hacky.
    /* We have received something from the real console driver.
     * Our job is to inspect the sDDF active RX queue, and dequeue everything
     * we can and give it to the guest driver.
     */
    serial_queue_handle_t *sddf_rx_queue = (serial_queue_handle_t *)dev->sddf_handlers[SDDF_SERIAL_RX_HANDLE].queue_h;
    uintptr_t sddf_buffer = 0;
    unsigned int sddf_buffer_len = 0;
    int ret = serial_dequeue_active(sddf_rx_queue, &sddf_buffer, &sddf_buffer_len);
    assert(!ret);
    if (ret != 0) {
        LOG_CONSOLE_ERR("could not dequeue from RX active queue\n");
        // @ivanv: handle properly
    }

    assert(dev->num_vqs > RX_QUEUE);
    struct virtio_queue_handler *rx_queue = &dev->vqs[RX_QUEUE];
    struct virtq *virtq = &rx_queue->virtq;
    uint16_t guest_idx = virtq->avail->idx;
    size_t idx = rx_queue->last_idx;

    if (idx != guest_idx) {
        size_t bytes_written = 0;

        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];
        struct virtq_desc desc;
        uint16_t desc_idx = desc_head;

        do {
            desc = virtq->desc[desc_idx];

            size_t bytes_to_copy = (desc.len < sddf_buffer_len) ? desc.len : sddf_buffer_len;
            memcpy((char *) desc.addr, (char *) sddf_buffer, bytes_to_copy - bytes_written);

            bytes_written += bytes_to_copy;
        } while (bytes_written != sddf_buffer_len);

        struct virtq_used_elem used_elem;
        used_elem.id = desc_head;
        used_elem.len = bytes_written;
        virtq->used->ring[guest_idx % virtq->num] = used_elem;
        virtq->used->idx++;

        rx_queue->last_idx++;

        // 3. Inject IRQ to guest
        // @ivanv: is setting interrupt status necesary?
        dev->data.InterruptStatus = BIT_LOW(0);
        bool success = virq_inject(GUEST_VCPU_ID, dev->virq);
        assert(success);
    }

    // 4. Enqueue sDDF buffer into RX free queue
    ret = serial_enqueue_free(sddf_rx_queue, sddf_buffer, BUFFER_SIZE);
    assert(!ret);
    // @ivanv: error handle for release mode

    return -1;
}

virtio_device_funs_t functions = {
    .device_reset = virtio_console_reset,
    .get_device_features = virtio_console_get_device_features,
    .set_driver_features = virtio_console_set_driver_features,
    .get_device_config = virtio_console_get_device_config,
    .set_device_config = virtio_console_set_device_config,
    .queue_notify = virtio_console_handle_tx,
};

void virtio_console_init(struct virtio_device *dev,
                         struct virtio_queue_handler *vqs, size_t num_vqs,
                         size_t virq,
                         sddf_handler_t *sddf_handlers)
{
    // @ivanv: check that num_vqs is greater than the minimum vqs to function?
    dev->data.DeviceID = DEVICE_ID_VIRTIO_CONSOLE;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = vqs;
    dev->num_vqs = num_vqs;
    dev->virq = virq;
    dev->sddf_handlers = sddf_handlers;
}
