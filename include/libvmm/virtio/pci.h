/*
 * Copyright 2025, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <libvmm/util/util.h>

/* PCI REVISION */
#define VIRTIO_PCI_REVISION 1

/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG 1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
/* ISR Status */
#define VIRTIO_PCI_CAP_ISR_CFG 3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG 4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG 5
/* Shared memory region */
#define VIRTIO_PCI_CAP_SHARED_MEMORY_CFG 8
/* Vendor-specific data */
#define VIRTIO_PCI_CAP_VENDOR_CFG 9

#define REG_RANGE(r0, r1)   r0 ... (r1 - 1)

/* This is the default for PCI devices as it allows enough space
 * for all the PCI capabilities */
#define VIRTIO_PCI_DEFAULT_BAR_SIZE 0x10000

#define PCI_CFG_OFFSET_COMMAND       0x04
#define PCI_CFG_OFFSET_STATUS        0x06
#define PCI_CFG_OFFSET_LATENCY_TIMER 0x0d
#define PCI_CFG_OFFSET_BAR1          0x10
#define PCI_CFG_OFFSET_BAR2          0x14
#define PCI_CFG_OFFSET_BAR3          0x18
#define PCI_CFG_OFFSET_BAR4          0x1C
#define PCI_CFG_OFFSET_BAR5          0x20
#define PCI_CFG_OFFSET_BAR6          0x24
#define PCI_CFG_OFFSET_CARDBUS       0x28
#define PCI_CFG_OFFSET_CAP_PTR       0x34
#define PCI_CFG_OFFSET_IRQ_LINE      0x3c

// PCI Capability IDs
#define PCI_CAP_ID_PM       0x01    // Power Management
#define PCI_CAP_ID_AGP      0x02    // AGP
#define PCI_CAP_ID_VPD      0x03    // Vital Product Data
#define PCI_CAP_ID_SLOTID   0x04    // Slot Identification
#define PCI_CAP_ID_MSI      0x05    // Message Signaled Interrupts
#define PCI_CAP_ID_CHSWP    0x06    // CompactPCI HotSwap
#define PCI_CAP_ID_PCIX     0x07    // PCI-X
#define PCI_CAP_ID_HT       0x08    // HyperTransport
#define PCI_CAP_ID_VNDR     0x09    // Vendor Specific
#define PCI_CAP_ID_DBG      0x0A    // Debug port
#define PCI_CAP_ID_CCRC     0x0B    // CompactPCI Central Resource Control
#define PCI_CAP_ID_SHPC     0x0C    // PCI Standard Hot-Plug Controller
#define PCI_CAP_ID_SSVID    0x0D    // Bridge subsystem vendor/device ID
#define PCI_CAP_ID_AGP3     0x0E    // AGP Target PCI-PCI bridge
#define PCI_CAP_ID_SECDEV   0x0F    // Secure Device
#define PCI_CAP_ID_EXP      0x10    // PCI Express
#define PCI_CAP_ID_MSIX     0x11    // MSI-X
#define PCI_CAP_ID_SATA     0x12    // SATA Data/Index Conf.
#define PCI_CAP_ID_AF       0x13    // PCI Advanced Features
#define PCI_CAP_ID_EA       0x14    // PCI Enhanced Allocation

// PCI Class
#define PCI_CLASS_STORAGE_SCSI           0x0100
#define PCI_CLASS_NETWORK_ETHERNET       0x0200
#define PCI_CLASS_COMMUNICATION_OTHER    0x0780

#define PCI_CLASS_CODE(x) ((x >> 8) & 0xFF)
#define PCI_SUB_CLASS(x) (x & 0xFF)

// PCI Status Register
#define PCI_STATUS_IMM_READY            0x01    /* Immediate Readiness */
#define PCI_STATUS_INTERRUPT            0x08    /* Interrupt status */
#define PCI_STATUS_CAP_LIST             0x10    /* Support Capability List */
#define PCI_STATUS_66MHZ                0x20    /* Support 66 MHz PCI 2.1 bus */
#define PCI_STATUS_UDF                  0x40    /* Support User Definable Features [obsolete] */
#define PCI_STATUS_FAST_BACK            0x80    /* Accept fast-back to back */
#define PCI_STATUS_PARITY               0x100   /* Detected parity error */
#define PCI_STATUS_DEVSEL_MASK          0x600   /* DEVSEL timing */
#define PCI_STATUS_DEVSEL_FAST          0x000
#define PCI_STATUS_DEVSEL_MEDIUM        0x200
#define PCI_STATUS_DEVSEL_SLOW          0x400
#define PCI_STATUS_SIG_TARGET_ABORT     0x800   /* Set on target abort */
#define PCI_STATUS_REC_TARGET_ABORT     0x1000  /* Master ack of " */
#define PCI_STATUS_REC_MASTER_ABORT     0x2000  /* Set on master abort */
#define PCI_STATUS_SIG_SYSTEM_ERROR     0x4000  /* Set when we drive SERR */
#define PCI_STATUS_DETECTED_PARITY      0x8000  /* Set on parity error */

// PCI Command Register
#define PCI_COMMAND_IO                 0x1    /* Enable response in I/O space */
#define PCI_COMMAND_MEMORY             0x2    /* Enable response in Memory space */
#define PCI_COMMAND_MASTER             0x4    /* Enable bus mastering */
#define PCI_COMMAND_SPECIAL            0x8    /* Enable response to special cycles */
#define PCI_COMMAND_INVALIDATE         0x10   /* Use memory write and invalidate */
#define PCI_COMMAND_VGA_PALETTE        0x20   /* Enable palette snooping */
#define PCI_COMMAND_PARITY             0x40   /* Enable parity checking */
#define PCI_COMMAND_WAIT               0x80   /* Enable address/data stepping */
#define PCI_COMMAND_SERR               0x100  /* Enable SERR */
#define PCI_COMMAND_FAST_BACK          0x200  /* Enable back-to-back writes */
#define PCI_COMMAND_INTX_DISABLE       0x400  /* INTx Emulation Disable */

// PCI Common configuration
#define VIRTIO_PCI_COMMON_DEV_FEATURE_SEL       0x0
#define VIRTIO_PCI_COMMON_DEV_FEATURE           0x4
#define VIRTIO_PCI_COMMON_DRI_FEATURE_SEL       0x8
#define VIRTIO_PCI_COMMON_DRI_FEATURE           0xc
#define VIRTIO_PCI_COMMON_MSIX                  0x10
#define VIRTIO_PCI_COMMON_NUM_QUEUES            0x12
#define VIRTIO_PCI_COMMON_DEV_STATUS            0x14
#define VIRTIO_PCI_COMMON_CFG_GENERATION        0x15
#define VIRTIO_PCI_COMMON_Q_SELECT              0x16
#define VIRTIO_PCI_COMMON_Q_SIZE                0x18
#define VIRTIO_PCI_COMMON_Q_MSIX                0x1a
#define VIRTIO_PCI_COMMON_Q_ENABLE              0x1c
#define VIRTIO_PCI_COMMON_Q_NOTIF_OFF           0x1e
#define VIRTIO_PCI_COMMON_Q_DESC_LO             0x20
#define VIRTIO_PCI_COMMON_Q_DESC_HI             0x24
#define VIRTIO_PCI_COMMON_Q_AVAIL_LO            0x28
#define VIRTIO_PCI_COMMON_Q_AVAIL_HI            0x2c
#define VIRTIO_PCI_COMMON_Q_USED_LO             0x30
#define VIRTIO_PCI_COMMON_Q_USED_HI             0x34
#define VIRTIO_PCI_COMMON_Q_NOTIF_DATA          0x38
#define VIRTIO_PCI_COMMON_Q_RESET               0x3a
#define VIRTIO_PCI_COMMON_ADM_Q_IDX             0x3c
#define VIRTIO_PCI_COMMON_ADM_Q_NUM             0x3e
#define VIRTIO_PCI_COMMON_END                   0x40

#define VIRTIO_PCI_BUS_NUM                      0x1
#define VIRTIO_PCI_DEVS_PER_BUS                 (1 << 5)
#define VIRTIO_PCI_FUNCS_PER_DEV                4         // (1 << 3)
#define VIRTIO_PCI_DEV_FUNC_MAX                 (VIRTIO_PCI_BUS_NUM * VIRTIO_PCI_DEVS_PER_BUS * VIRTIO_PCI_FUNCS_PER_DEV)
#define VIRTIO_PCI_FUNC_CFG_SPACE_SIZE          0x100
#define VIRTIO_PCI_MAX_MEM_BARS                 0x10

#define VIRTIO_PCI_VENDOR_ID             0x1AF4
#define VIRTIO_PCI_MODERN_BASE_DEVICE_ID 0x1040 // "non-transitional"
#define VIRTIO_PCI_NOTIF_OFF_MULTIPLIER  0x2
#define VIRTIO_PCI_QUEUE_NUM_MAX         0x2
#define VIRTIO_PCI_QUEUE_SIZE            0x100

/* Specification section 4.1.5.1.2 - MSI-X Vector Configuration */
/* Vector value used to disable MSI for queue */
#define VIRTIO_MSI_NO_VECTOR 0xffff

// Type 0 headers for endpoints
struct pci_config_space {
    // Device Identification
    uint16_t vendor_id;           // 0x00: Vendor ID
    uint16_t device_id;           // 0x02: Device ID
    uint16_t command;             // 0x04: Command Register
    uint16_t status;              // 0x06: Status Register
    uint8_t revision_id;         // 0x08: Revision ID
    uint8_t prog_if;             // 0x09: Programming Interface
    uint8_t subclass;            // 0x0A: Sub Class Code
    uint8_t class_code;          // 0x0B: Base Class Code
    uint8_t cache_line_size;     // 0x0C: Cache Line Size
    uint8_t latency_timer;       // 0x0D: Latency Timer
    uint8_t header_type;         // 0x0E: Header Type
    uint8_t bist;                // 0x0F: Built-in Self Test

    // Base Address Registers (BARs)
    uint32_t bar[6];              // 0x10-0x27: Base Address Registers

    // Subsystem Information
    uint32_t cardbus_cis_ptr;     // 0x28: CardBus CIS Pointer
    uint16_t subsystem_vendor_id; // 0x2C: Subsystem Vendor ID
    uint16_t subsystem_device_id; // 0x2E: Subsystem Device ID
    uint32_t expansion_rom_addr;  // 0x30: Expansion ROM Base Address

    // Capabilities and Interrupts
    uint8_t cap_ptr;             // 0x34: Capabilities Pointer
    uint8_t reserved1[3];        // 0x35-0x37: Reserved
    uint32_t reserved2;           // 0x38-0x3B: Reserved
    uint8_t interrupt_line;      // 0x3C: Interrupt Line
    uint8_t interrupt_pin;       // 0x3D: Interrupt Pin
    uint8_t min_gnt;             // 0x3E: Min_Gnt
    uint8_t max_lat;             // 0x3F: Max_Lat

    // Capability list
    uint8_t cap_data[192];
} __attribute__((packed));

struct pci_memory_resource {
    uintptr_t vm_addr;
    uintptr_t vmm_addr;
    uint32_t size;
    uint32_t free_offset;
};

struct virtio_pci_ecam {
    uintptr_t vm_base;
    uintptr_t vmm_base;
    uint32_t size;
};

typedef struct virtio_pci_data {
    uint32_t device_id;
    uint32_t vendor_id;
    uint32_t device_class;
    // Index to get dev's data structure:
    //   dev_table_idx = ((bus_id * #dev_per_bus) + dev_slot) * #funcs_per_dev + func_id
    uint32_t dev_table_idx;
    // Indices to the bar in global_memory_bars
    uint32_t mem_bar_ids[6];
    // Leave this so multiple memory regions can be supported in the future
    struct pci_memory_resource *mem_resources;
} virtio_pci_data_t;

typedef struct virtio_device virtio_device_t;

struct pci_memory_bar {
    uintptr_t vaddr;
    uint64_t size;
    uintptr_t free_offset;
    virtio_device_t *dev;
    uint8_t idx;                  // bar index in the device
};

struct pci_bar_memory_bits {
    uint32_t memory_type : 1;    // Bit 0: 0 = Memory space
    uint32_t mem_type : 2;    // Bits 2-1: Memory type
    uint32_t prefetchable : 1;    // Bit 3: Prefetchable
    uint32_t base_address : 28;   // Bits 31-4: Base address
} __attribute__((packed));

struct pci_bar_io_bits {
    uint32_t io_type : 1;    // Bit 0: 1 = I/O space
    uint32_t reserved : 1;    // Bit 1: Reserved (must be 0)
    uint32_t base_address : 30;   // Bits 31-2: I/O base address
} __attribute__((packed));

// Vendor-specific Capability (ID 0x09)
struct virtio_pci_cap {
    uint8_t cap_id;               /* Generic PCI field: PCI_CAP_ID_VNDR */
    uint8_t next_ptr;             /* Generic PCI field: next ptr. */
    uint8_t cap_len;              /* Generic PCI field: capability length */
    uint8_t cfg_type;             /* Identifies the structure. */
    uint8_t bar;                  /* Where to find it. */
    uint8_t id;                   /* Multiple capabilities of the same type */
    uint8_t padding[2];           /* Pad to full dword. */
    uint32_t offset;              /* Offset within bar. */
    uint32_t length;              /* Length of the structure, in bytes. */
};

// MSI-X Capability (ID 0x11)
struct msix_capability {
    uint8_t cap_id;              // 0x11 (MSI-X capability ID)
    uint8_t next_ptr;            // Pointer to next capability
    uint16_t message_control;     // Control register
    uint32_t table_offset_bir;    // Table offset and BAR indicator
    uint32_t pba_offset_bir;      // Pending bit array offset and BAR
};

/* PCI vendor config capability */
struct virtio_pci_vendor_cap {
    uint8_t cap_vndr;             /* Generic PCI field: PCI_CAP_ID_VNDR */
    uint8_t cap_next;             /* Generic PCI field: next ptr. */
    uint8_t cap_len;              /* Generic PCI field: capability length */
    uint8_t cfg_type;             /* Identifies the structure. */
    uint16_t vendor_id;           /* Identifies the vendor-specific format. */
};

/* PCI Notify capability */
struct virtio_pci_notify_cap {
    struct virtio_pci_cap cap;
    uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */
};

/* PCI CFG capability: provides indirect access */
struct virtio_pci_cfg_cap {
    struct virtio_pci_cap cap;
    uint32_t pci_cfg_data;
};

struct virtio_pci_common_cfg {
    /* About the whole device. */
    uint32_t device_feature_select;     /* read-write */
    uint32_t device_feature;            /* read-only for driver */
    uint32_t driver_feature_select;     /* read-write */
    uint32_t driver_feature;            /* read-write */
    uint16_t config_msix_vector;        /* read-write */
    uint16_t num_queues;                /* read-only for driver */
    uint8_t device_status;              /* read-write */
    uint8_t config_generation;          /* read-only for driver */

    /* About a specific virtqueue. */
    uint16_t queue_select;              /* read-write */
    uint16_t queue_size;                /* read-write */
    uint16_t queue_msix_vector;         /* read-write */
    uint16_t queue_enable;              /* read-write */
    uint16_t queue_notify_off;          /* read-only for driver */
    uint64_t queue_desc;                /* read-write */
    uint64_t queue_driver;              /* read-write */
    uint64_t queue_device;              /* read-write */
    uint16_t queue_notif_config_data;   /* read-only for driver */
    uint16_t queue_reset;               /* read-write */

    /* About the administration virtqueue. */
    uint16_t admin_queue_index;         /* read-only for driver */
    uint16_t admin_queue_num;           /* read-only for driver */
};

// typedef bool (*virtio_pci_cfg_exception_handler_t)(virtio_device_t *dev, size_t vcpu_id, size_t offset, size_t fsr,
//                                                    seL4_UserContext *regs);

bool virtio_pci_ecam_init(uintptr_t ecam_base_vm, uintptr_t ecam_base_vmm, uint32_t ecam_size);
/*
 * Registers a new virtIO device at a given guest-physical region.
 *
 * Assumes the virtio_device_t *dev struct passed has been populated
 * and virtual IRQ associated with the device has been registered.
 */
bool pci_register_virtio_device(virtio_device_t *dev, int virq);
bool pci_ecam_add_device(uint8_t bus, uint8_t dev, uint8_t func, struct pci_config_space *config_space);

bool virtio_pci_alloc_dev_cfg_space(virtio_device_t *dev, uint8_t dev_slot);
bool virtio_pci_register_memory_resource(uintptr_t vm_addr, uintptr_t vmm_addr, uint32_t size);
bool virtio_pci_alloc_memory_bar(virtio_device_t *dev, uint8_t bar_id, uint32_t size);
