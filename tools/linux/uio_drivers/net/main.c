#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#include <uio/libuio.h>

#include "log.h"

#include <sddf/network/queue.h>
#include <config/ethernet_config.h>

/* Change this if you want to bind to a different interface 
   make sure it is brought up first by the init script */
#define NET_INTERFACE "eth0"

/* Event queue for polling */
#define MAX_EVENTS 20
struct epoll_event events[MAX_EVENTS];

/* Shared memory between guest and VMM to ferry frames in and out */
#define NET_CONTROL_QUEUE_REGION_SIZE 0x200000
#define NET_DATA_SECTION_SIZE NET_CONTROL_QUEUE_REGION_SIZE

#define PAGE_SIZE_4K 0x1000

/* Raw socket FD to send/recv network frames */
int sock_fd;

/* UIO FD to access the sDDF control and data queues */
int uio_sddf_net_queues_fd;
char* sddf_net_queues_vaddr;
net_queue_handle_t rx_queue;
net_queue_handle_t tx_queue;

/* UIO FDs to wait for TX/RX interrupts from VMM */
int uio_sddf_net_tx_fd;
int uio_sddf_net_rx_fd;
/* Polling FD to wait for events from the TX/RX UIO FD */
int epoll_fd;

/* UIO FDs to access the physical addresses of the sDDF data segment 
   so we can deduct it from the offset in control queue to access the data */
int uio_sddf_vmm_net_info_passing_fd;
typedef struct {
    /* Any info that the VMM wants to give us go in here */
    uint64_t rx_paddr;
    uint64_t tx_paddr;
} vmm_net_info_t;
vmm_net_info_t *vmm_info_passing;

int set_socket_nonblocking(int sock_fd)
{
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }
    return fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
}

int create_nb_socket(void) {
    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd == -1) {
        LOG_NET_ERR("can't create the raw socket.\n");
        exit(1);
    } else {
        LOG_NET("created raw socket with fd %d\n", sock_fd);
    }

    if (set_socket_nonblocking(sock_fd) == -1) {
        LOG_NET_ERR("can't set the socket to non-blocking mode.\n");
        exit(1);
    } else {
        LOG_NET("set raw socket %d to non-blocking\n", sock_fd);
    }
    return sock_fd;
}

void bind_sock_to_net_inf(int sockfd) {
    struct sockaddr_ll socket_address;
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, NET_INTERFACE, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
        LOG_NET_ERR("can't get the interface index of the network interface.\n");
        exit(1);
    } else {
        LOG_NET("got network interface named %s\n", ifr.ifr_name);
    }

    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_ifindex = ifr.ifr_ifindex;
    socket_address.sll_protocol = htons(ETH_P_ALL);
    if (bind(sockfd, (struct sockaddr*)&socket_address, sizeof(socket_address)) == -1) {
        LOG_NET_ERR("can't bind the socket to the network interface.\n");
        exit(1);
    } else {
        LOG_NET("binded sock %d to network interface\n", sockfd);
    }
}

int create_epoll(void) {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        LOG_NET_ERR("can't create the epoll fd.\n");
        exit(1);
    } else {
        LOG_NET("created epoll fd %d\n", epoll_fd);
    }
    return epoll_fd;
} 

void bind_fd_to_epoll(int fd, int epollfd) {
    struct epoll_event sock_event;
    sock_event.events = EPOLLIN;
    sock_event.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &sock_event) == -1) {
        LOG_NET_ERR("can't register fd %d to epoll.\n", fd);
        exit(1);
    } else {
        LOG_NET("registered fd %d to epoll\n", fd);
    }
}

int open_uio(const char *abs_path) {
    int fd = open(abs_path, O_RDWR);
    if (fd == -1) {
        LOG_NET_ERR("can't open uio @ %s.\n", abs_path);
        exit(1);
    } else {
        LOG_NET("opened uio %s with fd %d\n", abs_path, fd);
    }
    return fd;
}

char *map_uio(uint64_t length, int uiofd) {
    void *base = (char *) mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, uiofd, 0);
    if (base == MAP_FAILED) {
        LOG_NET_ERR("can't mmap uio fd %d\n", uiofd);
        exit(1);
    } else {
        LOG_NET("mmap uio success for fd %d\n", uiofd);
    }

    return (char *) base;
}

void enable_uio_interrupt(int uiofd) {
    uint32_t enable = 1;
    if (write(uiofd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
        LOG_NET_ERR("Failed to Enable interrupts on uio fd %d\n", uiofd);
        exit(1);
    } else {
        LOG_NET("Enabled/ACK'ed interrupt on fd %d\n", uiofd);
    }
    fsync(uiofd);
}

int main(int argc, char **argv)
{
    LOG_NET("*** Starting up\n");

    LOG_NET("*** Setting up raw socket\n");
    sock_fd = create_nb_socket();
    bind_sock_to_net_inf(sock_fd);

    LOG_NET("*** Binding raw socket to epoll\n");
    epoll_fd = create_epoll();
    bind_fd_to_epoll(sock_fd, epoll_fd);

    LOG_NET("*** Mapping in sDDF control and data queues\n");
    uio_sddf_net_queues_fd = open_uio("/dev/uio0");
    sddf_net_queues_vaddr = map_uio(NET_CONTROL_QUEUE_REGION_SIZE, uio_sddf_net_queues_fd);

    LOG_NET("*** Setting up sDDF control and data queues\n");
    char *rx_free_drv   = sddf_net_queues_vaddr;
    char *rx_active_drv = rx_free_drv + NET_CONTROL_QUEUE_REGION_SIZE;
    char *tx_free_drv   = rx_active_drv + NET_CONTROL_QUEUE_REGION_SIZE;
    char *tx_active_drv = tx_free_drv + NET_CONTROL_QUEUE_REGION_SIZE;
    net_queue_init(&rx_queue, (net_queue_t *)rx_free_drv, (net_queue_t *)rx_active_drv, NET_RX_QUEUE_CAPACITY_DRIV);
    net_queue_init(&tx_queue, (net_queue_t *)tx_free_drv, (net_queue_t *)tx_active_drv, NET_TX_QUEUE_CAPACITY_DRIV);

    LOG_NET("*** Setting up UIO TX and RX interrupts\n");
    uio_sddf_net_tx_fd = open_uio("/dev/uio1");
    uio_sddf_net_rx_fd = open_uio("/dev/uio2");

    /* Is this needed??? */
    // enable_uio_interrupt(uio_sddf_net_tx_fd);
    // enable_uio_interrupt(uio_sddf_net_rx_fd);

    LOG_NET("*** Binding UIO TX and RX interrupts to epoll\n");
    bind_fd_to_epoll(uio_sddf_net_tx_fd, epoll_fd);
    bind_fd_to_epoll(uio_sddf_net_rx_fd, epoll_fd);

    LOG_NET("*** Setting up UIO data passing between VMM and us\n");
    uio_sddf_vmm_net_info_passing_fd = open_uio("/dev/uio3");
    vmm_info_passing = (vmm_net_info_t *) map_uio(PAGE_SIZE_4K, uio_sddf_vmm_net_info_passing_fd);

    LOG_NET("*** All initialisation successful, entering event loop\n");
    while (1) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_events == -1) {
            LOG_NET_ERR("epoll wait failed\n");
            return -1;
        }
        if (n_events == MAX_EVENTS) {
            LOG_NET_WARN("epoll_wait() returned MAX_EVENTS, there maybe dropped events!\n");
        }

        for (int i = 0; i < n_events; i++) {
            if (!(events[i].events & EPOLLIN)) {
                LOG_NET_WARN("got non EPOLLIN event on fd %d\n", events[i].data.fd);
                continue;
            }

            if (events[i].data.fd == sock_fd) {
                // Oh hey got a frame from network device!
                // rx_ferry->len = recv(sock_fd, rx_ferry->data, sizeof(rx_ferry->data), 0);
                // if (rx_ferry->len == -1) {
                //     LOG_NET_WARN("got EPOLLIN on socket FD but nothing came through???\n");
                // }

                // Now poke the VMM to process the incoming frame, we won't return until the VMM has
                // safely copied this frame into the sDDF queue


            } else if (events[i].data.fd == uio_sddf_net_tx_fd) {
                // Got TX ntfn from VMM, send it thru the raw socket
                // ssize_t send_len = send(sock_fd, tx_ferry->data, tx_ferry->len, 0);
                // if (send_len == -1) {
                //     LOG_NET_ERR("Failed to send frame in event loop, abort\n");
                //     return -1;
                // }

                // Now poke the VMM saying that it is safe to for us to transmit the next frame
            } else if (events[i].data.fd == uio_sddf_net_rx_fd) {
                LOG_NET("got rx notif\n");
            } else {
                LOG_NET_WARN("epoll_wait() returned event on unknown fd %d\n", events[i].data.fd);
            }
        }
    }
    return 0;
}
