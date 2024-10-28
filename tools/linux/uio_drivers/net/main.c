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
#include <sddf/network/net_info.h>
#include <config/ethernet_config.h>
#include <uio/net.h>

/* Change this if you want to bind to a different interface 
   make sure it is brought up first by the init script */
#define NET_INTERFACE "eth0"

/* Event queue for polling */
#define MAX_EVENTS 20
struct epoll_event events[MAX_EVENTS];

#define PAGE_SIZE_4K 0x1000

/* Raw socket FD to send/recv network frames */
int sock_fd;

/* UIO FD to access the sDDF control and data queues */
int uio_sddf_net_queues_fd;
char* sddf_net_queues_vaddr;
net_queue_handle_t rx_queue;
net_queue_handle_t tx_queue;

/* UIO FDs to wait for TX/RX interrupts from VMM */
int uio_sddf_net_tx_incoming_fd;
int uio_sddf_net_rx_incoming_fd;

/* UIO FDs to signal TX/RX to VMM */
int uio_sddf_net_tx_outgoing_fd;
int uio_sddf_net_rx_outgoing_fd;
char *sddf_net_tx_outgoing_irq_fault_vaddr;
char *sddf_net_rx_outgoing_irq_fault_vaddr;

/* Polling FD to wait for events from the TX/RX UIO FD */
int epoll_fd;

/* UIO FDs to access the physical addresses of the sDDF data segment 
   so we can deduct it from the offset in control queue to access the data */
int uio_sddf_vmm_net_info_passing_fd;
vmm_net_info_t *vmm_info_passing;

int uio_sddf_net_info_fd;
net_info_t *sddf_net_info;

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

void uio_interrupt_ack(int uiofd) {
    uint32_t enable = 1;
    if (write(uiofd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
        LOG_NET_ERR("Failed to Enable interrupts on uio fd %d\n", uiofd);
        exit(1);
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
    sddf_net_queues_vaddr = map_uio((NET_DATA_REGION_CAPACITY * 4) + (NET_DATA_REGION_CAPACITY * (1 + NUM_NETWORK_CLIENTS)), uio_sddf_net_queues_fd);

    LOG_NET("*** Setting up sDDF control and data queues\n");
    char *rx_free_drv   = sddf_net_queues_vaddr;
    char *rx_active_drv = (char *) ((uint64_t) rx_free_drv + NET_DATA_REGION_CAPACITY);
    char *tx_free_drv   = (char *) ((uint64_t) rx_active_drv + NET_DATA_REGION_CAPACITY);
    char *tx_active_drv = (char *) ((uint64_t) tx_free_drv + NET_DATA_REGION_CAPACITY);
    char *rx_data_drv   = (char *) ((uint64_t) tx_active_drv + NET_DATA_REGION_CAPACITY);
    char *tx_datas_drv[NUM_NETWORK_CLIENTS] = {
        (char *) ((uint64_t) rx_data_drv + (NET_DATA_REGION_CAPACITY)),
        (char *) ((uint64_t) rx_data_drv + (NET_DATA_REGION_CAPACITY * 2))
    };

    net_queue_init(&rx_queue, (net_queue_t *)rx_free_drv, (net_queue_t *)rx_active_drv, NET_RX_QUEUE_CAPACITY_DRIV);
    net_queue_init(&tx_queue, (net_queue_t *)tx_free_drv, (net_queue_t *)tx_active_drv, NET_TX_QUEUE_CAPACITY_DRIV);

    LOG_NET("rx_free_drv   = 0x%p\n", rx_free_drv);
    LOG_NET("rx_active_drv = 0x%p\n", rx_active_drv);
    LOG_NET("tx_free_drv   = 0x%p\n", tx_free_drv);
    LOG_NET("tx_active_drv = 0x%p\n", tx_active_drv);
    LOG_NET("rx_data_drv   = 0x%p\n", rx_data_drv);
    LOG_NET("tx_data_drv cli0 = 0x%p\n", tx_datas_drv[0]);
    LOG_NET("tx_data_drv cli1 = 0x%p\n", tx_datas_drv[1]);

    LOG_NET("*** Setting up UIO TX and RX interrupts from VMM \"incoming\"\n");
    uio_sddf_net_tx_incoming_fd = open_uio("/dev/uio1");
    uio_sddf_net_rx_incoming_fd = open_uio("/dev/uio2");
    uio_interrupt_ack(uio_sddf_net_tx_incoming_fd);
    uio_interrupt_ack(uio_sddf_net_rx_incoming_fd);

    LOG_NET("*** Binding UIO TX and RX incoming interrupts to epoll\n");
    bind_fd_to_epoll(uio_sddf_net_tx_incoming_fd, epoll_fd);
    bind_fd_to_epoll(uio_sddf_net_rx_incoming_fd, epoll_fd);

    LOG_NET("*** Setting up UIO data passing between VMM and us\n");
    uio_sddf_vmm_net_info_passing_fd = open_uio("/dev/uio3");
    vmm_info_passing = (vmm_net_info_t *) map_uio(PAGE_SIZE_4K, uio_sddf_vmm_net_info_passing_fd);
    LOG_NET("RX paddr: 0x%p\n", vmm_info_passing->rx_paddr);
    LOG_NET("TX cli0 paddr: 0x%p\n", vmm_info_passing->tx_paddrs[0]);
    LOG_NET("TX cli1 paddr: 0x%p\n", vmm_info_passing->tx_paddrs[1]);

    LOG_NET("*** Setting up UIO TX and RX interrupts to VMM \"outgoing\"\n");
    uio_sddf_net_tx_outgoing_fd = open_uio("/dev/uio4");
    uio_sddf_net_rx_outgoing_fd = open_uio("/dev/uio5");
    sddf_net_tx_outgoing_irq_fault_vaddr = map_uio(PAGE_SIZE_4K, uio_sddf_net_tx_outgoing_fd);
    sddf_net_rx_outgoing_irq_fault_vaddr = map_uio(PAGE_SIZE_4K, uio_sddf_net_rx_outgoing_fd);

    LOG_NET("*** Setting up UIO sDDF net info\n");
    uio_sddf_net_info_fd = open_uio("/dev/uio6");
    sddf_net_info = map_uio(PAGE_SIZE_4K, uio_sddf_net_info_fd);

    LOG_NET("*** Writing the ready bit\n");
    __atomic_store_n(&sddf_net_info->driver_ready, true, __ATOMIC_RELEASE);

    LOG_NET("*** Waiting for RX virt to boot up\n");
    net_buff_desc_t rx_buffer;
    while (net_queue_empty_free(&rx_queue)) {
        LOG_NET("tail of rx free is %d\n", ((net_queue_t *) rx_free_drv)->tail);
    }

    LOG_NET("*** Fetching a free buffer from the RX free queue\n");
    if (net_dequeue_free(&rx_queue, &rx_buffer) != 0) {
        LOG_NET_ERR("couldn't dequeue first free RX buffer, quitting.\n");
        exit(1);
    }

    LOG_NET("head %d\n", ((net_queue_t *) rx_free_drv)->head);
    LOG_NET("tail %d\n", ((net_queue_t *) rx_free_drv)->tail);
    LOG_NET(">>> %d, io is %p\n", rx_buffer.len, rx_buffer.io_or_offset);

    LOG_NET("*** All initialisation successful, entering event loop\n");
    while (1) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_events == -1) {
            LOG_NET_ERR("epoll wait failed\n");
            exit(1);
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
                // Oh hey got a frame from the network device!
                // Convert DMA addr from virtualiser to offset
                printf("got stuff from sock\n");
                
                uintptr_t offset = rx_buffer.io_or_offset - vmm_info_passing->rx_paddr;
                char *buf_in_sddf_rx_data = (char *) ((uintptr_t) rx_data_drv + offset);

                // Write to the data buffer
                int num_bytes = recv(sock_fd, buf_in_sddf_rx_data, ETH_FRAME_LEN, 0);
                if (num_bytes < 0) {
                    LOG_NET_ERR("couldnt recv from raw sock\n");
                    exit(1);
                }

                // Enqueue it to the active queue
                rx_buffer.len = num_bytes;
                if (net_enqueue_active(&rx_queue, rx_buffer) != 0) {
                    LOG_NET_ERR("couldn't enqueue active RX buffer, quitting.\n");
                    exit(1);
                }

                // Prepare a buffer for the next recv
                if (net_dequeue_free(&rx_queue, &rx_buffer) != 0) {
                    LOG_NET_ERR("couldn't dequeue free RX buffer for next recv, quitting.\n");
                    exit(1);
                }  

                // Signal to virtualiser 
                *sddf_net_rx_outgoing_irq_fault_vaddr = 0;

            } else if (events[i].data.fd == uio_sddf_net_tx_incoming_fd) {
                printf("Got virt TX ntfn from VMM\n");

                // Got virt TX ntfn from VMM, send it thru the raw socket
                if (net_queue_empty_active(&tx_queue)) {
                    LOG_NET_WARN("active TX queue is empty when TX virt signalled? weird.\n");
                    exit(1);
                }

                while (!net_queue_empty_active(&tx_queue)) {
                    net_buff_desc_t tx_buffer;
                    if (net_dequeue_active(&tx_queue, &tx_buffer) != 0) {
                        LOG_NET_ERR("couldn't dequeue active TX buffer, err is %d, quitting.\n");
                        exit(1);
                    }  

                    // Workout which client it is from, so we can get the offset in data region
                    uintptr_t dma_tx_addr = tx_buffer.io_or_offset;
                    uintptr_t tx_data_offset;
                    int tx_client;
                    bool tx_data_paddr_found = false;
                    for (int i = 0; i < NUM_NETWORK_CLIENTS; i++) {
                        if (dma_tx_addr >= vmm_info_passing->tx_paddrs[i] && 
                            dma_tx_addr < (vmm_info_passing->tx_paddrs[i] + NET_DATA_REGION_CAPACITY)) {
                            
                            tx_data_offset = dma_tx_addr - vmm_info_passing->tx_paddrs[i];
                            tx_client = i;
                            tx_data_paddr_found = true;
                        }
                    }

                    if (!tx_data_paddr_found) {
                        LOG_NET_ERR("couldn't find corresponding client for DMA addr 0x%p, quitting.\n", dma_tx_addr);
                        exit(1);
                    }

                    char *tx_data_base = tx_datas_drv[tx_client];
                    char *tx_data = (char *) ((uintptr_t) tx_data_base + tx_data_offset);

                    // Blocking send!
                    int sent_bytes = send(sock_fd, tx_data, tx_buffer.len, 0);
                    if (sent_bytes != tx_buffer.len) {
                        LOG_NET_ERR("TX sent %d != expected %d. qutting.\n", sent_bytes, tx_buffer.len);
                        exit(1);
                    }

                    if (net_enqueue_free(&tx_queue, tx_buffer) != 0) {
                        LOG_NET_ERR("Couldn't return free TX buffer into the free queue. quiting\n");
                        exit(1);
                    }
                }

                uio_interrupt_ack(uio_sddf_net_tx_incoming_fd);
                *sddf_net_tx_outgoing_irq_fault_vaddr = 0;

            } else if (events[i].data.fd == uio_sddf_net_rx_incoming_fd) {
                // Got RX virt ntfn from VMM.
                // Don't care, we are grabbing the free bufs ourselves.
                printf("Got virt RX ntfn from VMM\n");
                uio_interrupt_ack(uio_sddf_net_rx_incoming_fd);
            } else {
                LOG_NET_WARN("epoll_wait() returned event on unknown fd %d\n", events[i].data.fd);
            }
        }
    }

    LOG_NET_WARN("event loop has break??\n");
    return 0;
}
