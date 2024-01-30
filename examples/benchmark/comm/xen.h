/* SPDX-License-Identifier: (BSD-3-Clause) */
/*
 * Xen definitions, hypercalls, and functions used to setup event
 * channels and send and receive event notifications.
 */

#ifndef XEN_H
#define XEN_H

#include <stdint.h>

#define GUEST_EVTCHN_PPI        31
#define DOMID_SELF              0x7FF0U

struct vcpu_time_info {
    uint32_t version;
    uint32_t pad0;
    uint64_t tsc_timestamp;
    uint64_t system_time;
    uint32_t tsc_to_system_mul;
    int8_t   tsc_shift;
    uint8_t  flags;
    uint8_t  pad1[2];
} __attribute__((__packed__)); /* 32 bytes */

struct pvclock_wall_clock {
    uint32_t version;
    uint32_t sec;
    uint32_t nsec;
    uint32_t sec_hi;
} __attribute__((__packed__));

struct arch_vcpu_info { };
struct arch_shared_info { };

struct vcpu_info {
    uint8_t evtchn_upcall_pending;
    uint8_t evtchn_upcall_mask;
    uint64_t evtchn_pending_sel;
    struct arch_vcpu_info arch;
    struct vcpu_time_info time;
};

struct shared_info {
    struct vcpu_info vcpu_info[1];
    uint64_t evtchn_pending[sizeof(uint64_t) * 8];
    uint64_t evtchn_mask[sizeof(uint64_t) * 8];

    struct pvclock_wall_clock wc;
    uint32_t wc_sec_hi;
    struct arch_shared_info arch;
};

#define active_evtchns(cpu,sh,idx)              \
    ((sh)->evtchn_pending[idx] &                \
     ~(sh)->evtchn_mask[idx])

#define HYPERVISOR_memory_op            12
#define HYPERVISOR_xen_version          17
#define HYPERVISOR_console_io           18
#define HYPERVISOR_grant_table_op       20
#define HYPERVISOR_vcpu_op              24
#define HYPERVISOR_xsm_op               27
#define HYPERVISOR_sched_op             29
#define HYPERVISOR_callback_op          30
#define HYPERVISOR_event_channel_op     32
#define HYPERVISOR_physdev_op           33
#define HYPERVISOR_hvm_op               34
#define HYPERVISOR_sysctl               35
#define HYPERVISOR_domctl               36
#define HYPERVISOR_argo_op              39
#define HYPERVISOR_dm_op                41
#define HYPERVISOR_hypfs_op             42


/* hypercalls */
static inline int64_t xen_hypercall(unsigned long arg0, unsigned long arg1,
                                    unsigned long arg2, unsigned long arg3,
                                    unsigned long hypercall)
{
    register uintptr_t a0 asm("x0") = arg0;
    register uintptr_t a1 asm("x1") = arg1;
    register uintptr_t a2 asm("x2") = arg2;
    register uintptr_t a3 asm("x3") = arg3;
    register uintptr_t nr asm("x16") = hypercall;
    asm volatile("hvc 0x4a48\n"
                     : "=r" (a0), "=r"(a1), "=r" (a2), "=r" (a3), "=r" (nr)
                     : "0" (a0),
                       "r" (a1),
                       "r" (a2),
                       "r" (a3),
                       "r" (nr));
    return a0;
}


/* console_io */
#define CONSOLEIO_write 0


/* memory_op */
#define XENMAPSPACE_shared_info  0 /* shared info page */
#define XENMAPSPACE_grant_table  1 /* grant table page */

#define XENMEM_add_to_physmap      7

struct xen_add_to_physmap {
    /* Which domain to change the mapping for. */
    uint16_t domid;

    /* Number of pages to go through for gmfn_range */
    uint16_t    size;

    /* Source mapping space. */
    unsigned int space;

    /* Index into source mapping space. */
    uint64_t idx;

    /* GPFN where the source mapping page should appear. */
    uint64_t gpfn;
};

static inline int xen_register_shared_info(struct shared_info *shared_info)
{
    int rc;
    struct xen_add_to_physmap xatp;

    xatp.domid = DOMID_SELF;
    xatp.idx = 0;
    xatp.space = XENMAPSPACE_shared_info;
    xatp.gpfn = ((unsigned long)shared_info) >> 12;
    rc = xen_hypercall(XENMEM_add_to_physmap, (unsigned long)&xatp, 0, 0,
                       HYPERVISOR_memory_op);
    return rc;
}


/* event_channel_op */
#define EVTCHNOP_bind_interdomain 0
#define EVTCHNOP_close            3
#define EVTCHNOP_send             4
#define EVTCHNOP_status           5
#define EVTCHNOP_alloc_unbound    6
#define EVTCHNOP_unmask           9

struct evtchn_bind_interdomain {
    /* IN parameters. */
    uint16_t remote_dom;
    uint32_t remote_port;
    /* OUT parameters. */
    uint32_t local_port;
};

struct evtchn_alloc_unbound {
    /* IN parameters */
    uint16_t dom, remote_dom;
    /* OUT parameters */
    uint32_t port;
};

struct evtchn_send {
    /* IN parameters. */
    uint32_t port;
};


// /* printf */
// static inline void xen_console_write(const char *str)
// {
//     ssize_t len = strlen(str);

//     xen_hypercall(CONSOLEIO_write, len, (unsigned long)str, 0,
//                   HYPERVISOR_console_io);
// }

// static inline void xen_printf(const char *fmt, ...)
// {
//     char buf[128];
//     va_list ap;
//     char *str = &buf[0];
//     memset(buf, 0x0, 128);

//     va_start(ap, fmt);
//     vsprintf(str, fmt, ap);
//     va_end(ap);

//     xen_console_write(buf);
// }


// /* 
//  * utility functions, not xen specific, but needed by the function
//  * below
//  */
// #define xchg(ptr,v) __atomic_exchange_n(ptr, v, __ATOMIC_SEQ_CST)

// static __inline__ unsigned long __ffs(unsigned long word)
// {
//         return __builtin_ctzl(word);
// }


// /* event handling */
// static inline void handle_event_irq(struct shared_info *s,
//                                     void (*do_event)(unsigned int event))
// {
//     uint64_t  l1, l2, l1i, l2i;
//     unsigned int   port;
//     int            cpu = 0;
//     struct vcpu_info   *vcpu_info = &s->vcpu_info[cpu];

//     vcpu_info->evtchn_upcall_pending = 0;
//     mb();

//     l1 = xchg(&vcpu_info->evtchn_pending_sel, 0);
//     while ( l1 != 0 )
//     {
//         l1i = __ffs(l1);
//         l1 &= ~(1UL << l1i);
//         l2 = xchg(&s->evtchn_pending[l1i], 0);

//         while ( l2 != 0 )
//         {
//             l2i = __ffs(l2);
//             l2 &= ~(1UL << l2i);

//             port = (l1i * sizeof(uint64_t)) + l2i;

//             do_event(port);
//         }
//     }
// }

#endif /* XEN_H */
