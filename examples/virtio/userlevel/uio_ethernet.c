/*
 * TODO: license
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define DATAPORT_SIZE                   8192
#define VIRTIO_BLK_SIZE_MAX             (DATAPORT_SIZE - (sizeof(guest_sataserver_method_t) + 2 * sizeof(unsigned int)))
#define VIRTIO_BLK_DISK_BLK_SIZE        512
#define MAX_CLIENT_NUM                  16      // TODO: what should it be?
#define MAX_CLIENT_SECTOR               2048000      // TODO: what should it be?


#define SATASERVER_STATUS_GOOD          0
#define SATASERVER_STATUS_NOT_DONE      1
#define SATASERVER_STATUS_INVALID_CONF  2

typedef enum {
    /* used by the server (me) */
    SERVER_TX,
    SERVER_RX,
    SERVER_CAPACITY,
    SERVER_STATUS,

    /* use by the clients */
    CLIENT_TX,
    CLIENT_RX,
    CLIENT_CAPACITY,
    CLIENT_STATUS,

    NUM_METHOD
} guest_sataserver_method_t;

/* this structure should be mapped into the dataport */
typedef struct guest_sataserver_handler {
    guest_sataserver_method_t method;
    unsigned int value;                 /* sector/capacity/status, depends on the method */
    unsigned int len;                   /* len/bytes read/bytes written, depends on the method */
    char buffer[VIRTIO_BLK_SIZE_MAX];
} __attribute__((packed)) guest_sataserver_handler_t;

// static_assert(sizeof(guest_sataserver_handler_t) == DATAPORT_SIZE, "Something went wrong: ...");

typedef struct connection_info {
    char *connection;                   /* we need this to signal the client */
    int blocks_fd;                      /* the file that we keep client's data in */
} connection_info_t;

// static_assert(sizeof(guest_sataserver_handler_t) == DATAPORT_SIZE, "Something went wrong...");

volatile static guest_sataserver_handler_t **handlers = NULL;
volatile static struct pollfd *fds = NULL;
static connection_info_t *connections = NULL;

static inline void ready_emit(int client_idx)
{
    connections[client_idx].connection[0] = 1;

    // int ret = msync(handlers[client_idx], DATAPORT_SIZE, MS_SYNC);
    // if (ret < 0) {
    //     printf("msync [%p] failed with error: %s\n", handlers[client_idx], strerror(errno));
    // }
}

static int find_victim(int num_conns)
{
    for (int i = 0; i < num_conns; i++) {
        if (fds[i].revents != 0) {
            return i;
        }
    }
    return -1;
}

unsigned int handle_tx(int client_idx)
{
    assert(handlers[client_idx]->len % VIRTIO_BLK_DISK_BLK_SIZE == 0);

    off_t offset = lseek(connections[client_idx].blocks_fd, handlers[client_idx]->value * VIRTIO_BLK_DISK_BLK_SIZE, SEEK_SET);
    if (offset < 0) {
        printf("lseek failed with error: %s\n", strerror(errno));
        return 0;
    }

    if (offset != handlers[client_idx]->value * VIRTIO_BLK_DISK_BLK_SIZE) {
        printf("weird things happened in lseek, expected offset: %d, returned offset: %lu",
               handlers[client_idx]->value * VIRTIO_BLK_DISK_BLK_SIZE, offset);
        return 0;
    }

    ssize_t byte_written = write(connections[client_idx].blocks_fd, (char *)handlers[client_idx]->buffer, handlers[client_idx]->len);
    if (byte_written < 0) {
        printf("write failed with error: %s\n", strerror(errno));
        return 0;
    }
    return byte_written;
}

unsigned int handle_rx(int client_idx)
{
    assert(handlers[client_idx]->len % VIRTIO_BLK_DISK_BLK_SIZE == 0);

    off_t offset = lseek(connections[client_idx].blocks_fd, handlers[client_idx]->value * VIRTIO_BLK_DISK_BLK_SIZE, SEEK_SET);
    if (offset < 0) {
        printf("lseek failed with error: %s\n", strerror(errno));
        return 0;
    }

    if (offset != handlers[client_idx]->value * VIRTIO_BLK_DISK_BLK_SIZE) {
        printf("weird things happened in lseek, expected offset: %d, returned offset: %lu",
               handlers[client_idx]->value * VIRTIO_BLK_DISK_BLK_SIZE, offset);
        return 0;
    }

    ssize_t byte_read = read(connections[client_idx].blocks_fd, (char *)handlers[client_idx]->buffer, handlers[client_idx]->len);
    if (byte_read < 0) {
        printf("read failed with error: %s\n", strerror(errno));
        return 0;
    }
    return byte_read;
}

static int serve_victim(int client_idx)
{

    int ret = 0;
    switch(handlers[client_idx]->method) {
        case CLIENT_TX:
            printf("handled CLIENT_TX (sector: %d, len: %d) for client %d\n",
                    handlers[client_idx]->value, handlers[client_idx]->len, client_idx);

            handlers[client_idx]->method = SERVER_TX;
            // handlers[client_idx]->len = handle_tx(client_idx);
            // printf("byte_written: %d\n", handlers[client_idx]->len);

            break;
        case CLIENT_RX:
            printf("handled CLIENT_RX (sector: %d, len: %d) for client %d\n",
                    handlers[client_idx]->value, handlers[client_idx]->len, client_idx);

            handlers[client_idx]->method = SERVER_RX;
            // handlers[client_idx]->len = handle_rx(client_idx);
            // printf("byte_read: %d\n", handlers[client_idx]->len);

            break;
        case CLIENT_CAPACITY:
            // printf("handled CLIENT_CAPACITY for client %d\n", client_idx);

            handlers[client_idx]->method = SERVER_CAPACITY;
            handlers[client_idx]->value = MAX_CLIENT_SECTOR;

            break;

        case CLIENT_STATUS:
            // printf("handled CLIENT_STATUS for client %d\n", client_idx);

            handlers[client_idx]->method = SERVER_STATUS;
            handlers[client_idx]->value = SATASERVER_STATUS_GOOD;

            break;

        default:
            printf("Mysterious method %d from client %d\n", handlers[client_idx]->method, client_idx);
            ret = -1;
    }

    return ret;
}

int main(int argc, char *argv[])
{
    assert(DATAPORT_SIZE % getpagesize() == 0);

    if (argc != 2) {
        printf("Usage: %s #clients\n\n", argv[0]);
        return -1;
    }

    int num_conns = atoi(argv[1]);
    if (num_conns > MAX_CLIENT_NUM) {
        printf("Too many clients\n");
        return -1;
    }

    fds = calloc(num_conns, sizeof(struct pollfd));
    handlers = calloc(num_conns, sizeof(guest_sataserver_handler_t *));
    connections = calloc(num_conns, sizeof(connection_info_t));

    for (int i = 0; i < num_conns; i++) {
        char dataport_name[20];
        sprintf(dataport_name, "/dev/uio%d", i);

        fds[i].fd = open(dataport_name, O_RDWR);
        assert(fds[i].fd >= 0);

        fds[i].events = POLLIN;

        /* mmap the actually dataport */
        if ((handlers[i] = mmap(NULL, DATAPORT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fds[i].fd, 1 * getpagesize())) == (void *) -1) {
            printf("mmap failed for /dev/uio%d\n.", i);
            goto clean_up;
        }

        /* mmap the irq handler register */
        if ((connections[i].connection = mmap(NULL, DATAPORT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fds[i].fd, 0 * getpagesize())) == (void *) -1) {
            printf("mmap failed for /dev/uio%d\n.", i);
            goto clean_up;
        }

        /* create a file to put data in */
        // char client_file_name[25];
        // sprintf(client_file_name, "/mnt/data/file-image-%d", i);
        // connections[i].blocks_fd = open(client_file_name, O_RDWR);
        // assert(connections[i].blocks_fd >= 0);
    }

    /* ping everybody in case we already missed some interrupts */
    for (int i = 0; i < num_conns; i++) {
        ready_emit(i);
    }

    while (num_conns) {
        int num_victims = poll((struct pollfd *)fds, num_conns, -1);
        assert(num_victims != 0);
        if (num_victims < 0) {
            printf("poll failed with error: %s\n", strerror(errno));
        }
        printf("num_victims: %d, revents v0: %d, revents v1: %d\n", num_victims, fds[0].revents, fds[1].revents);

        while (num_victims) {
            int victim_idx = find_victim(num_conns);
            assert(victim_idx >= 0);
            assert(fds[victim_idx].revents == POLLIN);
            // printf("victim_idx: %d\n", victim_idx);

            /* ack the irq */
            int irq_count;
            int read_ret = read(fds[victim_idx].fd, &irq_count, sizeof(irq_count));
            assert(read_ret >= 0);

            fds[victim_idx].revents = 0; /* not sure if this is needed */
            num_victims--;

            int result = serve_victim(victim_idx);
            assert(result != -1);
            ready_emit(victim_idx);
         }
    }

clean_up:
    // TODO: proper clean up
    for (int i = 0; i < num_conns; i++) {
        if (fds[i].fd > 0) {
            munmap((void *)handlers[i], DATAPORT_SIZE);
            munmap((void *)connections[i].connection, DATAPORT_SIZE);
            close(fds[i].fd);

        } else {
            break;
        }
    }
    return 0;
}