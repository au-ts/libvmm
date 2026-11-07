/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/libvmm.h>
#include <sddf/util/util.h>

/* Implement a "virtual" bochs-display device as per QEMU:

QEMU 11.0.2 monitor - type 'help' for more information
(qemu) info pci
...
  Bus  0, device   4, function 0:
    Display controller: PCI device 1234:1111
      PCI subsystem 1af4:1100
      BAR0: 32 bit prefetchable memory at 0xfd000000 [0xfdffffff]
      BAR2: 32 bit memory at 0xfebca000 [0xfebcafff]
      BAR6: 32 bit memory (not mapped)
      id ""
...

We will expose this device onto the guest's virtual PCI bus for video.
You must map the real bochs-display device's BARs into guest RAM where
OVMF/Linux wants to place them.

The nice thing about boch-display compared to ramfb or virtio-gpu is
that it support resizing the viewport AND does not need DMA, so we
don't need the IOMMU or map the guest RAM physically contiguous.
 */

#define VENDOR_ID 0x1234
#define DEVICE_ID 0x1111
#define REVISION 0x2 /* See `bochs_display_realize` in QEMU's bochs-display.c */
#define CLASS 0x3
#define SUBCLASS 0x80
#define SUBSYS_VENDOR 0x1af4
#define SUBSYS_DEVICE 0x1100

/* See `bochs_display_properties` in QEMU's bochs-display.c */
#define FRAMEBFUFFER_SIZE 0x1000000

/* See `PCI_VGA_MMIO_SIZE` in QEMU's bochs-vbe.h */
#define VIDEO_REGS_SIZE 0x1000

/* See `bochs_display_realize` in QEMU's bochs-display.c */
#define FRAMEBUFFER_BAR_ID 0
#define VIDEO_REGS_BAR_ID 2

bool register_qemu_bochs_display_on_pci_bus(uint8_t bus, uint8_t dev, uint8_t func)
{
    pci_dev_handle_t handle = pci_register_device(bus, dev, func,
                                                  &(pci_device_register_data_t) {
                                                      .vendor_id = VENDOR_ID,
                                                      .device_id = DEVICE_ID,
                                                      .command = BIT(1) /* Respond to memory space */,
                                                      .status = 0,
                                                      .revision_id = REVISION,
                                                      .subclass = SUBCLASS,
                                                      .class_code = CLASS,
                                                      .subsystem_vendor_id = SUBSYS_VENDOR,
                                                      .subsystem_device_id = SUBSYS_DEVICE,
                                                  });
    if (handle == INVALID_PCI_DEVICE_HANDLE) {
        LOG_VMM_ERR("Failed to register bochs-display on virtual PCI Bus\n");
        return false;
    }

    if (!pci_register_device_mmio_bar(handle, FRAMEBUFFER_BAR_ID, FRAMEBFUFFER_SIZE, NULL, NULL)) {
        LOG_VMM_ERR("Failed to register bochs-display framebuffer BAR on virtual PCI Bus\n");
        return false;
    }

    if (!pci_register_device_mmio_bar(handle, VIDEO_REGS_BAR_ID, VIDEO_REGS_SIZE, NULL, NULL)) {
        LOG_VMM_ERR("Failed to register bochs-display video registers BAR on virtual PCI Bus\n");
        return false;
    }

    return true;
}