#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#define SHMEM_BASE  ((void*)0x30000000)
#define SHMEM_SIZE  (0x1000)
#define SHMEM_IRQ   (96)

#include "xen.h"
#include "jailhouse.h"

static struct shared_info shared_info __attribute__((aligned(0x1000)));

#if defined SEL4

// #undef SHMEM_BASE
#define EVENT_BASE  (SHMEM_BASE-0x1000)
// #define SHMEM_BASE  (EVENT_BASE+SHMEM_SIZE)

static inline void shmem_init() {

}

static inline void shmem_clear_event() {
    
}

static inline void shmem_send_event() {
    *((volatile uint32_t*)EVENT_BASE) = 1;
}

#elif defined XEN

#undef SHMEM_IRQ
#define SHMEM_IRQ   (31)

#ifdef TX

static inline void shmem_init() {

    // int64_t ret  = 0;

    // struct evtchn_alloc_unbound alloc;
    // alloc.dom = DOMID_SELF;
    // alloc.remote_dom = 1;
    // alloc.port = 0xdeadbeef;

    // ret = xen_hypercall(EVTCHNOP_alloc_unbound, (unsigned long)&alloc, 0, 0, HYPERVISOR_event_channel_op);
    // if(ret) goto err;

    // printf("tx: created port %d!\n", alloc.port);

    // struct evtchn_bind_interdomain bind;
    // bind.remote_dom = 1;
    // bind.remote_port = 0;
    // bind.local_port = 0;
    // ret = xen_hypercall(EVTCHNOP_bind_interdomain, (unsigned long)&bind, 0, 0, HYPERVISOR_event_channel_op);
    // if(ret) goto err;
    
//     return;

// err:
//     printf("tx: hypercall failed %d\n", ret);
}

static inline void shmem_send_event() {
    struct evtchn_send send = { .port = 1 };
    int64_t ret = xen_hypercall(EVTCHNOP_send, (unsigned long)&send, 0, 0, HYPERVISOR_event_channel_op);
    if(ret) {
        printf("send hypercall failed %d\n", ret);
        return;
    }
}

#else

static inline void shmem_init() {

    // printf("rx: creating binding...\n");

    // struct evtchn_bind_interdomain bind;
    // bind.remote_dom = 2;
    // bind.remote_port = 1;
    // bind.local_port = 0xdeadbeef;
    // int64_t ret = xen_hypercall(EVTCHNOP_bind_interdomain, (unsigned long)&bind, 0, 0, HYPERVISOR_event_channel_op);
    // if(ret) {
    //     printf("rx: hypercall failed %d\n", ret);
    //     return;
    // }

    // printf("rx: bound port %d\n", bind.local_port);

    int64_t ret = xen_register_shared_info(&shared_info);
    if(ret) {
        printf("rx: shared info setup failed %d\n", ret);
        return;
    }
}

static inline void shmem_clear_event() {
    shared_info.vcpu_info[0].evtchn_upcall_pending = 0;
    shared_info.vcpu_info[0].evtchn_pending_sel = 0;
    shared_info.evtchn_pending[0] = 0;
}

#endif


#elif defined BAO

static inline void shmem_init() {

}

static inline void shmem_clear_event() {
    
}

static inline void shmem_send_event() {

    register uint64_t x0 asm("x0") = 1;
    register uint64_t x1 asm("x1") = 0;
    register uint64_t x2 asm("x2") = 0;

    asm volatile("hvc #0\n" :: "r"(x0), "r"(x1), "r"(x2));
}

#elif defined JAILHOUSE

static inline void pci_init()
{


}

static inline void shmem_init() {

    dprintf("comm_region->pci_mmconfig_base = 0x%lx\n", comm_region->pci_mmconfig_base);

    dprintf("Finding pci device...\n");
    int bdf = pci_find_device(VENDORID, DEVICEID, 0);
	if (bdf == -1) {
		printf("IVSHMEM: No PCI devices found .. nothing to do.\n");
		stop();
	}

	dprintf("IVSHMEM: Found device at %02x:%02x.%x\n",
	       bdf >> 8, (bdf >> 3) & 0x1f, bdf & 0x3);
	unsigned int class_rev = pci_read_config(bdf, 0x8, 4);
	if (class_rev != (PCI_DEV_CLASS_OTHER << 24 |
	    JAILHOUSE_SHMEM_PROTO_UNDEFINED << 8)) {
		printf("IVSHMEM: class/revision %08x, not supported\n",
		       class_rev);
		stop();
	}

	dev.bdf = bdf;
	init_device(&dev);
	dprintf("IVSHMEM: initialized device\n");

}

static inline void shmem_clear_event() {
    
}

static inline void shmem_send_event() {
    send_irq(&dev);
}


#endif

// #if defined SEL4
// #undef SHMEM_BASE
// #define EVENT_BASE  ((void*)0x3F000000)
// #define SHMEM_BASE  (EVENT_BASE+0x1000)
// #endif


// static inline void shmem_init() {

// }

// static inline void shmem_clear_event() {
    
// }

// static inline void shmem_send_event() {

// }

static inline void shmem_write(uint8_t* buf, size_t size)  {
    
    uint8_t *shmem_it = (void*) SHMEM_BASE;
    for(size_t i = 0; i < size && i < SHMEM_SIZE; i++) {
        *shmem_it++ = buf[i];
    }
}

static inline void shmem_print(char* base)  {
    printf("%s.10\n", base);
}

static volatile struct {
    bool received;
    bool finished_tx;
    bool finished_rx;
    uint64_t timer_start;
    uint64_t timer_end;
} *notification_ctrl = (void*) SHMEM_BASE;

static volatile struct {
    bool sent;
    bool received;
    bool polling;
    bool start_polling;
    size_t buf_size;
    size_t transfer_size;
} *transfer_ctrl = (void*) SHMEM_BASE;

// static volatile struct {
//     struct { bool sent;} __attribute__((align(64)));
//     struct { bool received;} __attribute__((align(64)));
//     struct { bool start_polling;} __attribute__((align(64)));
//     struct { size_t transfer_size_index; } __attribute__((align(64)));
// } *transfer_ctrl = (void*) SHMEM_BASE;

void shmem_test();
void shmem_benchmark_notification();

void* const SHMEM_DATA_BASE = SHMEM_BASE + 0x1000;
const size_t SHMEM_DATA_SIZE = 0x1000;

static size_t transfer_sizes[] = {
    4 * 1024, 
    32 * 1024, 
    256 * 1024, 
    512 * 1024,
    1 * 1024 * 1024, 
    2 * 1024 * 1024,
    4 * 1024 * 1024,
    16 * 1024 * 1024,
};

static const char* const transfer_names[] = {
    "4K",
    "32K", 
    "256K", 
    "512K",
    "1M",
    "2M",
    "4M",
    "16M"
};

static size_t buffer_sizes[] = {
    4 * 1024, 
    32 * 1024, 
    256 * 1024, 
    512 * 1024,
    1 * 1024 * 1024, 
    2 * 1024 * 1024,
    4 * 1024 * 1024,
    16 * 1024 * 1024,
};

static const char* const buffer_names[] = {
    "4K",
    "32K", 
    "256K", 
    "512K",
    "1M",
    "2M",
    "4M",
    "16M"
};


#define NUM_TRANSFER_SIZES (sizeof(transfer_sizes)/sizeof(size_t))
#define NUM_BUFFER_SIZES (sizeof(buffer_sizes)/sizeof(size_t))

#define NUM_SAMPLES  (50)
#define NUM_WARMUPS  (10)

// #define SAMPLE_PMU_METRICS
