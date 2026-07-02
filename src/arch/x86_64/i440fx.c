/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <libvmm/libvmm.h>

/* Document referenced:
 * [1] INTEL 440FX PCISET 82441FX PCI AND MEMORY CONTROLLER
 * (PMC) AND 82442FX DATA BUS ACCELERATOR (DBX)
 * https://theretroweb.com/chip/documentation/440fx-630a5b376fe30538505578.pdf
 *
 * [2] 82371FB (PIIX) AND 82371SB (PIIX3) PCI ISA IDE XCELERATOR
 * https://theretroweb.com/chip/documentation/82371sb-intelcorporation-62f8fda6daac0627455529.pdf
 *
 * [3]
 * https://www.intel.com/Assets/PDF/datasheet/290562.pdf
 */

/* [1] See section "3.2.1. PCI CONFIGURATION ACCESS" */
#define INTEL_82441_VENDOR_ID 0x8086
#define INTEL_82441_DEVICE_ID 0x1237
#define INTEL_82441_CMD_REG 0x6
#define INTEL_82441_STS_REG 0x280
#define INTEL_82441_CLASS_CODE 0x6

/* [2] See section "2.2. PCI Configuration Registers—PCI To ISA Bridge (Function 0)" */
#define INTEL_82371_VENDOR_ID 0x8086
#define INTEL_82371_DEVICE_ID 0x7000
#define INTEL_82371_CMD_REG 0x7
#define INTEL_82371_STS_REG 0x200
#define INTEL_82371_CLASS_CODE 0x6
#define INTEL_82371_SUBCLASS_CODE 0x1
#define INTEL_82371_REVISION 0x8

/* [3] See section "7.1. Power Management PCI Configuration Registers (PCI Function 3)" */
#define INTEL_82371_PM_VENDOR_ID 0x8086
#define INTEL_82371_PM_DEVICE_ID 0x7113
#define INTEL_82371_PM_STS_REG 0x280
#define INTEL_82371_PM_REVISION 0x8
#define INTEL_82371_PM_CLASS_CODE 0x6
#define INTEL_82371_PM_SUBCLASS_CODE 0x80

bool register_i440fx_on_pci_bus(void)
{
    /* Host Bridge (440FX) at 0:0.0,
     * ISA Bridge (PIIX3) at 0:1.0 and 0:1.3 */
    bool success = pci_register_device(0, 0, 0,
                                       &(pci_device_register_data_t) {
                                           .vendor_id = INTEL_82441_VENDOR_ID,
                                           .device_id = INTEL_82441_DEVICE_ID,
                                           .command = INTEL_82441_CMD_REG,
                                           .status = INTEL_82441_STS_REG,
                                           .revision_id = 0,
                                           .subclass = 0,
                                           .class_code = INTEL_82441_CLASS_CODE,
                                           .subsystem_vendor_id = 0,
                                           .subsystem_device_id = 0,
                                       })
                != INVALID_PCI_DEVICE_HANDLE;

    if (!success) {
        return false;
    }

    /* ISA bridge */
    success = pci_register_device(0, 1, 0,
                                  &(pci_device_register_data_t) {
                                      .vendor_id = INTEL_82371_VENDOR_ID,
                                      .device_id = INTEL_82371_DEVICE_ID,
                                      .command = INTEL_82371_CMD_REG,
                                      .status = INTEL_82371_STS_REG,
                                      .revision_id = INTEL_82371_REVISION,
                                      .subclass = INTEL_82371_SUBCLASS_CODE,
                                      .class_code = INTEL_82371_CLASS_CODE,
                                      .subsystem_vendor_id = 0,
                                      .subsystem_device_id = 0,
                                  })
           != INVALID_PCI_DEVICE_HANDLE;
    if (!success) {
        return false;
    }

    /* Cheating here, the PIIX3 doesn't have power management function, so uses the PIIX4's
     * since Windows expect this. */
    success = pci_register_device(0, 1, 3,
                                  &(pci_device_register_data_t) {
                                      .vendor_id = INTEL_82371_PM_VENDOR_ID,
                                      .device_id = INTEL_82371_PM_DEVICE_ID,
                                      .command = 0,
                                      .status = INTEL_82371_PM_STS_REG,
                                      .revision_id = INTEL_82371_PM_REVISION,
                                      .subclass = INTEL_82371_PM_SUBCLASS_CODE,
                                      .class_code = INTEL_82371_PM_CLASS_CODE,
                                      .subsystem_vendor_id = 0,
                                      .subsystem_device_id = 0,
                                  })
           != INVALID_PCI_DEVICE_HANDLE;
    if (!success) {
        return false;
    }

    return true;
}
