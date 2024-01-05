#include "sound.h"
#include "config.h"
#include "microkit.h"
#include "virtio/mmio.h"
#include "virq.h"
#include "sound/libsoundsharedringbuffer/include/sddf_snd_shared_ringbuffer.h"

#define DEBUG_SOUND

#if defined(DEBUG_SOUND)
#define LOG_SOUND(...) do{ printf("VIRTIO(SOUND): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_SOUND(...) do{}while(0)
#endif

#define LOG_SOUND_ERR(...) do{ printf("VIRTIO(BLOCK)|ERROR: "); printf(__VA_ARGS__); }while(0)

// @ivanv: put in util or remove
#define BIT_LOW(n)  (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32))

#define CONTROLQ 0
#define EVENTQ 1
#define TXQ 2
#define RXQ 3

// @alexbr: why is this global?
static struct virtio_snd_config snd_config;

/* Mapping for command ID and its virtio descriptor */
// @alexbr: currently commands are responded to synchronously, so
// this is a bit overkill. Could change to a simple circular queue.
static struct virtio_snd_cmd_store {
    uint16_t sent_cmds[SDDF_SND_NUM_CMD_BUFFERS]; /* index is command ID, maps to virtio descriptor head */
    uint32_t freelist[SDDF_SND_NUM_CMD_BUFFERS]; /* index is free command ID, maps to next free command ID */
    uint32_t head;
    uint32_t tail;
    uint32_t num_free;
} cmd_store;

static sddf_snd_state_t *get_state(struct virtio_device *dev)
{
    // @alexbr: currently this casts from a void ** which doesn't make sense.
    // I prefer to have a struct instead of array however, need to discuss.
    return (sddf_snd_state_t *)dev->sddf_ring_handles;
}

static void virtio_snd_cmd_store_init(unsigned int num_buffers)
{
    cmd_store.head = 0;
    cmd_store.tail = num_buffers - 1;
    cmd_store.num_free = num_buffers;
    for (unsigned int i = 0; i < num_buffers - 1; i++) {
        cmd_store.freelist[i] = i + 1;
    }
    cmd_store.freelist[num_buffers - 1] = -1;
}

/** Check if the command store is full. */
static bool virtio_snd_cmd_store_full()
{
    return cmd_store.num_free == 0;
}

/**
 * Allocate a command store slot for a new virtio command.
 * 
 * @param desc virtio descriptor to store in a command store slot 
 * @param id pointer to command ID allocated
 * @return 0 on success, -1 on failure
 */
static int virtio_snd_cmd_store_allocate(uint16_t desc, uint32_t *id)
{
    if (virtio_snd_cmd_store_full()) {
        return -1;
    }

    // Store descriptor into head of command store
    cmd_store.sent_cmds[cmd_store.head] = desc;
    *id = cmd_store.head;

    // Update head to next free command store slot
    cmd_store.head = cmd_store.freelist[cmd_store.head];

    // Update number of free command store slots
    cmd_store.num_free--;
    
    return 0;
}

/**
 * Retrieve and free a command store slot.
 * 
 * @param id command ID to be retrieved
 * @return virtio descriptor stored in slot
 */
static uint16_t virtio_snd_cmd_store_retrieve(uint32_t id)
{
    assert(cmd_store.num_free < SDDF_SND_NUM_CMD_BUFFERS);

    if (cmd_store.num_free == 0) {
        // Head points to stale index, so restore it
        cmd_store.head = id;
    }

    cmd_store.freelist[cmd_store.tail] = id;
    cmd_store.tail = id;

    cmd_store.num_free++;

    return cmd_store.sent_cmds[id];
}

static void virtio_snd_config_init()
{
    snd_config.jacks = 0;
    snd_config.streams = 0;
    snd_config.chmaps = 0;
}

static void virtio_snd_mmio_reset(struct virtio_device *dev)
{
    LOG_SOUND("virtio_snd_mmio_reset\n");

    for (int i = 0; i < VIRTIO_SND_NUM_VIRTQ; i++) {
        dev->vqs[i].ready = 0;
        dev->vqs[i].last_idx = 0;
    }
}

static int virtio_snd_mmio_get_device_features(struct virtio_device *dev, uint32_t *features)
{
    LOG_SOUND("virtio_snd_mmio_get_device_features\n");

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
    LOG_SOUND("virtio_snd_mmio_set_driver_features %#x\n", features);

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

    LOG_SOUND("get device config with base_addr 0x%x and field_address 0x%x has value %d\n", config_base_addr, config_field_addr, *ret_val);
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
    LOG_SOUND("  DeviceID %#x\n", info->DeviceID);
    LOG_SOUND("  VendorID %#x\n", info->VendorID);
    LOG_SOUND("  DeviceFeaturesSel %#x\n", info->DeviceFeaturesSel);
    LOG_SOUND("  DriverFeaturesSel %#x\n", info->DriverFeaturesSel);
    LOG_SOUND("  features_happy %i\n", info->features_happy);
    LOG_SOUND("  QueueSel %#x\n", info->QueueSel);
    LOG_SOUND("  QueueNotify %#x\n", info->QueueNotify);
    LOG_SOUND("  InterruptStatus %#x\n", info->InterruptStatus);
    LOG_SOUND("  Status %#x\n", info->InterruptStatus);
    LOG_SOUND("-------------------------\n");
}

static void debug_virq_contents(struct virtio_device *dev)
{
     for (int i = 0; i < VIRTIO_SND_NUM_VIRTQ; i++) {
         virtio_queue_handler_t *vq = &dev->vqs[i];
         struct virtq *virtq = &vq->virtq;

         uint16_t idx = vq->last_idx;

         for (; idx != virtq->avail->idx; idx++) {
             uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

             uint16_t curr_desc_head = desc_head;

             // Print out what the command type is
             struct virtio_snd_hdr *virtio_cmd = (void *)virtq->desc[curr_desc_head].addr;
             LOG_SOUND("  [%s] %s\n", queue_name[i], code_to_str(virtio_cmd->code));
         }
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
    switch(direction)
    {
        case SDDF_SND_D_INPUT: return VIRTIO_SND_D_INPUT;
        case SDDF_SND_D_OUTPUT: return VIRTIO_SND_D_OUTPUT;
    }
    return (uint8_t)-1;
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
    }

    *bytes_written += i * sizeof(*responses);
    LOG_SOUND("Sent %d PCM info responses\n", i);
    return status;
}

static int handle_pcm_set_params(struct virtio_device *dev,
                                 struct virtq *virtq,
                                 uint16_t desc_head,
                                 struct virtio_snd_pcm_set_params *set_params,
                                 bool *is_async)
{
    sddf_snd_state_t *state = get_state(dev);

    uint32_t id;
    int err = virtio_snd_cmd_store_allocate(desc_head, &id);
    if (err != 0) {
        LOG_SOUND_ERR("Failed to allocate command store slot\n");
        return -1;
    }

    sddf_snd_command_t cmd;
    cmd.code = SDDF_SND_CMD_PCM_SET_PARAMS;
    cmd.cmd_id = id;
    cmd.stream_id = set_params->hdr.stream_id;
    cmd.set_params.buffer_bytes = set_params->buffer_bytes;
    cmd.set_params.period_bytes = set_params->period_bytes;
    cmd.set_params.channels = set_params->channels;
    cmd.set_params.format = set_params->format;
    cmd.set_params.rate = set_params->rate;

    if (sddf_snd_enqueue_cmd(state->rings.commands, &cmd) != 0) {
        LOG_SOUND_ERR("Failed to enqueue command\n");
        return -1;
    }

    *is_async = true;
    return 0;
}

// Returns number of bytes written to virtq
static void handle_control_msg(struct virtio_device *dev,
                               struct virtq *virtq,
                               uint16_t desc_head,
                               bool *is_async,
                               uint32_t *bytes_written)
{
    struct virtq_desc *req_desc = &virtq->desc[desc_head];
    struct virtio_snd_hdr *hdr = (void *)req_desc->addr;

    if ((req_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
        LOG_SOUND_ERR("Control message missing status descriptor\n");
    }

    struct virtq_desc *status_desc = &virtq->desc[req_desc->next];
    uint32_t *status_ptr = (void *)status_desc->addr;

    LOG_SOUND("  Command code %s\n", code_to_str(hdr->code));

    switch (hdr->code) {
    case VIRTIO_SND_R_PCM_INFO: {

        if ((status_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
            LOG_SOUND_ERR("Control message missing response descriptor\n");
            *status_ptr = VIRTIO_SND_S_BAD_MSG;
            *bytes_written += sizeof(uint32_t);
            break;
        }

        struct virtq_desc *response_desc = &virtq->desc[status_desc->next];
        *status_ptr = handle_pcm_info(dev, virtq,
                                      (void *)hdr,
                                      (void *)response_desc->addr,
                                      response_desc->len / sizeof(struct virtio_snd_pcm_info),
                                      bytes_written);
        *bytes_written += sizeof(uint32_t);
        break;
    }
    case VIRTIO_SND_R_PCM_SET_PARAMS: {
        int err = handle_pcm_set_params(dev, virtq, desc_head, (void *)hdr, is_async);
        if (err) {
            *status_ptr = VIRTIO_SND_S_IO_ERR;
            *bytes_written += sizeof(uint32_t);
        }
        break;
    }
    case VIRTIO_SND_R_JACK_INFO:
    case VIRTIO_SND_R_JACK_REMAP:
    case VIRTIO_SND_R_PCM_PREPARE:
    case VIRTIO_SND_R_PCM_RELEASE:
    case VIRTIO_SND_R_PCM_START:
    case VIRTIO_SND_R_PCM_STOP:
    case VIRTIO_SND_R_CHMAP_INFO:
        LOG_SOUND_ERR("Control message not implemented: %s\n", code_to_str(hdr->code));
        *status_ptr = VIRTIO_SND_S_NOT_SUPP;
        *bytes_written += sizeof(uint32_t);
        break;
    default:
        LOG_SOUND_ERR("Unknown control message %s\n", code_to_str(hdr->code));
        *status_ptr = VIRTIO_SND_S_BAD_MSG;
        *bytes_written += sizeof(uint32_t);
        break;
    }
}

static int virtio_snd_mmio_queue_notify(struct virtio_device *dev)
{
    LOG_SOUND("virtio_snd_mmio_queue_notify\n");

    if (dev->data.QueueSel > VIRTIO_SND_NUM_VIRTQ) {
        // @alexbr: handle error appropriately
        LOG_SOUND_ERR("Invalid queue\n");
        return 0;
    }

    // @alexbr: why not QueueSel?
    LOG_SOUND("Driver notified device on %s\n", queue_name[dev->data.QueueNotify]);
    debug_device_info(&dev->data);

    virtio_queue_handler_t *vq_handler = &dev->vqs[dev->data.QueueNotify];
    struct virtq *virtq = &vq_handler->virtq;

    uint16_t idx = vq_handler->last_idx;

    bool notify_driver = false;
    bool respond = false;

    for (; idx != virtq->avail->idx; idx++) {
        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

        uint32_t len = 0;

        struct virtq_desc *desc = &virtq->desc[desc_head];
        uint32_t flags;
        do {
            const char *write = desc->flags & VIRTQ_DESC_F_WRITE ? "write" : "read";
            LOG_SOUND("  Descriptor addr %#x len %d flags %#x (%s) next %d\n",
                    desc->addr, desc->len, desc->flags, write, desc->next);

            flags = desc->flags; 
            desc = &virtq->desc[desc->next];
        }
        while (flags & VIRTQ_DESC_F_NEXT);

        bool is_async = false;
        if (dev->data.QueueNotify == CONTROLQ) {
            handle_control_msg(dev, virtq, desc_head, &is_async, &len);
        } else {
            LOG_SOUND("Queue %d not implemented", dev->data.QueueNotify);
        }

        if (is_async) {
            notify_driver = true;
            respond = false;
            assert(len == 0);
        } else {
            struct virtq_used_elem *used_elem = &virtq->used->ring[virtq->used->idx % virtq->num];
            used_elem->id = desc_head;
            used_elem->len = len;
            virtq->used->idx++;

            respond = true;
        }
    }

    vq_handler->last_idx = idx;

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
    virtio_snd_cmd_store_init(SDDF_SND_NUM_CMD_BUFFERS);
}

void virtio_snd_notified(struct virtio_device *dev)
{
    LOG_SOUND("virtio snd notified by driver\n");

    sddf_snd_state_t *state = get_state(dev);
    bool respond = false;

    // is it ok/performant to set this every time?
    snd_config.streams = state->shared_state->streams;

    sddf_snd_pcm_data_t pcm;
    while (sddf_snd_dequeue_pcm_data(state->rings.rx_used, &pcm) == 0) {
        LOG_SOUND("deq device.rx_used\n");
    }

    sddf_snd_response_t response;
    while (sddf_snd_dequeue_response(state->rings.responses, &response) == 0) {
        LOG_SOUND("deq response id=%u, status=%u\n", response.cmd_id, response.status);
        // TODO: respond to virtio driver

        uint16_t desc_head = virtio_snd_cmd_store_retrieve(response.cmd_id);
        struct virtq *virtq = &dev->vqs[CONTROLQ].virtq;
        struct virtq_desc *req_desc = &virtq->desc[desc_head];
        struct virtio_snd_hdr *hdr = (void *)req_desc->addr;

        if ((req_desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
            LOG_SOUND_ERR("Control message missing status descriptor\n");
        }

        struct virtq_desc *status_desc = &virtq->desc[req_desc->next];
        uint32_t *status_ptr = (void *)status_desc->addr;

        LOG_SOUND("Responding to code %s with %s\n",
            code_to_str(hdr->code), sddf_snd_status_code_str(response.status));
        *status_ptr = response.status;
        
        struct virtq_used_elem *used_elem = &virtq->used->ring[virtq->used->idx % virtq->num];
        used_elem->id = desc_head;
        used_elem->len = sizeof(uint32_t);
        virtq->used->idx++;

        respond = true;
    }

    if (respond) {
        virtio_snd_respond(dev);
    }
}
