/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include <unistd.h>

#define NUMS_TO_RECV (16384)
/* 32k buffers of 2-bytes unsigneds. */
uint16_t nums[NUMS_TO_RECV];

_Static_assert(NUMS_TO_RECV <= UINT16_MAX);

int main(int argc, char *argv[])
{
    printf("VSOCK RECV|INFO: starting\n");

    if (argc != 2) {
        printf("./vsock_recv.elf <CID>\n");
        return 1;
    }
    uint32_t cid = atoi(argv[1]);
    printf("VSOCK RECV|INFO: creating socket to wait on CID %d\n", cid);

    int s = socket(AF_VSOCK, SOCK_STREAM, 0);

    struct sockaddr_vm addr;
    memset(&addr, 0, sizeof(struct sockaddr_vm));
    addr.svm_family = AF_VSOCK;
    addr.svm_port = 9999;
    addr.svm_cid = cid;

    int r = bind(s, &addr, sizeof(struct sockaddr_vm));
    if (r) {
        printf("VSOCK RECV|ERROR: bind failed with '%s'\n", strerror(errno));
        return 1;
    }

    r = listen(s, 0);
    if (r) {
        printf("VSOCK RECV|ERROR: listen failed with '%s'\n", strerror(errno));
        return 1;
    }

    struct sockaddr_vm peer_addr;
    socklen_t peer_addr_size = sizeof(struct sockaddr_vm);
    int peer_fd = accept(s, &peer_addr, &peer_addr_size);

    printf("VSOCK RECV|INFO: peer connected\n");

    size_t bytes_recved = 0;
    char *buf = (char *) nums;
    while ((bytes_recved += recv(peer_fd, &buf[bytes_recved], sizeof(nums), 0)) != sizeof(nums)) {
        printf("VSOCK RECV|INFO: Accumulatively received %lu bytes\n", bytes_recved);
    }

    printf("VSOCK RECV|INFO: Total bytes received %lu, verifying data...\n", sizeof(nums));

    for (uint16_t i = 0; i < NUMS_TO_RECV; i++) {
        if (nums[i] != i) {
            printf("VSOCK RECV|ERROR: at sequence %u, but got %u!\n", i, nums[i]);
            return 1;
        }
    }

    printf("VSOCK RECV|INFO: All is well in the universe\n");

    return 0;
}
