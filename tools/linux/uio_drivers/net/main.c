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

/* Change this if you want to bind to a different interface 
   make sure it is brought up first by the init script */
#define NET_INTERFACE "eth0"

/* Event queue for polling */
#define MAX_EVENTS 20
struct epoll_event events[MAX_EVENTS];

/* sDDF net queues */
#define QUEUE_MR_SIZE 0x200000
#define UIO_MAP_PATH "/sys/class/uio/uio0/maps/map" // This is linked to DITS
#define ETH_RX_FREE_Q_PADDR   0x40000000  // These are linked to SDF
#define ETH_RX_ACTIVE_Q_PADDR 0x40200000
#define ETH_TX_FREE_Q_PADDR   0x40400000
#define ETH_TX_ACTIVE_Q_PADDR 0x40600000

int set_socket_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char **argv)
{
    LOG_NET("starting up\n");

    /* Set up raw network async socket to intercept RX frames and send TX frames */
    int sockfd, epoll_fd;
    struct sockaddr_ll socket_address;
    struct ifreq ifr;
    char buffer[ETH_FRAME_LEN];

    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd == -1) {
        LOG_NET_ERR("can't create the raw socket, quiting.\n");
        return -1;
    } else {
        LOG_NET("created raw socket with fd %d\n", sockfd);
    }

    if (set_socket_nonblocking(sockfd) == -1) {
        LOG_NET_ERR("can't set the socket to non-blocking mode.\n");
        return -1;
    } else {
        LOG_NET("set raw socket %d to non-blocking\n", sockfd);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, NET_INTERFACE, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
        LOG_NET_ERR("can't get the interface index of the network interface.\n");
        return -1;
    } else {
        LOG_NET("got network interface named %s\n", ifr.ifr_name);
    }

    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_ifindex = ifr.ifr_ifindex;
    socket_address.sll_protocol = htons(ETH_P_ALL);
    if (bind(sockfd, (struct sockaddr*)&socket_address, sizeof(socket_address)) == -1) {
        LOG_NET_ERR("can't bind the socket to the network interface.\n");
        return -1;
    } else {
        LOG_NET("binded sock %d to network interface\n", sockfd);
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        LOG_NET_ERR("can't create the epoll fd.\n");
        return -1;
    } else {
        LOG_NET("created epoll fd %d\n", epoll_fd);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
        LOG_NET_ERR("can't register the socket fd to epoll.\n");
        return -1;
    } else {
        LOG_NET("registered socket fd to epoll\n");
    }

    /* Set up sDDF queues */
    int uio_fd = open("/dev/uio0", O_RDWR);
    if (uio_fd == -1) {
        LOG_NET_ERR("can't open /dev/uio0.\n");
        return -1;
    } else {
        LOG_NET("opened /dev/uio0 for R/W\n");
    }

    void* uio_base = mmap(NULL, QUEUE_MR_SIZE * 4, PROT_READ | PROT_WRITE, MAP_SHARED, uio_fd, 0);
    if (uio_base == MAP_FAILED) {
        LOG_NET_ERR("can't mmap uio\n");
        return -1;
    } else {
        LOG_NET("mmap uio success for net queues\n");
    }

    void *eth_rx_free_drv   = uio_base;
    void *eth_rx_active_drv = (void *) ((uint64_t) uio_base + QUEUE_MR_SIZE);
    void *eth_tx_free_drv   = (void *) ((uint64_t) uio_base + (QUEUE_MR_SIZE * 2));
    void *eth_tx_active_drv = (void *) ((uint64_t) uio_base + (QUEUE_MR_SIZE * 3));

    /* Set up FD to talk to the VMM, then bind it to epoll to listen to notification on TX */
    

    LOG_NET("all initialisation done, polling now\n");
    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (n == MAX_EVENTS) {
                LOG_NET_WARN("epoll_wait() returned MAX_EVENTS, there maybe dropped events!\n");
            }
            if (events[i].events & EPOLLIN) {
                // Data is available to read
                ssize_t len = recv(sockfd, buffer, sizeof(buffer), 0);
                if (len == -1) {
                    if (errno != EAGAIN) {
                        perror("recv");
                    }
                    continue;
                }

                // Process the Ethernet frame
                printf("Received Ethernet frame of length %zd\n", len);
            }
        }
    }

    close(sockfd);
    close(epoll_fd);
    return 0;
}
