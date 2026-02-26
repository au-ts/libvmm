#include <stdbool.h>
#include <libvmm/virtio/virtio.h>

void virtio_set_interrupt_status(struct virtio_device *dev, bool used_buffer, bool config_change)
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

    if (dev->transport_type == VIRTIO_TRANSPORT_PCI) {
        /*
         * virtIO spec 4.1.4.5.1 Device Requirements: ISR status capability
         */
        struct pci_config_space *config_space = virtio_pci_find_dev_cfg_space(dev);
        if (dev->regs.InterruptStatus) {
            config_space->status |= PCI_STATUS_INTERRUPT;
        } else {
            config_space->status &= ~PCI_STATUS_INTERRUPT;
        }
    }
}
