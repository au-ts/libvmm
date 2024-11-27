/*
 * Copyright 2024, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/epoll.h>

#include <lions/fs/protocol.h>
#include <uio/fs.h>

#include "log.h"

#define ARGC_REQURED 3

/* Event queue for polling */
#define MAX_EVENTS 16 // Arbitrary length
struct epoll_event events[MAX_EVENTS];

int create_epoll(void)
{
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("create_epoll(): epoll_create1()");
        LOG_FS_ERR("can't create the epoll fd.\n");
        exit(EXIT_FAILURE);
    }
    return epoll_fd;
}

void bind_fd_to_epoll(int fd, int epollfd)
{
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) == -1) {
        perror("bind_fd_to_epoll(): epoll_ctl()");
        LOG_FS_ERR("can't register fd %d to epoll fd %d.\n", fd, epollfd);
        exit(EXIT_FAILURE);
    }
}

int open_uio(const char *abs_path)
{
    int fd = open(abs_path, O_RDWR);
    if (fd == -1) {
        perror("open_uio(): open()");
        LOG_FS_ERR("can't open uio @ %s.\n", abs_path);
        exit(EXIT_FAILURE);
    }
    return fd;
}

char *map_uio(uint64_t length, int uiofd)
{
    void *base = (char *) mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, uiofd, 0);
    if (base == MAP_FAILED) {
        perror("map_uio(): mmap()");
        LOG_FS_ERR("can't mmap uio fd %d\n", uiofd);
        exit(EXIT_FAILURE);
    }
    return (char *) base;
}

void uio_interrupt_ack(int uiofd)
{
    uint32_t enable = 1;
    if (write(uiofd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
        perror("uio_interrupt_ack(): write()");
        LOG_FS_ERR("Failed to write enable/ack interrupts on uio fd %d\n", uiofd);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    if (argc != ARGC_REQURED) {
        LOG_FS_ERR("usage: ./uio_fs_driver <blk_device> <mount_point>");
        exit(EXIT_FAILURE);
    } else {
        LOG_FS("*** Starting up\n");
        LOG_FS("Block device: %s\n", argv[1]);
        LOG_FS("Mount point: %s\n", argv[2]);
    }
    char *blk_device = argv[1];
    char *mnt_point = argv[2];

    LOG_FS("*** Setting up command queue via UIO\n");
    int cmd_uio_fd = open_uio(UIO_PATH_FS_COMMAND_QUEUE_AND_IRQ);
    struct fs_queue *cmd_queue = (struct fs_queue *) map_uio(UIO_LENGTH_FS_COMMAND_QUEUE, cmd_uio_fd);
    
    LOG_FS("*** Setting up completion queue via UIO\n");
    int comp_uio_fd = open_uio(UIO_PATH_FS_COMPLETION_QUEUE);
    struct fs_queue *comp_queue = (struct fs_queue *) map_uio(UIO_LENGTH_FS_COMPLETION_QUEUE, comp_uio_fd);
    
    LOG_FS("*** Setting up FS data region via UIO\n");
    int fs_data_uio_fd = open_uio(UIO_PATH_FS_DATA);
    char *fs_data = map_uio(UIO_LENGTH_FS_DATA, fs_data_uio_fd);

    LOG_FS("*** Setting up fault region via UIO\n");
    // For Guest -> VMM notifications
    int fault_uio_fd = open_uio(UIO_PATH_GUEST_TO_VMM_NOTIFY_FAULT);
    char *vmm_notify_fault = map_uio(UIO_LENGTH_GUEST_TO_VMM_NOTIFY_FAULT, fault_uio_fd);

    LOG_FS("*** Enabling UIO interrupt on command queue\n");
    uio_interrupt_ack(cmd_uio_fd);

    LOG_FS("*** Creating epoll object\n");
    int epoll_fd = create_epoll();

    LOG_FS("*** Binding command queue IRQ to epoll\n");
    bind_fd_to_epoll(cmd_uio_fd, epoll_fd);

    LOG_FS("*** All initialisation successful!\n");
    LOG_FS("*** You won't see any output from UIO FS anymore. Unless there is a warning or error.\n");

    while (1) {
        int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_events == -1) {
            perror("main(): epoll_wait():");
            LOG_FS("epoll wait failed\n");
            exit(EXIT_FAILURE);
        }
        if (n_events == MAX_EVENTS) {
            LOG_FS_WARN("epoll_wait() returned MAX_EVENTS, there maybe dropped events!\n");
        }

        for (int i = 0; i < n_events; i++) {
            if (!(events[i].events & EPOLLIN)) {
                LOG_FS_WARN("got non EPOLLIN event on fd %d\n", events[i].data.fd);
                continue;
            }

            if (events[i].data.fd == cmd_uio_fd) {
                // Got command!
                continue; // for now
            } else {
                LOG_FS_WARN("epoll_wait() returned event on unknown fd %d\n", events[i].data.fd);
            }
        }
    }

    LOG_FS_WARN("Exit\n");
    return EXIT_SUCCESS;
}
