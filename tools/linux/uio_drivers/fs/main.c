/*
 * Copyright 2024, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "log.h"

#define ARGC_REQURED 3

int main(int argc, char **argv)
{
    if (argc != ARGC_REQURED) {
        LOG_ERR_FS("usage: ./uio_fs_driver <blk_device> <mount_point>");
        exit(EXIT_FAILURE);
    } else {
        LOG_FS("*** Starting up\n");
        LOG_FS("Block device: %s\n", argv[1]);
        LOG_FS("Mount point: %s\n", argv[2]);
    }
    char *blk_device = argv[1];
    char *mnt_point = argv[2];

    LOG_FS("*** Starting up\n");

    LOG_FS_WARN("Exit\n");
    return EXIT_SUCCESS;
}
