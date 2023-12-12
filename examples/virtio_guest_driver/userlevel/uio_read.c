/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{

    if (argc != 4) {
        printf("Usage: %s file dataport_size map_index\n\n"
               "Reads the c string contents of a specified dataport file to stdout\n",
               argv[0]);
        return 1;
    }

    char *dataport_name = argv[1];
    int length = atoi(argv[2]);
    assert(length > 0);

    int region = atoi(argv[3]);
    assert(region >= 0 && region < 3);

    printf("dataport name: %s, size: %d, region: %d\n", dataport_name, length, region);

    int fd = open(dataport_name, O_RDWR);
    assert(fd >= 0);

    char *dataport;
    if ((dataport = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, region * getpagesize())) == (void *) -1) {
        printf("mmap failed, errno: %d\n", errno);
        close(fd);
	return 1;
    }

    printf("%s\n", dataport);

    munmap(dataport, length);
    close(fd);

    return 0;
}
