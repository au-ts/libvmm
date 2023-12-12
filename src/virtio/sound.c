#include "sound.h"
#include "config.h"
#include "virtio/mmio.h"
#include "virq.h"

#define DEBUG_SOUND

#if defined(DEBUG_SOUND)
#define LOG_SOUND(...) do{ printf("VIRTIO(SOUND): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_SOUND(...) do{}while(0)
#endif

#define LOG_SOUND_ERR(...) do{ printf("VIRTIO(BLOCK)|ERROR: "); printf(__VA_ARGS__); }while(0)

// @ivanv: put in util or remove
#define BIT_LOW(n)  (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32 ))

// @alexbr: why is this global?
static struct virtio_snd_config snd_config;

static void virtio_snd_config_init()
{
    snd_config.jacks = 0;
    snd_config.streams = 1;
    snd_config.chmaps = 0;
}

static void virtio_snd_mmio_reset(struct virtio_device *dev)
{
    LOG_SOUND("virtio_snd_mmio_reset\n");
    virtio_snd_config_init();

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
    LOG_SOUND_ERR("tried to set device config\n");

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

static int virtio_snd_mmio_queue_notify(struct virtio_device *dev)
{
    LOG_SOUND("virtio_snd_mmio_queue_notify\n");

    if (dev->data.QueueSel > VIRTIO_SND_NUM_VIRTQ) {
        // @alexbr: handle error appropriately
        LOG_SOUND_ERR("tried to set device config\n");
        return 0;
    }

    // @alexbr: why not QueueSel?
    LOG_SOUND("Driver notified device on %s\n", queue_name[dev->data.QueueNotify]);
    debug_device_info(&dev->data);

    virtio_queue_handler_t *vq = &dev->vqs[dev->data.QueueNotify];
    struct virtq *virtq = &vq->virtq;

    uint16_t idx = vq->last_idx;

    for (; idx != virtq->avail->idx; idx++) {
        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

        uint16_t curr_desc_head = desc_head;

        // Print out what the command type is
        struct virtio_snd_hdr *virtio_cmd = (void *)virtq->desc[curr_desc_head].addr;
        LOG_SOUND("  Command code %s\n", code_to_str(virtio_cmd->code));
    }

    vq->last_idx = idx;

    // just respond for now
    virtio_snd_respond(dev);

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
}
