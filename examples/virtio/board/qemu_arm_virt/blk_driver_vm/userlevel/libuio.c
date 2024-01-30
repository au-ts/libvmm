/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdint.h>
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
#include <sddf/blk/shared_queue.h>

#include "libuio.h"
#include "block.h"

/*
 * the kernel UIO driver we use (uio_pdrv_genirq) only handles one IRQ source. In the case that
 * we have multiple channels, we have to store the channel id somewhere in the shared memorys
 */
typedef struct connection_info {
    int32_t irq;                        /* we need this to notify */
    microkit_channel notify_ch;         /* the channel number to notify */
    microkit_channel notified_ch;       /* the channel number to be notified */
} connection_info_t;

static struct pollfd pfd;
static connection_info_t *conn;

/*
 * Just happily abort if the user can't be bother to provide these functions
 */
__attribute__((weak)) void init(int fd)
{
    assert(!"should not be here!");
}

__attribute__((weak)) void notified(microkit_channel ch)
{
    assert(!"should not be here!");
}

void microkit_notify(microkit_channel ch)
{
    conn->notify_ch = ch;

    // this *should* re-enable the IRQ according to the UIO spec. Not sure why it doesn't
    // conn->sirq = 1;

    // writing directly to the file also re-enables the IRQ
    int32_t one = 1;
    int ret = write(pfd.fd, &one, 4);
    if (ret < 0) {
        printf("ret val: %d, errno: %d\n", ret, errno);
    }
    fsync(pfd.fd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <uio_device_name> <#mapping>\n\n"
               "TODO(@jade): say something useful here\n",
               argv[0]);
        return 1;
    }

    char *uio_device_name = argv[1];
    int num_mapping = atoi(argv[2]);
    assert(num_mapping > 0);

    printf("UIO device name: %s, number of mappings: %d\n", uio_device_name, num_mapping);

    // get the file descriptor for polling
    pfd.fd = open(uio_device_name, O_RDWR);
    assert(pfd.fd >= 0);

    // the event we are polling for
    pfd.events = POLLIN;

    /* mmap the irq handler register */
    if ((conn = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, pfd.fd, 0 * getpagesize())) == (void *) -1) {
        printf("mmap failed, errno: %d\n", errno);
        close(pfd.fd);
    }

    /* tell the guest driver to initialize some queues */
    init(pfd.fd);

    // The IRQ is likly disabled by Linux, in this case we need to re-enable the IRQ
    microkit_notify(0);

    while (true) {
        // poll() returns when there is something to read, in our case, when there is an IRQ occur.
        // poll() doesn't do anything with the IRQ.
        int num_victims = poll(&pfd, 1, -1);

        // TODO(@jade): handle this gracefully
        assert(num_victims != 0);
        assert(num_victims != -1);
        assert(pfd.revents == POLLIN);

        // actually ACK the IRQ by performing a read()
        int irq_count;
        int read_ret = read(pfd.fd, &irq_count, sizeof(irq_count));
        assert(read_ret >= 0);
        printf("receives irq, count: %d\n", irq_count);

        /* clear the return event(s). */
        pfd.revents = 0;

        /* wake the guest driver up to do some real works */
        notified(conn->notified_ch);
    }

    return 0;
}
