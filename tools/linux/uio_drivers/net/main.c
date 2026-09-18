/*
 * Copyright 2024, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <limits.h>
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

#include <signal.h>
#include <ucontext.h>

#include "log.h"

#include <sddf/network/config.h>
#include <sddf/network/queue.h>
#include <sddf/network/constants.h>

#include <uio/net.h>

#define ARGC_REQURED 2
#define PAGE_SIZE_4K 0x1000

/* IMPORTANT: This driver currently assumes the network interface
   has an "Ethernet-like" MTU, you should change this if your net inf is
   different. */
char frame[ETH_FRAME_LEN];

/* Raw socket FD to send/recv network frames */
int sock_fd;
struct ifreq ifr;

/* Event queue for polling */
#define MAX_EVENTS 8 // Arbitrary length
struct epoll_event events[MAX_EVENTS];

/* Polling FD to wait for events from the TX/RX UIO FD */
int epoll_fd;

/* UIO: data passing */
int uio_net_data_passing_fd;
net_drv_vm_data_passing_t *driver_config_from_uio;
net_drv_vm_data_passing_t driver_config;

/* UIO: sDDF net queues */
int uio_net_rx_free_fd;
int uio_net_tx_free_fd;
int uio_net_rx_active_fd;
int uio_net_tx_active_fd;

char *rx_free;
char *rx_active;
char *tx_free;
char *tx_active;

net_queue_handle_t rx_queue;
net_queue_handle_t tx_queue;

/* UIO: faulting mechanism for guest -> VMM notifications */
int uio_net_rx_fault_fd;
int uio_net_tx_fault_fd;

char *rx_fault;
char *tx_fault;

/* UIO: IRQs for VMM -> guest notifications */
int uio_net_rx_irq;
int uio_net_tx_irq;

/* UIO: Data */
int uio_net_rx_data_fd;
int uio_net_tx_data_fd;

char *rx_data;
char *tx_data;

static int create_promiscuous_socket(const char *net_inf)
{
    // See https://man7.org/linux/man-pages/man7/packet.7.html for more details on these operations
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd == -1) {
        perror("create_promiscuous_socket(): socket()");
        LOG_NET_ERR("could not create socket.\n");
        exit(EXIT_FAILURE);
    }

    strncpy(ifr.ifr_name, net_inf, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
        perror("create_promiscuous_socket(): ioctl()");
        LOG_NET_ERR("could not get network interface index.\n");
        exit(EXIT_FAILURE);
    }
    int ifindex = ifr.ifr_ifindex;

    struct packet_mreq mr;
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        perror("create_promiscuous_socket(): setsockopt()");
        LOG_NET_ERR("could not set net device to promiscuous mode.\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_ll bind_address;
    memset(&bind_address, 0, sizeof(bind_address));
    bind_address.sll_family = AF_PACKET;
    bind_address.sll_protocol = htons(ETH_P_ALL);
    bind_address.sll_ifindex = ifindex;
    if (bind(sockfd, (struct sockaddr *)&bind_address, sizeof(bind_address)) == -1) {
        perror("create_promiscuous_socket(): bind()");
        LOG_NET_ERR("could not set net device to promiscuous mode.\n");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

static int create_epoll(void)
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("create_epoll(): epoll_create1()");
        LOG_NET_ERR("can't create the epoll fd.\n");
        exit(EXIT_FAILURE);
    }
    return epoll_fd;
}

static void bind_fd_to_epoll(const int fd, const int epollfd)
{
    struct epoll_event sock_event;
    sock_event.events = EPOLLIN;
    sock_event.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &sock_event) == -1) {
        perror("bind_fd_to_epoll(): epoll_ctl()");
        LOG_NET_ERR("can't register fd %d to epoll fd %d.\n", fd, epollfd);
        exit(EXIT_FAILURE);
    }
}

static int open_uio(const char *dev_name)
{
    char path[PATH_MAX];
    int len = snprintf(path, sizeof(path), "/dev/%s", dev_name);
    if (len < 0 || len >= sizeof(path)) {
        LOG_NET_ERR("failed to create path string for uio device '%s'\n", dev_name);
        exit(EXIT_FAILURE);
    }

    int fd = open(path, O_RDWR);
    if (fd == -1) {
        perror("open_uio(): open()");
        LOG_NET_ERR("can't open uio @ %s.\n", path);
        exit(EXIT_FAILURE);
    }
    return fd;
}

static int uio_map_size(const char *dev_name)
{
    char path[PATH_MAX];
    char buf[PATH_MAX];

    int len = snprintf(path, sizeof(path), "/sys/class/uio/%s/maps/map0/size", dev_name);
    if (len < 0 || len >= sizeof(path)) {
        LOG_NET_ERR("failed to create map size path string for uio device '%s'\n", dev_name);
        exit(EXIT_FAILURE);
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOG_NET_ERR("Failed to open '%s' to read map size\n", path);
        exit(EXIT_FAILURE);
    }
    ssize_t ret = read(fd, buf, sizeof(buf));
    if (ret < 0) {
        LOG_NET_ERR("Failed to read map size from '%s'\n", path);
        exit(EXIT_FAILURE);
    }
    close(fd);

    int size = strtoul(buf, NULL, 0);
    if (size == 0 || size == ULONG_MAX) {
        LOG_NET_ERR("Failed to convert map size '%s' from '%s' to integer\n", path, buf);
        exit(EXIT_FAILURE);
    }

    return size;
}

static char *map_uio(const char *dev_name, const int uiofd)
{
    int size = uio_map_size(dev_name);
    int offset = 0; // Assume that UIO devices always have 1 memory region
    void *base = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, uiofd, offset);
    if (base == MAP_FAILED) {
        perror("map_uio(): mmap()");
        LOG_NET_ERR("can't mmap uio fd %d\n", uiofd);
        exit(EXIT_FAILURE);
    }
    LOG_NET("-> mapped device '%s' with size %p @ %p\n", dev_name, size, base);
    return (char *)base;
}

static void uio_interrupt_ack(const int uiofd)
{
    uint32_t enable = 1;
    if (write(uiofd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
        perror("uio_interrupt_ack(): write()");
        LOG_NET_ERR("Failed to write enable/ack interrupts on uio fd %d\n", uiofd);
        exit(EXIT_FAILURE);
    }
}

static void tx_notify(void)
{
    *tx_fault = 0;
}

static void tx_process(void)
{
    bool processed_tx = false;

    net_request_signal_active(&tx_queue);
    while (!net_queue_empty_active(&tx_queue)) {
        processed_tx = true;

        net_buff_desc_t tx_buffer;
        if (net_dequeue_active(&tx_queue, &tx_buffer) != 0) {
            LOG_NET_ERR("couldn't dequeue active TX buffer, quitting.\n");
            exit(EXIT_FAILURE);
        }

        // Workout which client it is from, so we can get the offset in data region
        const uintptr_t dma_tx_addr = tx_buffer.io_or_offset;
        const int tx_data_region_size = 0x100000; // @billn dubious
        uintptr_t tx_data_offset;
        bool tx_data_paddr_found = false;
        for (int i = 0; i < driver_config.num_tx_clients; i++) {
            if (dma_tx_addr >= driver_config.tx_data_paddrs[i]
                && dma_tx_addr < (driver_config.tx_data_paddrs[i] + tx_data_region_size)) {

                tx_data_offset = dma_tx_addr - driver_config.tx_data_paddrs[i];
                tx_data_offset += tx_data_region_size * i;
                tx_data_paddr_found = true;
                break;
            }
        }

        if (!tx_data_paddr_found) {
            LOG_NET_ERR("couldn't find corresponding client for DMA addr %p, quitting.\n", (void *)dma_tx_addr);
            exit(EXIT_FAILURE);
        }

        const char *tx_frame = (char *)((uintptr_t)tx_data + tx_data_offset);
        // Write TX frame into temp buff
        for (int i = 0; i < tx_buffer.len; i++) {
            frame[i] = tx_frame[i];
        }

        // Blocking send!
        struct sockaddr_ll sa;
        memset(&sa, 0, sizeof(sa));
        sa.sll_family = AF_PACKET;
        sa.sll_protocol = htons(ETH_P_ALL);
        sa.sll_ifindex = ifr.ifr_ifindex;
        sa.sll_halen = ETH_ALEN;

        int sent_bytes = sendto(sock_fd, frame, tx_buffer.len, 0, (struct sockaddr *)&sa, sizeof(sa));
        if (sent_bytes != tx_buffer.len) {
            perror("tx_process(): sendto()");
            LOG_NET_ERR("TX sent %d != expected %d. qutting.\n", sent_bytes, tx_buffer.len);
            exit(EXIT_FAILURE);
        }

        if (net_enqueue_free(&tx_queue, tx_buffer) != 0) {
            LOG_NET_ERR("Couldn't return free TX buffer into the sddf tx free queue. quiting\n");
            exit(EXIT_FAILURE);
        }
    }

    if (processed_tx && net_require_signal_free(&tx_queue)) {
        tx_notify();
    }
}

static int bytes_available_in_socket(void)
{
    int bytes_available;
    if (ioctl(sock_fd, FIONREAD, &bytes_available) == -1) {
        perror("bytes_available_in_socket(): ioctl()");
        LOG_NET_ERR("Couldn't poll the socket to see whether there is data available. quiting\n");
        exit(EXIT_FAILURE);
    }
    return bytes_available;
}

static void rx_notify(void)
{
    *rx_fault = 0;
}

static void rx_process(void)
{
    bool processed_rx = false;

    // Poll the socket and receive all frames until there is nothing to receive or the free queue is empty.
    net_request_signal_free(&rx_queue);
    while (bytes_available_in_socket() && !net_queue_empty_free(&rx_queue)) {
        processed_rx = true;

        net_buff_desc_t buffer;
        int err = net_dequeue_free(&rx_queue, &buffer);
        if (err) {
            LOG_NET_ERR("couldn't dequeue a free RX buf\n");
            exit(EXIT_FAILURE);
        }

        // Write frame out to temp buffer
        int num_bytes = recv(sock_fd, &frame[0], sizeof(frame), 0);
        if (num_bytes < 0) {
            perror("rx_process(): recv()");
            LOG_NET_ERR("couldnt recv from raw sock\n");
            exit(EXIT_FAILURE);
        }

        // Convert DMA addr from virtualiser to offset then mem copy
        uintptr_t offset = buffer.io_or_offset - driver_config.rx_data_paddr;
        char *buf_in_sddf_rx_data = (char *)((uintptr_t)rx_data + offset);
        for (uint64_t i = 0; i < num_bytes; i++) {
            buf_in_sddf_rx_data[i] = frame[i];
        }

        // Enqueue it to the active queue
        buffer.len = num_bytes;
        if (net_enqueue_active(&rx_queue, buffer) != 0) {
            LOG_NET_ERR("couldn't enqueue active RX buffer, quitting.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (processed_rx && net_require_signal_active(&rx_queue)) {
        rx_notify();
    }
}

int main(int argc, char **argv)
{
    if (argc != ARGC_REQURED) {
        LOG_NET("usage: ./uio_net_driver <network_interface>");
        exit(EXIT_FAILURE);
    } else {
        LOG_NET("*** Starting up\n");
        LOG_NET("*** Network interface: %s\n", argv[1]);
    }

    LOG_NET("*** Setting up raw promiscuous socket\n");
    sock_fd = create_promiscuous_socket(argv[1]);

    LOG_NET("*** Creating epoll\n");
    epoll_fd = create_epoll();

    LOG_NET("*** Binding socket to epoll\n");
    bind_fd_to_epoll(sock_fd, epoll_fd);

    LOG_NET("*** Setting up data passing page via UIO\n");
    uio_net_data_passing_fd = open_uio(UIO_NET_DATA_PASSING_DEVICE);
    driver_config_from_uio = (net_drv_vm_data_passing_t *)map_uio(UIO_NET_DATA_PASSING_DEVICE, uio_net_data_passing_fd);
    memcpy(&driver_config, driver_config_from_uio, sizeof(net_drv_vm_data_passing_t));
    if (!net_config_check_magic(&driver_config.net_config)) {
        LOG_NET_ERR("net_config magic incorrect\n");
        exit(EXIT_FAILURE);
    }

    LOG_NET("-> rx data paddr = %p\n", (void *)driver_config.rx_data_paddr);
    for (int i = 0; i < driver_config.num_tx_clients; i++) {
        LOG_NET("-> tx client #%d data paddr = %p\n", i, (void *)driver_config.tx_data_paddrs[i]);
    }

    LOG_NET("*** Setting up in sDDF net control queues\n");
    uio_net_rx_free_fd = open_uio(UIO_NET_RX_FREE_DEVICE);
    uio_net_rx_active_fd = open_uio(UIO_NET_RX_ACTIVE_DEVICE);
    uio_net_tx_free_fd = open_uio(UIO_NET_TX_FREE_DEVICE);
    uio_net_tx_active_fd = open_uio(UIO_NET_TX_ACTIVE_DEVICE);

    rx_free = map_uio(UIO_NET_RX_FREE_DEVICE, uio_net_rx_free_fd);
    rx_active = map_uio(UIO_NET_RX_ACTIVE_DEVICE, uio_net_rx_active_fd);
    tx_free = map_uio(UIO_NET_TX_FREE_DEVICE, uio_net_tx_free_fd);
    tx_active = map_uio(UIO_NET_TX_ACTIVE_DEVICE, uio_net_tx_active_fd);

    net_queue_init(&rx_queue, (net_queue_t *)rx_free, (net_queue_t *)rx_active,
                   driver_config.net_config.virt_rx.num_buffers);
    net_queue_init(&tx_queue, (net_queue_t *)tx_free, (net_queue_t *)tx_active,
                   driver_config.net_config.virt_tx.num_buffers);

    LOG_NET("*** Setting up TX/RX fault mechanism for guest -> VMM notifications\n");
    uio_net_rx_fault_fd = open_uio(UIO_NET_RX_FAULT_DEVICE);
    uio_net_tx_fault_fd = open_uio(UIO_NET_TX_FAULT_DEVICE);

    rx_fault = map_uio(UIO_NET_RX_FAULT_DEVICE, uio_net_rx_fault_fd);
    tx_fault = map_uio(UIO_NET_TX_FAULT_DEVICE, uio_net_tx_fault_fd);

    LOG_NET("*** Setting up TX/RX UIO IRQs for VMM -> guest notifications\n");
    uio_net_rx_irq = open_uio(UIO_NET_RX_IRQ_DEVICE);
    uio_net_tx_irq = open_uio(UIO_NET_TX_IRQ_DEVICE);
    uio_interrupt_ack(uio_net_rx_irq);
    uio_interrupt_ack(uio_net_tx_irq);

    LOG_NET("*** Binding UIO TX and RX incoming interrupts to epoll\n"); // So we can block on them, instead of polling.
    bind_fd_to_epoll(uio_net_rx_irq, epoll_fd);
    bind_fd_to_epoll(uio_net_tx_irq, epoll_fd);

    LOG_NET("*** Setting up in sDDF net data queues\n");
    uio_net_rx_data_fd = open_uio(UIO_NET_RX_DATA_DEVICE);
    uio_net_tx_data_fd = open_uio(UIO_NET_TX_DATA_DEVICE);

    rx_data = map_uio(UIO_NET_RX_DATA_DEVICE, uio_net_rx_data_fd);
    tx_data = map_uio(UIO_NET_TX_DATA_DEVICE, uio_net_tx_data_fd);

    LOG_NET("*** All initialisation successful, now sending all pending TX active before we block on events\n");
    tx_process();

    LOG_NET("*** All pending TX active have been sent thru the raw sock, entering event loop now.\n");
    LOG_NET("*** You won't see any output from UIO Net anymore. Unless there is a warning or error.\n");
    while (1) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_events == -1) {
            perror("main(): epoll_wait():");
            LOG_NET_ERR("epoll wait failed\n");
            exit(EXIT_FAILURE);
        }
        if (n_events == MAX_EVENTS) {
            LOG_NET_WARN("epoll_wait() returned MAX_EVENTS, there maybe dropped events!\n");
        }

        for (int i = 0; i < n_events; i++) {
            if (!(events[i].events & EPOLLIN)) {
                LOG_NET_ERR("got non EPOLLIN event on fd %d\n", events[i].data.fd);
                exit(EXIT_FAILURE);
            }

            if (events[i].data.fd == sock_fd) {
                // Oh hey got a frame from the network device!
                rx_process();
            } else if (events[i].data.fd == uio_net_tx_irq) {
                // Got virt TX ntfn from VMM, send it thru the raw socket
                tx_process();
                uio_interrupt_ack(uio_net_tx_irq);
            } else if (events[i].data.fd == uio_net_rx_irq) {
                // Got RX virt ntfn from VMM, the free RX queue got filled!
                rx_process();
                uio_interrupt_ack(uio_net_rx_irq);
            } else {
                LOG_NET_WARN("epoll_wait() returned event on unknown fd %d\n", events[i].data.fd);
            }
        }
    }

    LOG_NET_WARN("Exit\n");
    return 0;
}
