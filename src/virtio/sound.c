#include "sound.h"
#include "config.h"
#include "microkit.h"
#include "virtio/mmio.h"
#include "virq.h"
#include "virtio/virtq.h"
#include "util/buffer_queue.h"
#include <sddf/sound/queue.h>

#define DEBUG_SOUND

#if defined(DEBUG_SOUND)
#define LOG_SOUND(...) do{ printf("VIRTIO(SOUND): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_SOUND(...) do{}while(0)
#endif

#define LOG_SOUND_ERR(...) do{ printf("VIRTIO(SOUND)|ERROR: "); printf(__VA_ARGS__); }while(0)

#define CONTROLQ 0
#define EVENTQ 1
#define TXQ 2
#define RXQ 3
#define BUFFER_QUEUE_SIZE SOUND_NUM_BUFFERS

#define MIN(a, b) ((a) < (b) ? (a) : (b))


static struct virtio_snd_config snd_config;

typedef struct msg_handle {
    uint16_t desc_head;
    uint16_t ref_count;
    uint16_t status;
    uint16_t virtq_idx;
    // RX only
    uint32_t bytes_received;
} msg_handle_t;

/* Mapping for command ID and its virtio descriptor */
// @alexbr: currently commands and PCM frames are responded to synchronously, so
// this is a bit overkill. Could change to a simple circular queue.
typedef struct virtio_snd_msg_store {
    // Index is command ID, maps to virtio descriptor head
    msg_handle_t sent_msgs[SOUND_NUM_BUFFERS]; 
    // Index is free message ID, maps to next free command ID
    uint32_t freelist[SOUND_NUM_BUFFERS];
    uint32_t head;
    uint32_t tail;
    uint32_t num_free;
} msg_store_t;

static msg_store_t messages;
static buffer_queue_t free_buffers;
static buffer_t buffer_data[BUFFER_QUEUE_SIZE];


static sound_state_t *get_state(struct virtio_device *dev)
{
    // @alexbr: make this fit better into handlers struct
    return (sound_state_t *)dev->sddf_handlers->queue_h;
}

static void msg_store_init(msg_store_t *msg_store, unsigned int num_buffers)
{
    msg_store->head = 0;
    msg_store->tail = num_buffers - 1;
    msg_store->num_free = num_buffers;
    for (unsigned int i = 0; i < num_buffers - 1; i++) {
        msg_store->freelist[i] = i + 1;
    }
    msg_store->freelist[num_buffers - 1] = -1;
}

/** Check if the command store is full. */
static bool msg_store_full(msg_store_t *msg_store)
{
    return msg_store->num_free == 0;
}

/**
 * Allocate a command store slot for a new virtio command.
 * 
 * @param desc virtio descriptor to store in a command store slot 
 * @param id pointer to command ID allocated
 * @return 0 on success, -1 on failure
 */
static int msg_store_allocate(msg_store_t *msg_store, msg_handle_t handle, uint32_t *id)
{
    if (msg_store_full(msg_store)) {
        return -1;
    }

    // Store descriptor into head of command store
    msg_store->sent_msgs[msg_store->head] = handle;
    *id = msg_store->head;

    // Update head to next free command store slot
    msg_store->head = msg_store->freelist[msg_store->head];

    // Update number of free command store slots
    msg_store->num_free--;
    
    return 0;
}

static msg_handle_t *msg_store_get(msg_store_t *msg_store, uint32_t id)
{
    return &msg_store->sent_msgs[id];
}

/**
 * Retrieve and free a command store slot.
 * 
 * @param id command ID to be retrieved
 * @return virtio descriptor stored in slot
 */
static void msg_store_remove(msg_store_t *msg_store, uint32_t id)
{
    assert(msg_store->num_free < SOUND_NUM_BUFFERS);

    if (msg_store->num_free == 0) {
        // Head points to stale index, so restore it
        msg_store->head = id;
    }

    msg_store->freelist[msg_store->tail] = id;
    msg_store->tail = id;

    msg_store->num_free++;
}

static int msg_store_size(msg_store_t *msg_store)
{
    return SOUND_NUM_BUFFERS - msg_store->num_free;
}

static void virtio_snd_config_init(void)
{
    snd_config.jacks = 0;
    snd_config.streams = 0;
    snd_config.chmaps = 0;
}

static void fetch_buffers(struct virtio_device *dev)
{
    sound_state_t *state = get_state(dev);

    sound_pcm_t pcm;
    while (sound_dequeue_pcm(state->queues.pcm_res, &pcm) == 0) {
        buffer_queue_enqueue(&free_buffers, pcm.addr, pcm.len);
    }
}

static void virtio_snd_mmio_reset(struct virtio_device *dev)
{
    LOG_SOUND("Resetting virtIO sound device\n");

    for (int i = 0; i < VIRTIO_SND_NUM_VIRTQ; i++) {
        dev->vqs[i].ready = 0;
        dev->vqs[i].last_idx = 0;
    }
}

static int virtio_snd_mmio_get_device_features(struct virtio_device *dev, uint32_t *features)
{
    switch (dev->data.DeviceFeaturesSel) {
        case 0:
            // virtIO sound does not define any features
            *features = 0;
            break;
        case 1:
            *features = BIT_HIGH(VIRTIO_F_VERSION_1);
            break;
        default:
            LOG_SOUND_ERR("driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DeviceFeaturesSel);
            return 0;
    }
    return 1;
}

static int virtio_snd_mmio_set_driver_features(struct virtio_device *dev, uint32_t features)
{
    int success;
    switch (dev->data.DriverFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            success = (features == 0);
            break;
        // features bits 32 to 63
        case 1:
            success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
            break;
        default:
            LOG_SOUND_ERR("driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DriverFeaturesSel);
            success = 0;
    }

    if (success) {
        dev->data.features_happy = 1;
    }

    return success;
}

static int virtio_snd_mmio_get_device_config(struct virtio_device *dev, uint32_t offset, uint32_t *ret_val)
{
    // @alexbr: duplicated from block. can we abstract this?
    uintptr_t config_base_addr = (uintptr_t)&snd_config;
    uintptr_t config_field_offset = (uintptr_t)(offset - REG_VIRTIO_MMIO_CONFIG);
    uint32_t *config_field_addr = (uint32_t *)(config_base_addr + config_field_offset);
    *ret_val = *config_field_addr;

    // LOG_SOUND("get device config with base_addr 0x%x and field_address 0x%x has value %d\n", config_base_addr, config_field_addr, *ret_val);
    return 1;
}

static int virtio_snd_mmio_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t val)
{
    // @alexbr: we should be checking that the config update is valid
    // e.g., why are they allowed to change number of streams?
    LOG_SOUND_ERR("Not allowed to change sound config.");

    // uintptr_t config_base_addr = (uintptr_t)&snd_config;
    // uintptr_t config_field_offset = (uintptr_t)(offset - REG_VIRTIO_MMIO_CONFIG);
    // uint32_t *config_field_addr = (uint32_t *)(config_base_addr + config_field_offset);
    // *config_field_addr = val;
    // LOG_SOUND("set device config with base_addr 0x%x and field_address 0x%x with value %d\n", config_base_addr, config_field_addr, val);

    return 0;
}

// static const char *queue_name[] = { "controlq", "eventq", "txq", "rxq", NULL };

static const char *code_to_str(uint32_t code)
{
    switch(code)
    {
    case VIRTIO_SND_R_JACK_INFO:            return "VIRTIO_SND_R_JACK_INFO";
    case VIRTIO_SND_R_JACK_REMAP:           return "VIRTIO_SND_R_JACK_REMAP";
    case VIRTIO_SND_R_PCM_INFO:             return "VIRTIO_SND_R_PCM_INFO";
    case VIRTIO_SND_R_PCM_SET_PARAMS:       return "VIRTIO_SND_R_PCM_SET_PARAMS";
    case VIRTIO_SND_R_PCM_PREPARE:          return "VIRTIO_SND_R_PCM_PREPARE";
    case VIRTIO_SND_R_PCM_RELEASE:          return "VIRTIO_SND_R_PCM_RELEASE";
    case VIRTIO_SND_R_PCM_START:            return "VIRTIO_SND_R_PCM_START";
    case VIRTIO_SND_R_PCM_STOP:             return "VIRTIO_SND_R_PCM_STOP";
    case VIRTIO_SND_R_CHMAP_INFO:           return "VIRTIO_SND_R_CHMAP_INFO";
    case VIRTIO_SND_EVT_JACK_CONNECTED:     return "VIRTIO_SND_EVT_JACK_CONNECTED";
    case VIRTIO_SND_EVT_JACK_DISCONNECTED:  return "VIRTIO_SND_EVT_JACK_DISCONNECTED";
    case VIRTIO_SND_EVT_PCM_PERIOD_ELAPSED: return "VIRTIO_SND_EVT_PCM_PERIOD_ELAPSED";
    case VIRTIO_SND_EVT_PCM_XRUN:           return "VIRTIO_SND_EVT_PCM_XRUN";
    case VIRTIO_SOUND_S_OK:                   return "VIRTIO_SOUND_S_OK";
    case VIRTIO_SOUND_S_BAD_MSG:              return "VIRTIO_SOUND_S_BAD_MSG";
    case VIRTIO_SOUND_S_NOT_SUPP:             return "VIRTIO_SOUND_S_NOT_SUPP";
    case VIRTIO_SOUND_S_IO_ERR:               return "VIRTIO_SOUND_S_IO_ERR";
    default:
        return "<unknown>";
    }
}

static void virtio_snd_respond(struct virtio_device *dev)
{
    dev->data.InterruptStatus = BIT_LOW(0);
    bool success = virq_inject(GUEST_VCPU_ID, dev->virq);
    assert(success);
}

static inline void convert_flag(uint64_t *dest, uint64_t dest_bit, uint64_t src, uint32_t src_bit)
{
    if (src & (1 << src_bit)) {
        *dest |= (1 << dest_bit);
    }
}

static uint64_t virtio_formats_from_sddf(uint64_t formats)
{
    uint64_t result = 0;

    // @alexbr: Currently the enums are identical, but explicitly converting
    // allows us to change the enum values in the future.
    convert_flag(&result, VIRTIO_SND_PCM_FMT_IMA_ADPCM, formats, SOUND_PCM_FMT_IMA_ADPCM);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_MU_LAW,    formats, SOUND_PCM_FMT_MU_LAW);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_A_LAW,     formats, SOUND_PCM_FMT_A_LAW);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S8,        formats, SOUND_PCM_FMT_S8);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U8,        formats, SOUND_PCM_FMT_U8);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S16,       formats, SOUND_PCM_FMT_S16);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U16,       formats, SOUND_PCM_FMT_U16);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S18_3,     formats, SOUND_PCM_FMT_S18_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U18_3,     formats, SOUND_PCM_FMT_U18_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S20_3,     formats, SOUND_PCM_FMT_S20_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U20_3,     formats, SOUND_PCM_FMT_U20_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S24_3,     formats, SOUND_PCM_FMT_S24_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U24_3,     formats, SOUND_PCM_FMT_U24_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S20,       formats, SOUND_PCM_FMT_S20);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U20,       formats, SOUND_PCM_FMT_U20);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S24,       formats, SOUND_PCM_FMT_S24);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U24,       formats, SOUND_PCM_FMT_U24);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S32,       formats, SOUND_PCM_FMT_S32);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U32,       formats, SOUND_PCM_FMT_U32);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_FLOAT,     formats, SOUND_PCM_FMT_FLOAT);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_FLOAT64,   formats, SOUND_PCM_FMT_FLOAT64);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_DSD_U8,    formats, SOUND_PCM_FMT_DSD_U8);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_DSD_U16,   formats, SOUND_PCM_FMT_DSD_U16);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_DSD_U32,   formats, SOUND_PCM_FMT_DSD_U32);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_IEC958_SUBFRAME, formats, SOUND_PCM_FMT_IEC958_SUBFRAME);

    return result;
}

static uint64_t virtio_rates_from_sddf(uint64_t rates)
{
    uint64_t result = 0;

    convert_flag(&result, VIRTIO_SND_PCM_RATE_5512, rates, SOUND_PCM_RATE_5512);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_8000, rates, SOUND_PCM_RATE_8000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_11025, rates, SOUND_PCM_RATE_11025);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_16000, rates, SOUND_PCM_RATE_16000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_22050, rates, SOUND_PCM_RATE_22050);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_32000, rates, SOUND_PCM_RATE_32000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_44100, rates, SOUND_PCM_RATE_44100);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_48000, rates, SOUND_PCM_RATE_48000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_64000, rates, SOUND_PCM_RATE_64000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_88200, rates, SOUND_PCM_RATE_88200);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_96000, rates, SOUND_PCM_RATE_96000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_176400, rates, SOUND_PCM_RATE_176400);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_192000, rates, SOUND_PCM_RATE_192000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_384000, rates, SOUND_PCM_RATE_384000);

    return result;
}

static uint8_t virtio_direction_from_sddf(uint8_t direction)
{
    switch (direction) {
        case SOUND_D_INPUT: return VIRTIO_SND_D_INPUT;
        case SOUND_D_OUTPUT: return VIRTIO_SND_D_OUTPUT;
    }
    LOG_SOUND_ERR("Unknown direction %u\n", direction);
    return (uint8_t)-1;
}

static uint32_t virtio_status_from_sddf(sound_status_t status)
{
    switch (status) {
        case SOUND_S_OK: return VIRTIO_SOUND_S_OK;
        case SOUND_S_BAD_MSG: return VIRTIO_SOUND_S_BAD_MSG;
        case SOUND_S_NOT_SUPP: return VIRTIO_SOUND_S_NOT_SUPP;
        case SOUND_S_IO_ERR: return VIRTIO_SOUND_S_IO_ERR;
        case SOUND_S_BUSY: return VIRTIO_SOUND_S_IO_ERR;
    }
    return (uint32_t)-1;
}

static void get_pcm_info(struct virtio_snd_pcm_info *dest, const sound_pcm_info_t *src)
{
    dest->features = 0;
    dest->formats = virtio_formats_from_sddf(src->formats);
    dest->rates = virtio_rates_from_sddf(src->rates);
    dest->direction = virtio_direction_from_sddf(src->direction);
    dest->channels_min = src->channels_min;
    dest->channels_max = src->channels_max;
}

static int handle_pcm_info(struct virtio_device *dev,
                           struct virtq *virtq,
                           struct virtio_snd_query_info *query_info,
                           struct virtio_snd_pcm_info *responses,
                           uint32_t response_count,
                           uint32_t *bytes_written)
{
    if (response_count < query_info->count) {
        LOG_SOUND_ERR("Control message response descriptor too small (%d < %d)\n",
            response_count, query_info->count);
        return -SOUND_S_BAD_MSG;
    }

    sound_state_t *state = get_state(dev);
    sound_shared_state_t *shared_state = state->shared_state;

    memset(responses, 0, sizeof(*responses) * response_count);

    int i;
    for (i = 0; i < response_count && i < query_info->count; i++) {

        int stream = i + query_info->start_id;
        sound_pcm_info_t *pcm_info = &shared_state->stream_info[stream];

        responses[i].hdr.hda_fn_nid = stream;
        get_pcm_info(&responses[i], pcm_info);
    }

    return i;
}

static int handle_pcm_set_params(struct virtio_device *dev,
                                 uint16_t desc_head,
                                 struct virtio_snd_pcm_set_params *set_params)
{
    uint32_t id;
    msg_handle_t handle;
    handle.desc_head = desc_head;
    handle.ref_count = 1;
    handle.status = SOUND_S_OK;
    handle.virtq_idx = CONTROLQ;
    handle.bytes_received = 0;

    int err = msg_store_allocate(&messages, handle, &id);
    if (err != 0) {
        LOG_SOUND_ERR("Failed to allocate command store slot\n");
        return -SOUND_S_IO_ERR;
    }

    sound_cmd_t cmd;
    cmd.code = SOUND_CMD_TAKE;
    cmd.cookie = id;
    cmd.stream_id = set_params->hdr.stream_id;
    cmd.set_params.channels = set_params->channels;
    cmd.set_params.format = set_params->format;
    cmd.set_params.rate = set_params->rate;

    if (sound_enqueue_cmd(get_state(dev)->queues.cmd_req, &cmd) != 0) {
        LOG_SOUND_ERR("Failed to enqueue command\n");
        msg_store_remove(&messages, id);
        return -SOUND_S_IO_ERR;
    }

    return 0;
}

static int handle_basic_cmd(struct virtio_device *dev,
                            uint16_t desc_head,
                            uint32_t stream_id,
                            sound_cmd_code_t code)
{
    uint32_t id;
    msg_handle_t handle;
    handle.desc_head = desc_head;
    handle.ref_count = 1;
    handle.status = SOUND_S_OK;
    handle.virtq_idx = CONTROLQ;
    handle.bytes_received = 0;

    int err = msg_store_allocate(&messages, handle, &id);
    if (err != 0) {
        LOG_SOUND_ERR("Failed to allocate command store slot\n");
        return -SOUND_S_IO_ERR;
    }

    sound_cmd_t cmd;
    cmd.code = code;
    cmd.cookie = id;
    cmd.stream_id = stream_id;

    if (sound_enqueue_cmd(get_state(dev)->queues.cmd_req, &cmd) != 0) {
        LOG_SOUND_ERR("Failed to enqueue command\n");
        msg_store_remove(&messages, id);
        return -SOUND_S_IO_ERR;
    }

    return 0;
}

static void virtq_enqueue_used(struct virtq *virtq, uint32_t desc_head, uint32_t bytes_written)
{
    struct virtq_used_elem *used_elem = &virtq->used->ring[virtq->used->idx % virtq->num];
    used_elem->id = desc_head;
    used_elem->len = bytes_written;
    virtq->used->idx++;
}

// Returns number of bytes written to virtq
static void handle_control_msg(struct virtio_device *dev,
                               struct virtq *virtq,
                               uint16_t desc_head,
                               bool *notify_driver,
                               bool *respond)
{
    struct virtq_desc *req_desc = &virtq->desc[desc_head];
    struct virtio_snd_hdr *hdr = (void *)req_desc->addr;
    struct virtio_snd_pcm_hdr *pcm_hdr = (void *)hdr;
    
    bool immediate = false;

    uint32_t bytes_written = 0;

    if ((req_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
        LOG_SOUND_ERR("Control message missing status descriptor\n");
        return;
    }

    struct virtq_desc *status_desc = &virtq->desc[req_desc->next];
    uint32_t *status_ptr = (void *)status_desc->addr;

    int result;

    switch (hdr->code) {
    case VIRTIO_SND_R_PCM_INFO: {

        if ((status_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
            LOG_SOUND_ERR("Control message missing response descriptor\n");
            result = -VIRTIO_SOUND_S_BAD_MSG;
            break;
        }

        struct virtq_desc *response_desc = &virtq->desc[status_desc->next];
        result = handle_pcm_info(dev, virtq,
                                 (void *)hdr,
                                 (void *)response_desc->addr,
                                 response_desc->len / sizeof(struct virtio_snd_pcm_info),
                                 &bytes_written);
        if (result >= 0) {
            bytes_written += result * sizeof(struct virtio_snd_pcm_info);
        }
        immediate = true;
        break;
    }
    case VIRTIO_SND_R_PCM_SET_PARAMS:
        result = handle_pcm_set_params(dev, desc_head, (void *)hdr);
        break;
    case VIRTIO_SND_R_PCM_PREPARE:
        result = handle_basic_cmd(dev, desc_head, pcm_hdr->stream_id, SOUND_CMD_PREPARE);
        break;
    case VIRTIO_SND_R_PCM_RELEASE:
        result = handle_basic_cmd(dev, desc_head, pcm_hdr->stream_id, SOUND_CMD_RELEASE);
        break;
    case VIRTIO_SND_R_PCM_START:
        result = handle_basic_cmd(dev, desc_head, pcm_hdr->stream_id, SOUND_CMD_START);
        break;
    case VIRTIO_SND_R_PCM_STOP:
        result = handle_basic_cmd(dev, desc_head, pcm_hdr->stream_id, SOUND_CMD_STOP);
        break;
    case VIRTIO_SND_R_JACK_INFO:
    case VIRTIO_SND_R_JACK_REMAP:
    case VIRTIO_SND_R_CHMAP_INFO:
        LOG_SOUND_ERR("Control message not implemented: %s\n", code_to_str(hdr->code));
        result = -VIRTIO_SOUND_S_NOT_SUPP;
        break;
    default:
        LOG_SOUND_ERR("Unknown control message %s\n", code_to_str(hdr->code));
        result = -VIRTIO_SOUND_S_BAD_MSG;
        break;
    }

    uint32_t status = VIRTIO_SOUND_S_OK;
    if (result < 0) {
        LOG_SOUND("Command failed with result %d\n", -result);
        immediate = true;
        status = -result;
    }

    if (immediate) {
        *status_ptr = status;
        bytes_written += sizeof(uint32_t);
        virtq_enqueue_used(virtq, desc_head, bytes_written);
    } else {
        *notify_driver = true;
        assert(bytes_written == 0);
    }
    *respond = immediate;
}

static bool perform_xfer(struct virtq *virtq,
                         struct virtq_desc *desc,
                         sound_pcm_queue_t *req_ring,
                         bool transmit,
                         int stream_id,
                         int msg_id,
                         int *sent)
{
    buffer_t pcm_buffer;
    if (!buffer_queue_dequeue(&free_buffers, &pcm_buffer)) {
        LOG_SOUND_ERR("No free buffers\n");
        return false;
    }

    uint32_t pcm_transmitted = 0;
    uint32_t pcm_remaining = SOUND_PCM_BUFFER_SIZE;

    // Decompose descriptor chain into one or more sDDF requests.
    for (;
         desc->flags & VIRTQ_DESC_F_NEXT;
         desc = &virtq->desc[desc->next])
    {
        if (!!(desc->flags & VIRTQ_DESC_F_WRITE) == transmit) {
            LOG_SOUND_ERR("Incorrect xfer buffer type\n");
            return false;
        }

        uint32_t desc_transmitted = 0;
        uint32_t desc_remaining = desc->len;

        while (desc_remaining > 0) {

            int to_xfer = MIN(desc_remaining, pcm_remaining);

            if (transmit) {
                memcpy((void *)pcm_buffer.addr + pcm_transmitted, (void *)desc->addr + desc_transmitted, to_xfer);
            }
            desc_transmitted += to_xfer;
            desc_remaining -= to_xfer;

            pcm_transmitted += to_xfer;
            pcm_remaining -= to_xfer;

            // If current to-be-sent request is full, send it.
            if (pcm_remaining == 0) {

                sound_pcm_t pcm;
                pcm.addr = pcm_buffer.addr;
                pcm.len = pcm_transmitted;
                pcm.stream_id = stream_id;
                pcm.cookie = msg_id;
                pcm.status = 0;
                pcm.latency_bytes = 0;

                if (sound_enqueue_pcm(req_ring, &pcm) != 0) {
                    LOG_SOUND_ERR("Failed to enqueue to pcm request\n");
                    return false;
                }

                (*sent)++;

                if (!buffer_queue_dequeue(&free_buffers, &pcm_buffer)) {
                    LOG_SOUND_ERR("Failed to dequeue free buffer\n");
                    return false;
                }

                pcm_remaining = SOUND_PCM_BUFFER_SIZE;
                pcm_transmitted = 0;
            }
        }
    }

    // Transmit remaining PCM data.
    if (pcm_transmitted > 0) {
        sound_pcm_t pcm;
        pcm.addr = pcm_buffer.addr;
        pcm.len = pcm_transmitted;
        pcm.stream_id = stream_id;
        pcm.cookie = msg_id;
        pcm.status = 0;
        pcm.latency_bytes = 0;

        if (sound_enqueue_pcm(req_ring, &pcm) != 0) {
            LOG_SOUND_ERR("Failed to enqueue to tx used\n");
            return false;
        }
        (*sent)++;
    }
    return true;
}

static void handle_xfer(struct virtio_device *dev,
                        struct virtq *virtq,
                        uint16_t desc_head,
                        bool transmit,
                        bool *notify_driver, bool *respond)
{
    struct virtq_desc *req_desc = &virtq->desc[desc_head];
    struct virtio_snd_pcm_xfer *hdr = (void *)req_desc->addr;
    sound_pcm_queue_t *req_ring = get_state(dev)->queues.pcm_req;

    if ((req_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
        LOG_SOUND_ERR("XFER message missing data\n");
        return;
    }

    uint32_t msg_id;
    msg_handle_t handle;
    handle.desc_head = desc_head;
    handle.ref_count = 0;
    handle.status = SOUND_S_OK;
    handle.virtq_idx = transmit ? TXQ : RXQ;
    handle.bytes_received = 0;

    int err = msg_store_allocate(&messages, handle, &msg_id);
    if (err != 0) {
        LOG_SOUND_ERR("Failed to allocate command store slot\n");
        return;
    }

    msg_handle_t *msg = msg_store_get(&messages, msg_id);

    struct virtq_desc *desc = &virtq->desc[req_desc->next];
    int sent = 0;
    bool success = perform_xfer(virtq, desc, req_ring, transmit,
                                hdr->stream_id, msg_id, &sent);

    if (sent == 0) {
        for (;
            desc->flags & VIRTQ_DESC_F_NEXT;
            desc = &virtq->desc[desc->next]);
            
        assert(desc->flags & VIRTQ_DESC_F_WRITE);

        uint32_t *status_ptr = (void *)desc->addr;
        *status_ptr = VIRTIO_SOUND_S_IO_ERR;

        virtq_enqueue_used(virtq, desc_head, sizeof(uint32_t));

        msg_store_remove(&messages, msg_id);

        *respond = true;
        return;
    } else {
        assert(sent > 0);
        *notify_driver = true;
    }

    msg->ref_count = sent;

    if (!success) {
        msg->status = VIRTIO_SOUND_S_IO_ERR;
    }
}

static void handle_virtq(struct virtio_device *dev,
                         int index, bool *notify_driver, bool *respond)
{
    virtio_queue_handler_t *vq = &dev->vqs[index];
    struct virtq *virtq = &vq->virtq;

    uint16_t idx = vq->last_idx;
    for (; idx != virtq->avail->idx; idx++) {

        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

        switch (index) {
        case CONTROLQ:
            handle_control_msg(dev, virtq, desc_head, notify_driver, respond);
            break;
        case TXQ:
            handle_xfer(dev, virtq, desc_head, true, notify_driver, respond);
            break;
        case RXQ:
            handle_xfer(dev, virtq, desc_head, false, notify_driver, respond);
            break;
        default:
            LOG_SOUND("Queue %d not implemented", index);
        }
    }
    vq->last_idx = idx;
}

static int virtio_snd_mmio_queue_notify(struct virtio_device *dev)
{
    if (dev->data.QueueSel > VIRTIO_SND_NUM_VIRTQ) {
        // @alexbr: handle error appropriately
        LOG_SOUND_ERR("Invalid queue\n");
        return 0;
    }

    bool notify_driver = false;
    bool respond = false;

    handle_virtq(dev, dev->data.QueueNotify, &notify_driver, &respond);

    if (notify_driver) {
        microkit_notify(dev->sddf_handlers->ch);
    }
    if (respond) {
        virtio_snd_respond(dev);
    }

    return 1;
}

static virtio_device_funs_t functions = {
    .device_reset = virtio_snd_mmio_reset,
    .get_device_features = virtio_snd_mmio_get_device_features,
    .set_driver_features = virtio_snd_mmio_set_driver_features,
    .get_device_config = virtio_snd_mmio_get_device_config,
    .set_device_config = virtio_snd_mmio_set_device_config,
    .queue_notify = virtio_snd_mmio_queue_notify,
};

void virtio_snd_init(struct virtio_device *dev,
                    struct virtio_queue_handler *vqs, size_t num_vqs,
                    size_t virq,
                    sddf_handler_t *sddf_handlers)
{
    dev->data.DeviceID = DEVICE_ID_VIRTIO_SOUND;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = vqs;
    dev->num_vqs = num_vqs;
    dev->virq = virq;
    dev->sddf_handlers = sddf_handlers;
    
    virtio_snd_config_init();
    msg_store_init(&messages, SOUND_NUM_BUFFERS);

    buffer_queue_create(&free_buffers, buffer_data, BUFFER_QUEUE_SIZE);
    fetch_buffers(dev);
}

static unsigned copy_rx_data(struct virtq *virtq,
                         struct virtq_desc *desc,
                         msg_handle_t *msg,
                         void *pcm, unsigned pcm_len)
{
    uint32_t desc_position = 0;

    for (;
        desc->flags & VIRTQ_DESC_F_NEXT;
        desc = &virtq->desc[desc->next])
    {
        if ((desc->flags & VIRTQ_DESC_F_WRITE) == 0) {
            LOG_SOUND_ERR("Expected VIRTQ_DESC_F_WRITE on RX buffer\n");
            msg->status = SOUND_S_BAD_MSG;
            continue;
        }

        if (pcm_len == 0) {
            break;
        }

        if (desc_position + desc->len > msg->bytes_received) {
            assert(desc_position <= msg->bytes_received);

            uint32_t offset = msg->bytes_received - desc_position;
            uint32_t to_write = MIN(pcm_len, desc->len - offset);

            memcpy((void *)desc->addr + offset, pcm, to_write);
            pcm += to_write;
            msg->bytes_received += to_write;
            pcm_len -= to_write;
        }
        
        desc_position += desc->len;
    }

    // Sanity check when RX request is complete.
    if (msg->ref_count == 0) {
        if (pcm_len != 0) {
            LOG_SOUND_ERR("Received too much PCM for RX\n");
            msg->status = SOUND_S_BAD_MSG;
        }
        if (desc_position != msg->bytes_received) {
            LOG_SOUND_ERR(
                "Did not receive enough PCM for RX: desc_position %u, bytes_received %u\n",
                desc_position, msg->bytes_received);

            msg->status = SOUND_S_BAD_MSG;
        }
        if (desc->flags & VIRTQ_DESC_F_NEXT) {
            LOG_SOUND_ERR("Desc not fully advanced\n");
            msg->status = SOUND_S_BAD_MSG;
        }
        if ((desc->flags & VIRTQ_DESC_F_WRITE) == 0) {
            LOG_SOUND_ERR("Expected VIRTQ_DESC_F_WRITE on status buffer\n");
            msg->status = SOUND_S_BAD_MSG;
        }
    }
    return desc_position;
}

static bool respond_to_message(msg_handle_t *msg,
                               struct virtio_device *dev,
                               void *pcm, unsigned pcm_len,
                               void *response, unsigned response_len)
{
    if (msg->ref_count == 0) {
        LOG_SOUND_ERR("Message ref count is 0 (too many responses)\n");
        return false;
    }

    uint16_t desc_head = msg->desc_head;

    assert(msg->virtq_idx < VIRTIO_SND_NUM_VIRTQ);
    struct virtq *virtq = &dev->vqs[msg->virtq_idx].virtq;

    struct virtq_desc *req_desc = &virtq->desc[desc_head];
    struct virtq_desc *res_desc = &virtq->desc[req_desc->next];

    unsigned used;
    if (msg->virtq_idx == RXQ) {
        used = copy_rx_data(virtq, res_desc, msg, pcm, pcm_len);
    } else {
        used = 0;
    }

    if (--msg->ref_count > 0)
        return false;

    struct virtq_desc *status_desc = res_desc;
    for (;
        status_desc->flags & VIRTQ_DESC_F_NEXT;
        status_desc = &virtq->desc[status_desc->next]);
    
    if (status_desc == req_desc || (status_desc->flags & VIRTQ_DESC_F_WRITE) == 0) {
        LOG_SOUND_ERR("Message must contain writeable status descriptor\n");
    } else {
        memcpy((void *)status_desc->addr, response, response_len);
        used += response_len;
    }
    
    virtq_enqueue_used(virtq, desc_head, used);

    return true;
}

void virtio_snd_notified(struct virtio_device *dev)
{
    sound_state_t *state = get_state(dev);
    bool respond = false;

    // LOG_SOUND("virtIO sound device notified by server\n");

    snd_config.streams = state->shared_state->streams;

    sound_cmd_t cmd;
    while (sound_dequeue_cmd(state->queues.cmd_res, &cmd) == 0) {

        msg_handle_t *msg = msg_store_get(&messages, cmd.cookie);
        if (cmd.status != SOUND_S_OK) {
            msg->status = cmd.status;
        }
        
        uint32_t response = virtio_status_from_sddf(msg->status);

        bool responded = respond_to_message(msg, dev, NULL, 0,
                                            &response, sizeof(response));
        if (responded) {
            msg_store_remove(&messages, cmd.cookie);
            respond = true;
        }
    }

    sound_pcm_t pcm;
    while (sound_dequeue_pcm(state->queues.pcm_res, &pcm) == 0) {

        msg_handle_t *msg = msg_store_get(&messages, pcm.cookie);
        if (pcm.status != SOUND_S_OK) {
            msg->status = pcm.status;
        }

        struct virtio_snd_pcm_status response;
        response.status = virtio_status_from_sddf(msg->status);
        response.latency_bytes = pcm.latency_bytes;

        bool responded = respond_to_message(msg, dev,
                                            (void *)pcm.addr, pcm.len,
                                            &response, sizeof(response));
        if (responded) {
            msg_store_remove(&messages, pcm.cookie);
            respond = true;
        }

        buffer_queue_enqueue(&free_buffers, pcm.addr, pcm.len);
    }

    if (respond) {
        virtio_snd_respond(dev);
    }
}
