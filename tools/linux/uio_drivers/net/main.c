#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <uio/libuio.h>

#include "log.h"

#define NET_INTERFACE "eth0"

#define MAX_EVENTS 20
struct epoll_event events[MAX_EVENTS];

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
        close(sockfd);
        return -1;
    } else {
        LOG_NET("set raw socket %d to non-blocking\n", sockfd);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, NET_INTERFACE, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
        LOG_NET_ERR("can't get the interface index of the network interface.\n");
        close(sockfd);
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
        close(sockfd);
        return -1;
    } else {
        LOG_NET("binded sock %d to network interface\n", sockfd);
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        LOG_NET_ERR("can't create the epoll fd.\n");
        close(sockfd);
        return -1;
    } else {
        LOG_NET("created epoll fd %d\n", epoll_fd);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
        LOG_NET_ERR("can't register the socket fd to epoll.\n");
        close(sockfd);
        close(epoll_fd);
        return -1;
    } else {
        LOG_NET("registered socket fd to epoll\n");
    }

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
