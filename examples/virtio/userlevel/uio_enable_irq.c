/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{

    if (argc != 4) {
        printf("Usage: %s file dataport_size map_index\n\n"
               "Enables the IRQ of a certain UIO device\n",
               argv[0]);
        return 1;
    }

    char *dataport_name = argv[1];
    int length = atoi(argv[2]);
    assert(length > 0);

    int region = atoi(argv[3]);
    assert(region >= 0 && region < 3);

    int fd = open(dataport_name, O_RDWR);
    assert(fd >= 0);
    int64_t one = 1;
    int ret = write(fd, &one, 4);
    printf("ret val: %d, errno: %d\n", ret, errno);
    close(fd);

    return 0;
}
