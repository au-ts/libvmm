#ifndef JAILHOUSE_H
#define JAILHOUSE_H

#include <stdio.h>
#include <stdint.h>

#define dprintf(...)

typedef uint64_t __u64;
typedef uint32_t __u32;
typedef uint16_t __u16;
typedef  uint8_t  __u8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef  uint8_t  u8;

struct jailhouse_console {
	__u64 address;
	__u32 size;
	__u16 type;
	__u16 flags;
	__u32 divider;
	__u32 gate_nr;
	__u64 clock_reg;
} __attribute__((packed));

#define COMM_REGION_GENERIC_HEADER					\
	/** Communication region magic JHCOMM */			\
	char signature[6];						\
	/** Communication region ABI revision */			\
	__u16 revision;							\
	/** Cell state, initialized by hypervisor, updated by cell. */	\
	volatile __u32 cell_state;					\
	/** Message code sent from hypervisor to cell. */		\
	volatile __u32 msg_to_cell;					\
	/** Reply code sent from cell to hypervisor. */			\
	volatile __u32 reply_from_cell;					\
	/** Holds static flags, see JAILHOUSE_COMM_FLAG_*. */		\
	__u32 flags;							\
	/** Debug console that may be accessed by the inmate. */	\
	struct jailhouse_console console;				\
	/** Base address of PCI memory mapped config. */		\
	__u64 pci_mmconfig_base;

struct jailhouse_comm_region {
	COMM_REGION_GENERIC_HEADER;

	/** I/O port address of the PM timer (x86-specific). */
	__u16 pm_timer_address;
	/** Number of CPUs available to the cell (x86-specific). */
	__u16 num_cpus;
	/** Calibrated TSC frequency in kHz (x86-specific). */
	__u32 tsc_khz;
	/** Calibrated APIC timer frequency in kHz or 0 if TSC deadline timer
	 * is available (x86-specific). */
	__u32 apic_khz;
} __attribute__((packed));

#define COMM_REGION_BASE	0x70000000
#define comm_region	((struct jailhouse_comm_region *)COMM_REGION_BASE)


#define PCI_CFG_VENDOR_ID	0x000
#define PCI_CFG_DEVICE_ID	0x002
#define PCI_CFG_COMMAND		0x004
# define PCI_CMD_IO		(1 << 0)
# define PCI_CMD_MEM		(1 << 1)
# define PCI_CMD_MASTER		(1 << 2)
# define PCI_CMD_INTX_OFF	(1 << 10)
#define PCI_CFG_STATUS		0x006
# define PCI_STS_INT		(1 << 3)
# define PCI_STS_CAPS		(1 << 4)
#define PCI_CFG_BAR		0x010
# define PCI_BAR_64BIT		0x4
#define PCI_CFG_CAP_PTR		0x034

#define PCI_ID_ANY		0xffff

#define PCI_DEV_CLASS_OTHER	0xff

#define PCI_CAP_MSI		0x05
#define PCI_CAP_VENDOR		0x09
#define PCI_CAP_MSIX		0x11

#define MSIX_CTRL_ENABLE	0x8000
#define MSIX_CTRL_FMASK		0x4000


static inline u8 mmio_read8(void *address)
{
	return *(volatile u8 *)address;
}

static inline void mmio_write8(void *address, u8 value)
{
	*(volatile u8 *)address = value;
}

static inline u16 mmio_read16(void *address)
{
	return *(volatile u16 *)address;
}

static inline void mmio_write16(void *address, u16 value)
{
	*(volatile u16 *)address = value;
}

static inline u32 mmio_read32(void *address)
{
	return *(volatile u32 *)address;
}

static inline void mmio_write32(void *address, u32 value)
{
	*(volatile u32 *)address = value;
}

static inline u64 mmio_read64(void *address)
{
	return *(volatile u64 *)address;
}



static void *pci_get_device_mmcfg_base(u16 bdf)
{
	void *mmcfg = (void *)(unsigned long)comm_region->pci_mmconfig_base;

	return mmcfg + ((unsigned long)bdf << 12);
}

static u32 pci_read_config(u16 bdf, unsigned int addr, unsigned int size)
{
	void *cfgaddr = pci_get_device_mmcfg_base(bdf) + addr;

	switch (size) {
	case 1:
		return mmio_read8(cfgaddr);
	case 2:
		return mmio_read16(cfgaddr);
	case 4:
		return mmio_read32(cfgaddr);
	default:
		return -1;
	}
}

static void pci_write_config(u16 bdf, unsigned int addr, u32 value, unsigned int size)
{
	void *cfgaddr = pci_get_device_mmcfg_base(bdf) + addr;

	switch (size) {
	case 1:
		mmio_write8(cfgaddr, value);
		break;
	case 2:
		mmio_write16(cfgaddr, value);
		break;
	case 4:
		mmio_write32(cfgaddr, value);
		break;
	}
}

static void pci_msix_set_vector(u16 bdf, unsigned int vector, u32 index)
{
	/* dummy for now, should never be called */
	*(int *)0xdeaddead = 0;
}

static int pci_find_device(u16 vendor, u16 device, u16 start_bdf)
{
	unsigned int bdf;
	u16 id;

	for (bdf = start_bdf; bdf < 0x10000; bdf++) {
		id = pci_read_config(bdf, PCI_CFG_VENDOR_ID, 2);
		if (id == PCI_ID_ANY || (vendor != PCI_ID_ANY && vendor != id))
			continue;
		if (device == PCI_ID_ANY ||
		    pci_read_config(bdf, PCI_CFG_DEVICE_ID, 2) == device)
			return bdf;
	}
	return -1;
}

static int pci_find_cap(u16 bdf, u16 cap)
{
	u8 pos = PCI_CFG_CAP_PTR - 1;

	if (!(pci_read_config(bdf, PCI_CFG_STATUS, 2) & PCI_STS_CAPS))
		return -1;

	while (1) {
		pos = pci_read_config(bdf, pos + 1, 1);
		if (pos == 0)
			return -1;
		if (pci_read_config(bdf, pos, 1) == cap)
			return pos;
	}
}

#define VENDORID			0x110a
#define DEVICEID			0x4106

#define BAR_BASE			0xff100000
#define PAGE_SIZE           0x1000

#define IVSHMEM_CFG_STATE_TAB_SZ	0x04
#define IVSHMEM_CFG_RW_SECTION_SZ	0x08
#define IVSHMEM_CFG_OUT_SECTION_SZ	0x10
#define IVSHMEM_CFG_ADDRESS		0x18

#define JAILHOUSE_SHMEM_PROTO_UNDEFINED	0x0000

struct ivshm_regs {
	u32 id;
	u32 max_peers;
	u32 int_control;
	u32 doorbell;
	u32 state;
};

struct ivshmem_dev_data {
	u16 bdf;
	struct ivshm_regs *registers;
	u32 *state_table;
	u32 state_table_sz;
	u32 *rw_section;
	u64 rw_section_sz;
	u32 *in_sections;
	u32 *out_section;
	u64 out_section_sz;
	u32 *msix_table;
	u32 id;
	int msix_cap;
};

static struct ivshmem_dev_data dev;
static unsigned int irq_base, vectors, target;

static inline void stop() {
    while(1);
}

static u64 pci_cfg_read64(u16 bdf, unsigned int addr)
{
	return pci_read_config(bdf, addr, 4) |
		((u64)pci_read_config(bdf, addr + 4, 4) << 32);
}

static void init_device(struct ivshmem_dev_data *d)
{
	unsigned long baseaddr, addr, size;
	int vndr_cap, n;
	u32 max_peers;

	vndr_cap = pci_find_cap(d->bdf, PCI_CAP_VENDOR);
	if (vndr_cap < 0) {
		printf("IVSHMEM ERROR: missing vendor capability\n");
		stop();
	}

	d->registers = (struct ivshm_regs *)BAR_BASE;
	pci_write_config(d->bdf, PCI_CFG_BAR, (unsigned long)d->registers, 4);
	dprintf("IVSHMEM: bar0 is at %p\n", d->registers);

	d->msix_table = (u32 *)(BAR_BASE + PAGE_SIZE);
	pci_write_config(d->bdf, PCI_CFG_BAR + 4,
			 (unsigned long)d->msix_table, 4);
	dprintf("IVSHMEM: bar1 is at %p\n", d->msix_table);

	pci_write_config(d->bdf, PCI_CFG_COMMAND,
			 (PCI_CMD_MEM | PCI_CMD_MASTER), 2);

	// map_range((void *)BAR_BASE, 2 * PAGE_SIZE, MAP_UNCACHED);

	d->id = mmio_read32(&d->registers->id);
	dprintf("IVSHMEM: ID is %d\n", d->id);

	max_peers = mmio_read32(&d->registers->max_peers);
	dprintf("IVSHMEM: max. peers is %d\n", max_peers);

	target = d->id < max_peers ? (d->id + 1) : 0;
	// target = cmdline_parse_int("target", target);

	d->state_table_sz =
		pci_read_config(d->bdf, vndr_cap + IVSHMEM_CFG_STATE_TAB_SZ, 4);
	d->rw_section_sz =
		pci_cfg_read64(d->bdf, vndr_cap + IVSHMEM_CFG_RW_SECTION_SZ);
	d->out_section_sz =
		pci_cfg_read64(d->bdf, vndr_cap + IVSHMEM_CFG_OUT_SECTION_SZ);
	baseaddr = pci_cfg_read64(d->bdf, vndr_cap + IVSHMEM_CFG_ADDRESS);

	addr = baseaddr;
	d->state_table = (u32 *)addr;

	addr += d->state_table_sz;
	d->rw_section = (u32 *)addr;

	addr += d->rw_section_sz;
	d->in_sections = (u32 *)addr;

	addr += d->id * d->out_section_sz;
	d->out_section = (u32 *)addr;

	dprintf("IVSHMEM: state table is at %p\n", d->state_table);
	dprintf("IVSHMEM: R/W section is at %p\n", d->rw_section);
	dprintf("IVSHMEM: input sections start at %p\n", d->in_sections);
	dprintf("IVSHMEM: output section is at %p\n", d->out_section);

	// size = d->state_table_sz + d->rw_section_sz +
	// 	max_peers * d->out_section_sz;
	// map_range((void *)baseaddr, size, MAP_CACHED);

	// d->msix_cap = pci_find_cap(d->bdf, PCI_CAP_MSIX);
	// vectors = d->msix_cap > 0 ? MAX_VECTORS : 1;
	// for (n = 0; n < vectors; n++) {
	// 	if (d->msix_cap > 0)
	// 		pci_msix_set_vector(d->bdf, irq_base + n, n);
	// 	irq_enable(irq_base + n);
	// }
    mmio_write32(&d->registers->int_control, 1);
}

static void send_irq(struct ivshmem_dev_data *d)
{
	// u32 int_no = d->msix_cap > 0 ? (d->id + 1) : 0;
    u32 int_no = 0;

	// disable_irqs();
	dprintf("IVSHMEM: sending IRQ %d to peer %d\n", int_no, target);
	// enable_irqs();
	mmio_write32(&d->registers->doorbell, int_no | (target << 16));
}

#endif
