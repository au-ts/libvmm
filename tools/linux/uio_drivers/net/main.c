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

#include <config/ethernet_config.h>

#include "log.h"

/* Change this if you want to bind to a different interface 
   make sure it is brought up first by the init script */
#define NET_INTERFACE "eth0"

/* Event queue for polling */
#define MAX_EVENTS 10
struct epoll_event events[MAX_EVENTS];

/* Shared memory between guest and VMM to ferry frames in and out */
#define TX_FRAME_REGION_SIZE 0x10000
#define RX_FRAME_REGION_SIZE TX_FRAME_REGION_SIZE

typedef struct {
    uint16_t len;
    char data[ETH_FRAME_LEN];
} frame_ferrying_region_t;

#define PAGE_SIZE_4K 0x1000

int set_socket_nonblocking(int sock_fd)
{
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }
    return fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char **argv)
{
    LOG_NET("starting up\n");

    /* Set up raw network async socket to intercept RX frames and send TX frames */
    int sock_fd, epoll_fd;
    struct sockaddr_ll socket_address;
    struct ifreq ifr;

    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd == -1) {
        LOG_NET_ERR("can't create the raw socket, quiting.\n");
        return -1;
    } else {
        LOG_NET("created raw socket with fd %d\n", sock_fd);
    }

    if (set_socket_nonblocking(sock_fd) == -1) {
        LOG_NET_ERR("can't set the socket to non-blocking mode.\n");
        return -1;
    } else {
        LOG_NET("set raw socket %d to non-blocking\n", sock_fd);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, NET_INTERFACE, IFNAMSIZ - 1);
    if (ioctl(sock_fd, SIOCGIFINDEX, &ifr) == -1) {
        LOG_NET_ERR("can't get the interface index of the network interface.\n");
        return -1;
    } else {
        LOG_NET("got network interface named %s\n", ifr.ifr_name);
    }

    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_ifindex = ifr.ifr_ifindex;
    socket_address.sll_protocol = htons(ETH_P_ALL);
    if (bind(sock_fd, (struct sockaddr*)&socket_address, sizeof(socket_address)) == -1) {
        LOG_NET_ERR("can't bind the socket to the network interface.\n");
        return -1;
    } else {
        LOG_NET("binded sock %d to network interface\n", sock_fd);
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        LOG_NET_ERR("can't create the epoll fd.\n");
        return -1;
    } else {
        LOG_NET("created epoll fd %d\n", epoll_fd);
    }

    struct epoll_event sock_event;
    sock_event.events = EPOLLIN;
    sock_event.data.fd = sock_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &sock_event) == -1) {
        LOG_NET_ERR("can't register the socket fd to epoll.\n");
        return -1;
    } else {
        LOG_NET("registered socket fd to epoll\n");
    }

    /* Set up shared memory between the VM and hypervisor to ferry frames in and out */
    int uio_frames_fd = open("/dev/uio0", O_RDWR);
    if (uio_frames_fd == -1) {
        LOG_NET_ERR("can't open /dev/uio0.\n");
        return -1;
    } else {
        LOG_NET("opened /dev/uio0 for R/W\n");
    }

    void* uio_frames_base = mmap(NULL, TX_FRAME_REGION_SIZE + RX_FRAME_REGION_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, uio_frames_fd, 0);
    if (uio_frames_base == MAP_FAILED) {
        LOG_NET_ERR("can't mmap uio region for frames ferrying\n");
        return -1;
    } else {
        LOG_NET("mmap uio success for frames ferrying\n");
    }

    frame_ferrying_region_t *tx_ferry = (frame_ferrying_region_t *) uio_frames_base;
    frame_ferrying_region_t *rx_ferry = (frame_ferrying_region_t *) ((uint64_t) uio_frames_base + RX_FRAME_REGION_SIZE);

    /* Set up UIO FDs for TX and RX */
    int uio_tx_fd = open("/dev/uio1", O_RDWR);
    if (uio_tx_fd == -1) {
        LOG_NET_ERR("can't open /dev/uio1.\n");
        return -1;
    } else {
        LOG_NET("opened /dev/uio1 for TX IRQ\n");
    }
    int uio_rx_fd = open("/dev/uio2", O_RDWR);
    if (uio_rx_fd == -1) {
        LOG_NET_ERR("can't open /dev/uio2.\n");
        return -1;
    } else {
        LOG_NET("opened /dev/uio2 for RX IRQ\n");
    }

    /* Enable interrupts on UIO FDs for TX and RX */
    uint32_t enable = 1;
    if (write(uio_tx_fd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
        LOG_NET_ERR("Failed to Enable TX interrupts\n");
        return -1;
    }
    enable = 1;
    if (write(uio_rx_fd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
        LOG_NET_ERR("Failed to Enable RX interrupts\n");
        return -1;
    }
    LOG_NET("enabled interrupts for TX and RX\n");

    /* Then mmap them in so we can generate faults into VMM */
    char *tx_fault_addr = mmap(NULL, PAGE_SIZE_4K, PROT_READ | PROT_WRITE, MAP_SHARED, uio_tx_fd, 0);
    if (tx_fault_addr == MAP_FAILED) {
        LOG_NET_ERR("can't mmap uio TX\n");
        return -1;
    } else {
        LOG_NET("mmap uio success for TX\n");
    }
    char *rx_fault_addr = mmap(NULL, PAGE_SIZE_4K, PROT_READ | PROT_WRITE, MAP_SHARED, uio_rx_fd, 0);
    if (rx_fault_addr == MAP_FAILED) {
        LOG_NET_ERR("can't mmap uio RX\n");
        return -1;
    } else {
        LOG_NET("mmap uio success for RX\n");
    }

    /* Finally bind the TX and RX fds into the epoll fd so we can receive interrupts on them */
    struct epoll_event uio_tx_event;
    uio_tx_event.events = EPOLLIN;
    uio_tx_event.data.fd = uio_tx_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, uio_tx_fd, &uio_tx_event) == -1) {
        LOG_NET_ERR("can't register the UIO TX fd to epoll.\n");
        return -1;
    } else {
        LOG_NET("registered UIO TX fd to epoll\n");
    }
    struct epoll_event uio_rx_event;
    uio_rx_event.events = EPOLLIN;
    uio_rx_event.data.fd = uio_rx_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, uio_rx_fd, &uio_rx_event) == -1) {
        LOG_NET_ERR("can't register the UIO RX fd to epoll.\n");
        return -1;
    } else {
        LOG_NET("registered UIO RX fd to epoll\n");
    }

    LOG_NET("all initialisation successful, entering event loop\n");
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
                rx_ferry->len = recv(sock_fd, rx_ferry->data, sizeof(rx_ferry->data), 0);
                if (rx_ferry->len == -1) {
                    LOG_NET_WARN("got EPOLLIN on socket FD but nothing came through???\n");
                }

                // Now poke the VMM to process the incoming frame, we won't return until the VMM has
                // safely copied this frame into the sDDF queue
                *rx_fault_addr = (char) 0;

            } else if (events[i].data.fd == uio_tx_fd) {
                // Got TX ntfn from VMM, send it thru the raw socket
                ssize_t send_len = send(sock_fd, tx_ferry->data, tx_ferry->len, 0);
                if (send_len == -1) {
                    LOG_NET_ERR("Failed to send frame in event loop, abort\n");
                    return -1;
                }

                // Now poke the VMM saying that it is safe to for us to transmit the next frame
                *tx_fault_addr = (char) 0;

                enable = 1;
                if (write(uio_tx_fd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
                    LOG_NET_ERR("Failed to reenable RX interrupts in event loop\n");
                    return -1;
                }
            } else if (events[i].data.fd == uio_rx_fd) {
                LOG_NET("got rx notif\n");
            } else {
                LOG_NET_WARN("epoll_wait() returned event on unknown fd %d\n", events[i].data.fd);
            }
        }
    }
    return 0;
}
