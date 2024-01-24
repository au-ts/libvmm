#include "sound.h"
#include "config.h"
#include "microkit.h"
#include "virtio/mmio.h"
#include "virq.h"
#include "sound/libsoundsharedringbuffer/include/sddf_snd_shared_ringbuffer.h"
#include "virtio/virtq.h"
#include "util/buffer_queue.h"

#define DEBUG_SOUND

#if defined(DEBUG_SOUND)
#define LOG_SOUND(...) do{ printf("VIRTIO(SOUND): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_SOUND(...) do{}while(0)
#endif

#define LOG_SOUND_ERR(...) do{ printf("VIRTIO(SOUND)|ERROR: "); printf(__VA_ARGS__); }while(0)

// @ivanv: put in util or remove
#define BIT_LOW(n)  (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32))

#define CONTROLQ 0
#define EVENTQ 1
#define TXQ 2
#define RXQ 3

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static struct virtio_snd_config snd_config;

typedef struct msg_handle {
    uint16_t desc_head;
    uint16_t ref_count;
    uint16_t status;
    uint16_t replied;
    // RX only
    uint32_t bytes_received;
} msg_handle_t;

/* Mapping for command ID and its virtio descriptor */
// @alexbr: currently commands and PCM frames are responded to synchronously, so
// this is a bit overkill. Could change to a simple circular queue.
typedef struct virtio_snd_msg_store {
    // Index is command ID, maps to virtio descriptor head
    msg_handle_t sent_msgs[SDDF_SND_NUM_BUFFERS]; 
    // Index is free message ID, maps to next free command ID
    uint32_t freelist[SDDF_SND_NUM_BUFFERS];
    uint32_t head;
    uint32_t tail;
    uint32_t num_free;
} virtio_snd_msg_store_t;

static virtio_snd_msg_store_t messages;

#define BUFFER_QUEUE_SIZE SDDF_SND_NUM_BUFFERS

static buffer_queue_t tx_buffers;
static buffer_queue_t rx_buffers;

static buffer_t tx_buffer_data[BUFFER_QUEUE_SIZE];
static buffer_t rx_buffer_data[BUFFER_QUEUE_SIZE];

static sddf_snd_state_t *get_state(struct virtio_device *dev)
{
    // @alexbr: currently this casts from a void ** which doesn't make sense.
    // I prefer to have a struct instead of array however, need to discuss.
    return (sddf_snd_state_t *)dev->sddf_ring_handles;
}

static void virtio_snd_msg_store_init(virtio_snd_msg_store_t *msg_store, unsigned int num_buffers)
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
static bool virtio_snd_msg_store_full(virtio_snd_msg_store_t *msg_store)
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
static int virtio_snd_msg_store_allocate(virtio_snd_msg_store_t *msg_store, msg_handle_t handle, uint32_t *id)
{
    if (virtio_snd_msg_store_full(msg_store)) {
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

static msg_handle_t *virtio_snd_msg_store_get(virtio_snd_msg_store_t *msg_store, uint32_t id)
{
    return &msg_store->sent_msgs[id];
}

/**
 * Retrieve and free a command store slot.
 * 
 * @param id command ID to be retrieved
 * @return virtio descriptor stored in slot
 */
static void virtio_snd_msg_store_remove(virtio_snd_msg_store_t *msg_store, uint32_t id)
{
    assert(msg_store->num_free < SDDF_SND_NUM_BUFFERS);

    if (msg_store->num_free == 0) {
        // Head points to stale index, so restore it
        msg_store->head = id;
    }

    msg_store->freelist[msg_store->tail] = id;
    msg_store->tail = id;

    msg_store->num_free++;
}

static void virtio_snd_config_init(void)
{
    snd_config.jacks = 0;
    snd_config.streams = 0;
    snd_config.chmaps = 0;
}

static void fetch_buffers(struct virtio_device *dev)
{
    sddf_snd_state_t *state = get_state(dev);

    LOG_SOUND("Fetching buffers\n");

    sddf_snd_pcm_data_t pcm;
    while (sddf_snd_dequeue_pcm_data(state->rings.tx_res, &pcm) == 0) {
        buffer_queue_enqueue(&tx_buffers, pcm.addr, pcm.len);
    }
    while (sddf_snd_dequeue_pcm_data(state->rings.rx_res, &pcm) == 0) {
        buffer_queue_enqueue(&rx_buffers, pcm.addr, pcm.len);
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

static const char *queue_name[] = { "controlq", "eventq", "txq", "rxq", NULL };

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
    case VIRTIO_SND_S_OK:                   return "VIRTIO_SND_S_OK";
    case VIRTIO_SND_S_BAD_MSG:              return "VIRTIO_SND_S_BAD_MSG";
    case VIRTIO_SND_S_NOT_SUPP:             return "VIRTIO_SND_S_NOT_SUPP";
    case VIRTIO_SND_S_IO_ERR:               return "VIRTIO_SND_S_IO_ERR";
    default:
        return "<unknown>";
    }
}

static void debug_device_info(virtio_device_info_t *info)
{
    LOG_SOUND("--- Sound device info ---\n");
    LOG_SOUND("\tDeviceID %#x\n", info->DeviceID);
    LOG_SOUND("\tVendorID %#x\n", info->VendorID);
    LOG_SOUND("\tDeviceFeaturesSel %#x\n", info->DeviceFeaturesSel);
    LOG_SOUND("\tDriverFeaturesSel %#x\n", info->DriverFeaturesSel);
    LOG_SOUND("\tfeatures_happy %i\n", info->features_happy);
    LOG_SOUND("\tQueueSel %#x\n", info->QueueSel);
    LOG_SOUND("\tQueueNotify %#x\n", info->QueueNotify);
    LOG_SOUND("\tInterruptStatus %#x\n", info->InterruptStatus);
    LOG_SOUND("\tStatus %#x\n", info->InterruptStatus);
    LOG_SOUND("-------------------------\n");
}

static void debug_virq_contents(struct virtio_device *dev)
{
    LOG_SOUND("--- Virtq Contents ---\n");
    for (int i = 0; i < VIRTIO_SND_NUM_VIRTQ; i++) {
        virtio_queue_handler_t *vq = &dev->vqs[i];
        struct virtq *virtq = &vq->virtq;

        uint16_t idx = vq->last_idx;

        for (; idx != virtq->avail->idx; idx++) {
            uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

            uint16_t curr_desc_head = desc_head;

            if (i == CONTROLQ) {
                struct virtio_snd_hdr *virtio_cmd = (void *)virtq->desc[curr_desc_head].addr;
                LOG_SOUND("\t[%s] %s\n", queue_name[i], code_to_str(virtio_cmd->code));
            }
            else if (i == TXQ) {
                LOG_SOUND("\t[%s] \n", queue_name[i]);
                struct virtq_desc *desc = &virtq->desc[desc_head];
                uint32_t flags;
                do {
                    const char *write = desc->flags & VIRTQ_DESC_F_WRITE ? "write" : "read";
                    LOG_SOUND("\t\t%s %d [%#x, %#x) ->\n",
                            write, desc->len, desc->addr, desc->addr + desc->len);

                    flags = desc->flags; 
                    desc = &virtq->desc[desc->next];
                }
                while (flags & VIRTQ_DESC_F_NEXT);
                LOG_SOUND("\t\tNULL\n");
            }
        }
    }
    LOG_SOUND("-------------------------\n");
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
    convert_flag(&result, VIRTIO_SND_PCM_FMT_IMA_ADPCM, formats, SDDF_SND_PCM_FMT_IMA_ADPCM);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_MU_LAW,    formats, SDDF_SND_PCM_FMT_MU_LAW);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_A_LAW,     formats, SDDF_SND_PCM_FMT_A_LAW);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S8,        formats, SDDF_SND_PCM_FMT_S8);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U8,        formats, SDDF_SND_PCM_FMT_U8);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S16,       formats, SDDF_SND_PCM_FMT_S16);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U16,       formats, SDDF_SND_PCM_FMT_U16);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S18_3,     formats, SDDF_SND_PCM_FMT_S18_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U18_3,     formats, SDDF_SND_PCM_FMT_U18_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S20_3,     formats, SDDF_SND_PCM_FMT_S20_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U20_3,     formats, SDDF_SND_PCM_FMT_U20_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S24_3,     formats, SDDF_SND_PCM_FMT_S24_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U24_3,     formats, SDDF_SND_PCM_FMT_U24_3);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S20,       formats, SDDF_SND_PCM_FMT_S20);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U20,       formats, SDDF_SND_PCM_FMT_U20);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S24,       formats, SDDF_SND_PCM_FMT_S24);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U24,       formats, SDDF_SND_PCM_FMT_U24);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_S32,       formats, SDDF_SND_PCM_FMT_S32);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_U32,       formats, SDDF_SND_PCM_FMT_U32);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_FLOAT,     formats, SDDF_SND_PCM_FMT_FLOAT);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_FLOAT64,   formats, SDDF_SND_PCM_FMT_FLOAT64);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_DSD_U8,    formats, SDDF_SND_PCM_FMT_DSD_U8);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_DSD_U16,   formats, SDDF_SND_PCM_FMT_DSD_U16);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_DSD_U32,   formats, SDDF_SND_PCM_FMT_DSD_U32);
    convert_flag(&result, VIRTIO_SND_PCM_FMT_IEC958_SUBFRAME, formats, SDDF_SND_PCM_FMT_IEC958_SUBFRAME);

    return result;
}

static uint64_t virtio_rates_from_sddf(uint64_t rates)
{
    uint64_t result = 0;

    convert_flag(&result, VIRTIO_SND_PCM_RATE_5512, rates, SDDF_SND_PCM_RATE_5512);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_8000, rates, SDDF_SND_PCM_RATE_8000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_11025, rates, SDDF_SND_PCM_RATE_11025);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_16000, rates, SDDF_SND_PCM_RATE_16000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_22050, rates, SDDF_SND_PCM_RATE_22050);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_32000, rates, SDDF_SND_PCM_RATE_32000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_44100, rates, SDDF_SND_PCM_RATE_44100);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_48000, rates, SDDF_SND_PCM_RATE_48000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_64000, rates, SDDF_SND_PCM_RATE_64000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_88200, rates, SDDF_SND_PCM_RATE_88200);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_96000, rates, SDDF_SND_PCM_RATE_96000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_176400, rates, SDDF_SND_PCM_RATE_176400);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_192000, rates, SDDF_SND_PCM_RATE_192000);
    convert_flag(&result, VIRTIO_SND_PCM_RATE_384000, rates, SDDF_SND_PCM_RATE_384000);

    return result;
}

static uint8_t virtio_direction_from_sddf(uint8_t direction)
{
    switch (direction) {
        case SDDF_SND_D_INPUT: return VIRTIO_SND_D_INPUT;
        case SDDF_SND_D_OUTPUT: return VIRTIO_SND_D_OUTPUT;
    }
    LOG_SOUND_ERR("Unknown direction %u\n", direction);
    return (uint8_t)-1;
}

static uint32_t virtio_status_from_sddf(sddf_snd_status_code_t status)
{
    switch (status) {
        case SDDF_SND_S_OK: return VIRTIO_SND_S_OK;
        case SDDF_SND_S_BAD_MSG: return VIRTIO_SND_S_BAD_MSG;
        case SDDF_SND_S_NOT_SUPP: return VIRTIO_SND_S_NOT_SUPP;
        case SDDF_SND_S_IO_ERR: return VIRTIO_SND_S_IO_ERR;
    }
    return (uint32_t)-1;
}

static void get_pcm_info(struct virtio_snd_pcm_info *dest, const sddf_snd_pcm_info_t *src)
{
    dest->features = 0;
    dest->formats = virtio_formats_from_sddf(src->formats);
    dest->rates = virtio_rates_from_sddf(src->rates);
    dest->direction = virtio_direction_from_sddf(src->direction);
    dest->channels_min = src->channels_min;
    dest->channels_max = src->channels_max;
}

static void debug_pcm_info(struct virtio_snd_pcm_info *info)
{
    LOG_SOUND("--- PCM Info ---\n");
    LOG_SOUND("\tfeatures    \t%x\n", info->features);
    LOG_SOUND("\tformats     \t%lx\n", info->formats);
    LOG_SOUND("\trates       \t%lx\n", info->rates);
    LOG_SOUND("\tdirection   \t%x\n", info->direction);
    LOG_SOUND("\tchannels_min\t%d\n", info->channels_min);
    LOG_SOUND("\tchannels_max\t%d\n", info->channels_max);
    LOG_SOUND("----------------\n");
}

static uint32_t handle_pcm_info(struct virtio_device *dev,
                                struct virtq *virtq,
                                struct virtio_snd_query_info *query_info,
                                struct virtio_snd_pcm_info *responses,
                                uint32_t response_count,
                                uint32_t *bytes_written)
{
    uint32_t status = VIRTIO_SND_S_OK;

    if (response_count < query_info->count) {
        // If too small, fill up what we can
        LOG_SOUND_ERR("Control message response descriptor too small\n");
        LOG_SOUND_ERR("res count %d, query count %d\n", response_count, query_info->count);
        status = VIRTIO_SND_S_BAD_MSG;
    }

    sddf_snd_state_t *state = get_state(dev);
    sddf_snd_shared_state_t *shared_state = state->shared_state;

    memset(responses, 0, sizeof(*responses) * response_count);

    int i;
    for (i = 0; i < response_count && i < query_info->count; i++) {

        int stream = i + query_info->start_id;
        sddf_snd_pcm_info_t *pcm_info = &shared_state->stream_info[stream];

        responses[i].hdr.hda_fn_nid = stream;
        get_pcm_info(&responses[i], pcm_info);
        debug_pcm_info(&responses[i]);
    }

    *bytes_written += i * sizeof(*responses);
    return status;
}

static int handle_pcm_set_params(struct virtio_device *dev,
                                 uint16_t desc_head,
                                 struct virtio_snd_pcm_set_params *set_params,
                                 bool *is_async)
{
    uint32_t id;
    msg_handle_t handle;
    handle.desc_head = desc_head;
    handle.ref_count = 1;
    handle.status = SDDF_SND_S_OK;
    handle.replied = 0;
    handle.bytes_received = 0;

    int err = virtio_snd_msg_store_allocate(&messages, handle, &id);
    if (err != 0) {
        LOG_SOUND_ERR("Failed to allocate command store slot\n");
        return -1;
    }

    sddf_snd_command_t cmd;
    cmd.code = SDDF_SND_CMD_PCM_SET_PARAMS;
    cmd.cookie = id;
    cmd.stream_id = set_params->hdr.stream_id;
    cmd.set_params.buffer_bytes = set_params->buffer_bytes;
    cmd.set_params.period_bytes = set_params->period_bytes;
    cmd.set_params.channels = set_params->channels;
    cmd.set_params.format = set_params->format;
    cmd.set_params.rate = set_params->rate;

    if (sddf_snd_enqueue_cmd(get_state(dev)->rings.commands, &cmd) != 0) {
        LOG_SOUND_ERR("Failed to enqueue command\n");
        virtio_snd_msg_store_remove(&messages, id);
        return -1;
    }

    *is_async = true;
    return 0;
}

static int handle_basic_cmd(struct virtio_device *dev,
                            uint16_t desc_head,
                            uint32_t stream_id,
                            sddf_snd_command_code_t code,
                            bool *is_async)
{
    uint32_t id;
    msg_handle_t handle;
    handle.desc_head = desc_head;
    handle.ref_count = 1;
    handle.status = SDDF_SND_S_OK;
    handle.replied = 0;
    handle.bytes_received = 0;

    int err = virtio_snd_msg_store_allocate(&messages, handle, &id);
    if (err != 0) {
        LOG_SOUND_ERR("Failed to allocate command store slot\n");
        return -1;
    }

    sddf_snd_command_t cmd;
    cmd.code = code;
    cmd.cookie = id;
    cmd.stream_id = stream_id;

    if (sddf_snd_enqueue_cmd(get_state(dev)->rings.commands, &cmd) != 0) {
        LOG_SOUND_ERR("Failed to enqueue command\n");
        virtio_snd_msg_store_remove(&messages, id);
        return -1;
    }

    *is_async = true;
    return 0;
}

static void write_error(uint32_t *status_ptr, uint32_t *bytes_written, uint32_t err)
{
    *status_ptr = err;
    *bytes_written += sizeof(uint32_t);
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
    
    bool is_async = false;

    uint32_t bytes_written = 0;

    if ((req_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
        LOG_SOUND_ERR("Control message missing status descriptor\n");
        return;
    }

    struct virtq_desc *status_desc = &virtq->desc[req_desc->next];
    uint32_t *status_ptr = (void *)status_desc->addr;

    LOG_SOUND("Got command %s\n", code_to_str(hdr->code));

    switch (hdr->code) {
    case VIRTIO_SND_R_PCM_INFO: {

        if ((status_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
            LOG_SOUND_ERR("Control message missing response descriptor\n");
            write_error(status_ptr, &bytes_written, VIRTIO_SND_S_BAD_MSG);
            break;
        }

        struct virtq_desc *response_desc = &virtq->desc[status_desc->next];
        *status_ptr = handle_pcm_info(dev, virtq,
                                      (void *)hdr,
                                      (void *)response_desc->addr,
                                      response_desc->len / sizeof(struct virtio_snd_pcm_info),
                                      &bytes_written);
        bytes_written += sizeof(uint32_t);
        break;
    }
    case VIRTIO_SND_R_PCM_SET_PARAMS: {
        int err = handle_pcm_set_params(dev, desc_head, (void *)hdr, &is_async);
        if (err)
            write_error(status_ptr, &bytes_written, VIRTIO_SND_S_IO_ERR);
        break;
    }
    case VIRTIO_SND_R_PCM_PREPARE: {
        struct virtio_snd_pcm_hdr *pcm_hdr = (void *)hdr;
        int err = handle_basic_cmd(dev, desc_head, pcm_hdr->stream_id, SDDF_SND_CMD_PCM_PREPARE, &is_async);
        if (err)
            write_error(status_ptr, &bytes_written, VIRTIO_SND_S_IO_ERR);
        break;
    }
    case VIRTIO_SND_R_PCM_RELEASE: {
        struct virtio_snd_pcm_hdr *pcm_hdr = (void *)hdr;
        int err = handle_basic_cmd(dev, desc_head, pcm_hdr->stream_id, SDDF_SND_CMD_PCM_RELEASE, &is_async);
        if (err)
            write_error(status_ptr, &bytes_written, VIRTIO_SND_S_IO_ERR);
        break;
    }
    case VIRTIO_SND_R_PCM_START: {
        struct virtio_snd_pcm_hdr *pcm_hdr = (void *)hdr;
        int err = handle_basic_cmd(dev, desc_head, pcm_hdr->stream_id, SDDF_SND_CMD_PCM_START, &is_async);
        if (err)
            write_error(status_ptr, &bytes_written, VIRTIO_SND_S_IO_ERR);
        break;
    }
    case VIRTIO_SND_R_PCM_STOP: {
        struct virtio_snd_pcm_hdr *pcm_hdr = (void *)hdr;
        int err = handle_basic_cmd(dev, desc_head, pcm_hdr->stream_id, SDDF_SND_CMD_PCM_STOP, &is_async);
        if (err)
            write_error(status_ptr, &bytes_written, VIRTIO_SND_S_IO_ERR);
        break;
    }
    case VIRTIO_SND_R_JACK_INFO:
    case VIRTIO_SND_R_JACK_REMAP:
    case VIRTIO_SND_R_CHMAP_INFO:
        LOG_SOUND_ERR("Control message not implemented: %s\n", code_to_str(hdr->code));
        *status_ptr = VIRTIO_SND_S_NOT_SUPP;
        bytes_written += sizeof(uint32_t);
        break;
    default:
        LOG_SOUND_ERR("Unknown control message %s\n", code_to_str(hdr->code));
        *status_ptr = VIRTIO_SND_S_BAD_MSG;
        bytes_written += sizeof(uint32_t);
        break;
    }

    if (is_async) {
        *notify_driver = true;
        *respond = false;
        assert(bytes_written == 0);
    } else {
        struct virtq_used_elem *used_elem = &virtq->used->ring[virtq->used->idx % virtq->num];
        used_elem->id = desc_head;
        used_elem->len = bytes_written;
        virtq->used->idx++;

        *respond = true;
    }
}

static void print_bytes(void *data)
{
    for (int i = 0; i < 16; i++) {
        printf("%02x ", ((uint8_t *)data)[i]);
    }
    printf("\n");
}

static void handle_xfer(struct virtio_device *dev,
                        struct virtq *virtq,
                        uint16_t desc_head,
                        sddf_snd_pcm_data_ring_t *req_ring,
                        buffer_queue_t *free_buffers,
                        bool transmit,
                        bool *notify_driver,
                        bool *respond)
{
    struct virtq_desc *req_desc = &virtq->desc[desc_head];
    struct virtio_snd_pcm_xfer *hdr = (void *)req_desc->addr;

    if ((req_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
        LOG_SOUND_ERR("XFER message missing data\n");
        return;
    }

    uint32_t msg_id;
    msg_handle_t handle;
    handle.desc_head = desc_head;
    handle.ref_count = 0;
    handle.status = SDDF_SND_S_OK;
    handle.replied = 0;
    handle.bytes_received = 0;

    int err = virtio_snd_msg_store_allocate(&messages, handle, &msg_id);
    if (err != 0) {
        LOG_SOUND_ERR("Failed to allocate command store slot\n");
        return;
    }

    msg_handle_t *msg = virtio_snd_msg_store_get(&messages, msg_id);

    buffer_t pcm_buffer;
    if (!buffer_queue_dequeue(free_buffers, &pcm_buffer)) {
        LOG_SOUND_ERR("No free buffers\n");
        virtio_snd_msg_store_remove(&messages, msg_id);
        return;
    }

    uint32_t pcm_transmitted = 0;
    uint32_t pcm_remaining = SDDF_SND_PCM_BUFFER_SIZE;

    // LOG_SOUND("XFER desc_head %u\n", desc_head);

    // int desc_idx = 0;

    struct virtq_desc *desc;
    for (desc = &virtq->desc[req_desc->next];
         desc->flags & VIRTQ_DESC_F_NEXT;
         desc = &virtq->desc[desc->next])
    {
        if (!!(desc->flags & VIRTQ_DESC_F_WRITE) == transmit) {
            LOG_SOUND_ERR("Incorrect xfer buffer type\n");
            goto abort_xfer;
        }

        uint32_t desc_transmitted = 0;
        uint32_t desc_remaining = desc->len;

        // LOG_SOUND("Transmitting desc %d of len %u\n", desc_idx++, desc_remaining);

        while (desc_remaining > 0) {

            int to_xfer = MIN(desc_remaining, pcm_remaining);

            // LOG_SOUND("Transmitting chunk of len %u (pcm %u/%u, desc %u/%u)\n", to_transmit,
            //     pcm_written, pcm_buffer.len, desc_transmitted, desc->len);

            if (transmit) {
                memcpy((void *)pcm_buffer.addr + pcm_transmitted, (void *)desc->addr + desc_transmitted, to_xfer);
            }
            desc_transmitted += to_xfer;
            desc_remaining -= to_xfer;

            pcm_transmitted += to_xfer;
            pcm_remaining -= to_xfer;

            if (pcm_remaining == 0) {

                sddf_snd_pcm_data_t pcm;
                pcm.addr = pcm_buffer.addr;
                pcm.len = pcm_transmitted;
                pcm.stream_id = hdr->stream_id;
                pcm.cookie = msg_id;
                pcm.status = 0;
                pcm.latency_bytes = 0;

                if (sddf_snd_enqueue_pcm_data(req_ring, &pcm) != 0) {
                    LOG_SOUND_ERR("Failed to enqueue to pcm request\n");
                    goto abort_xfer;
                }

                // TODO: remove
                // LOG_SOUND("Enqueing %u bytes\n", pcm_transmitted);
                // if (transmit) {
                //     print_bytes((void *)pcm_buffer.addr);
                // }
                msg->ref_count++;
                *notify_driver = true;

                if (!buffer_queue_dequeue(free_buffers, &pcm_buffer)) {
                    LOG_SOUND_ERR("Failed to dequeue free buffer\n");
                    goto abort_xfer;
                }

                pcm_remaining = SDDF_SND_PCM_BUFFER_SIZE;
                pcm_transmitted = 0;
                // LOG_SOUND("Refreshed PCM, xfer %u\n", to_transmit);
            }
        }
    }

    if (pcm_transmitted > 0) {
        // LOG_SOUND("Enqueing last %u bytes\n", pcm_transmitted);
        // // TODO: remove
        // if (transmit) {
        //     print_bytes((void *)pcm_buffer.addr);
        // }

        sddf_snd_pcm_data_t pcm;
        pcm.addr = pcm_buffer.addr;
        pcm.len = pcm_transmitted;
        pcm.stream_id = hdr->stream_id;
        pcm.cookie = msg_id;
        pcm.status = 0;
        pcm.latency_bytes = 0;

        if (sddf_snd_enqueue_pcm_data(req_ring, &pcm) != 0) {
            LOG_SOUND_ERR("Failed to enqueue to tx used\n");
            goto abort_xfer;
        }
        msg->ref_count++;
        *notify_driver = true;
    }

    if (msg->ref_count == 0) {
        LOG_SOUND_ERR("XFER sent no data\n");
        goto abort_xfer;
    }

    if ((desc->flags & VIRTQ_DESC_F_WRITE) == 0) {
        LOG_SOUND_ERR("No status buffer\n");
        return;
    }
    // LOG_SOUND("Sent tx data, desc %u\n", desc_head);
    return;
    
abort_xfer:
    for (;
         desc->flags & VIRTQ_DESC_F_NEXT;
         desc = &virtq->desc[desc->next]);
        
    assert(desc->flags & VIRTQ_DESC_F_WRITE);
    
    uint32_t *status_ptr = (void *)desc->addr;
    *status_ptr = VIRTIO_SND_S_IO_ERR;

    struct virtq_used_elem *used_elem = &virtq->used->ring[virtq->used->idx % virtq->num];
    used_elem->id = desc_head;
    used_elem->len = sizeof(uint32_t);
    virtq->used->idx++;

    // Any inbound replies will be ignored.
    handle.replied = 1;
    *respond = true;
}

static void handle_virtq(struct virtio_device *dev,
                         int index, bool *notify_driver, bool *respond)
{
    virtio_queue_handler_t *vq = &dev->vqs[index];
    struct virtq *virtq = &vq->virtq;

    uint16_t idx = vq->last_idx;

    // LOG_SOUND("Handle VQ %s: idx=%u, avail->idx=%u\n", queue_name[index], idx, virtq->avail->idx);

    for (; idx != virtq->avail->idx; idx++) {
        // LOG_SOUND("Queue %s message %i\n", queue_name[index], idx);

        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

        switch (index) {
        case CONTROLQ:
            handle_control_msg(dev, virtq, desc_head, notify_driver, respond);
            break;
        case TXQ:
            handle_xfer(dev, virtq, desc_head, get_state(dev)->rings.tx_req, &tx_buffers, true, notify_driver, respond);
            break;
        case RXQ:
            handle_xfer(dev, virtq, desc_head, get_state(dev)->rings.rx_req, &rx_buffers, false, notify_driver, respond);
            break;
        default:
            LOG_SOUND("Queue %d not implemented", index);
        }
    }

    vq->last_idx = idx;
}

static int virtio_snd_mmio_queue_notify(struct virtio_device *dev)
{
    // LOG_SOUND("Virtio device notified by client\n");

    if (dev->data.QueueSel > VIRTIO_SND_NUM_VIRTQ) {
        // @alexbr: handle error appropriately
        LOG_SOUND_ERR("Invalid queue\n");
        return 0;
    }

    // debug_device_info(&dev->data);
    // debug_virq_contents(dev);

    bool notify_driver = false;
    bool respond = false;

    handle_virtq(dev, dev->data.QueueNotify, &notify_driver, &respond);

    if (notify_driver) {
        microkit_notify(dev->sddf_ch);
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
                     void **data_region_handles,
                     void **sddf_ring_handles, size_t sddf_ch)
{
    dev->data.DeviceID = DEVICE_ID_VIRTIO_SOUND;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = vqs;
    dev->num_vqs = num_vqs;
    dev->virq = virq;
    dev->data_region_handles = data_region_handles;
    dev->sddf_ring_handles = sddf_ring_handles;
    dev->sddf_ch = sddf_ch;
    
    virtio_snd_config_init();
    virtio_snd_msg_store_init(&messages, SDDF_SND_NUM_BUFFERS);

    buffer_queue_create(&tx_buffers, tx_buffer_data, BUFFER_QUEUE_SIZE);
    buffer_queue_create(&rx_buffers, rx_buffer_data, BUFFER_QUEUE_SIZE);
    fetch_buffers(dev);
}

static void respond_to_message(struct virtq *virtq, uint8_t status,
                               uint32_t cookie, void *response, unsigned len, bool *respond)
{
    msg_handle_t *msg = virtio_snd_msg_store_get(&messages, cookie);
    // TODO: msg might be invalid on tx error if already removed

    // LOG_SOUND("Got response %s, cookie %u, ref_count %u, desc_head %u\n",
    //     sddf_snd_status_code_str(response.status), response.cookie,
    //     msg->ref_count, msg->desc_head);

    if (status != SDDF_SND_S_OK) {
        msg->status = status;
    }
    
    assert(msg->ref_count > 0);
    if (--msg->ref_count > 0)
        return;

    uint16_t desc_head = msg->desc_head;
    virtio_snd_msg_store_remove(&messages, cookie);

    if (msg->replied) {
        LOG_SOUND_ERR("UIO SND|WARN: message already replied to\n");
        return;
    }
    
    struct virtq_desc *req_desc = &virtq->desc[desc_head];

    if ((req_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
        LOG_SOUND_ERR("Message missing status descriptor\n");
    }

    struct virtq_desc *status_desc = &virtq->desc[req_desc->next];

    for (;
        status_desc->flags & VIRTQ_DESC_F_NEXT;
        status_desc = &virtq->desc[status_desc->next]);

    assert(status_desc->flags & VIRTQ_DESC_F_WRITE);

    memcpy((void *)status_desc->addr, response, len);

    // LOG_SOUND("Responding with %s, desc %u, cookie %u\n",
    //         sddf_snd_status_code_str(status), desc_head, cookie);
    
    struct virtq_used_elem *used_elem = &virtq->used->ring[virtq->used->idx % virtq->num];
    used_elem->id = desc_head;
    used_elem->len = len;
    virtq->used->idx++;

    *respond = true;
}

void handle_rx_response(struct virtio_device *dev, sddf_snd_pcm_data_t *pcm, bool *respond)
{
    msg_handle_t *msg = virtio_snd_msg_store_get(&messages, pcm->cookie);

    if (pcm->status != SDDF_SND_S_OK) {
        msg->status = pcm->status;
    }
    
    // TODO: respond
    assert(msg->ref_count > 0);
    --msg->ref_count;

    virtio_queue_handler_t *vq = &dev->vqs[RXQ];
    struct virtq *virtq = &vq->virtq;

    struct virtq_desc *req_desc = &virtq->desc[msg->desc_head];

    if ((req_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
        LOG_SOUND_ERR("Message missing status descriptor\n");
    }

    struct virtq_desc *desc = &virtq->desc[req_desc->next];

    uint32_t desc_position = 0;

    void *pcm_data = (void *)pcm->addr;
    unsigned pcm_remaining = pcm->len;

    // LOG_SOUND("RX: pcm len: %u, desc_head %u, bytes_received %u (start)\n", pcm->len, msg->desc_head, msg->bytes_received);
    // print_bytes(pcm_data);

    for (;
        desc->flags & VIRTQ_DESC_F_NEXT;
        desc = &virtq->desc[desc->next])
    {
        if ((desc->flags & VIRTQ_DESC_F_WRITE) == 0) {
            // TODO: fail
            LOG_SOUND_ERR("Expected VIRTQ_DESC_F_WRITE on RX buffer\n");
            continue;
        }

        if (pcm_remaining == 0) {
            // LOG_SOUND("No more pcm remaining\n");
            break;
        }

        uint32_t desc_end = desc_position + desc->len;
        // printf("Processing desc: pos %u, end %u, (len %u)\n", desc_position, desc_end, desc->len);

        if (desc_end > msg->bytes_received) {
            assert(desc_position <= msg->bytes_received);

            uint32_t offset = msg->bytes_received - desc_position;
            uint32_t to_write = MIN(pcm_remaining, desc->len - offset);

            // LOG_SOUND("Desc %u: writing %u bytes at offset %u, pcm_remaining %u, bytes_received %u\n",
            //     desc_position, to_write, offset, pcm_remaining, msg->bytes_received);

            memcpy((void *)desc->addr + offset, pcm_data, to_write);
            pcm_data += to_write;
            msg->bytes_received += to_write;
            pcm_remaining -= to_write;
        }
        
        desc_position += desc->len;
    }

    if (msg->ref_count == 0) {
        if (pcm_remaining != 0) {
            LOG_SOUND_ERR("Received too much PCM for RX\n");
        }
        if (desc_position != msg->bytes_received) {
            LOG_SOUND_ERR("Did not receive enough PCM for RX: desc_position %u, bytes_received %u\n",
                desc_position, msg->bytes_received);
        }

        struct virtq_desc *prev_desc = desc;
        for (;
            desc->flags & VIRTQ_DESC_F_NEXT;
            desc = &virtq->desc[desc->next]);

        if (prev_desc != desc) {
            LOG_SOUND_ERR("Desc not fully advanced\n");
        }

        if ((desc->flags & VIRTQ_DESC_F_WRITE) == 0) {
            // TODO: fail
            LOG_SOUND_ERR("Expected VIRTQ_DESC_F_WRITE on status buffer\n");
        }

        struct virtio_snd_pcm_status response;
        response.status = virtio_status_from_sddf(msg->status);
        response.latency_bytes = pcm->latency_bytes;

        memcpy((void *)desc->addr, &response, sizeof(response));

        LOG_SOUND("Responding to RX with %s, desc %u, cookie %u, desc_position %u\n",
                sddf_snd_status_code_str(msg->status), msg->desc_head, pcm->cookie, desc_position);
        
        struct virtq_used_elem *used_elem = &virtq->used->ring[virtq->used->idx % virtq->num];
        used_elem->id = msg->desc_head;
        used_elem->len = desc_position + sizeof(response);
        virtq->used->idx++;

        virtio_snd_msg_store_remove(&messages, pcm->cookie);

        *respond = true;
    }
}

static void debug_ring(const char *name, sddf_snd_ring_state_t *ring)
{
    LOG_SOUND("%s: (%p) write_idx %u, read_idx %u, size %u\n",
        name, ring, ring->write_idx, ring->read_idx, ring->size);
    LOG_SOUND("%s: (%p) write_idx %#x, read_idx %#x, size %#x\n",
        name, ring, ring->write_idx, ring->read_idx, ring->size);
}

void virtio_snd_notified(struct virtio_device *dev)
{
    // LOG_SOUND("virtIO sound device notified by server\n");

    sddf_snd_state_t *state = get_state(dev);
    bool respond = false;

    snd_config.streams = state->shared_state->streams;

    sddf_snd_response_t response;
    while (sddf_snd_dequeue_response(state->rings.responses, &response) == 0) {

        uint32_t virtio_response = virtio_status_from_sddf(response.status);
        respond_to_message(&dev->vqs[CONTROLQ].virtq, response.status, response.cookie,
                    &virtio_response, sizeof(virtio_response), &respond);
    }

    sddf_snd_pcm_data_t pcm;
    while (sddf_snd_dequeue_pcm_data(state->rings.tx_res, &pcm) == 0) {
        buffer_queue_enqueue(&tx_buffers, pcm.addr, pcm.len);

        // TODO: this should use msg->status
        struct virtio_snd_pcm_status response;
        response.status = virtio_status_from_sddf(pcm.status);
        response.latency_bytes = pcm.latency_bytes;

        respond_to_message(&dev->vqs[TXQ].virtq, pcm.status, pcm.cookie,
                    &response, sizeof(response), &respond);
    }

    while (sddf_snd_dequeue_pcm_data(state->rings.rx_res, &pcm) == 0) {
        handle_rx_response(dev, &pcm, &respond);
        buffer_queue_enqueue(&rx_buffers, pcm.addr, pcm.len);
    }

    if (respond) {
        virtio_snd_respond(dev);
    }
}
