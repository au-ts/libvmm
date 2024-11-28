/*
 * Copyright 2024, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mount.h>
#include <linux/limits.h>

#include <lions/fs/protocol.h>

#include "log.h"
#include "op.h"
#include "util.h"

extern char blk_device[PATH_MAX];
extern char mnt_point[PATH_MAX];
extern char *fs_data;

bool mounted = false;

void fs_op_reply_success(uint64_t cmd_id, fs_cmpl_data_t result) {
    fs_queue_reply((fs_cmpl_t){ 
        .id = cmd_id,
        .status = FS_STATUS_SUCCESS,
        .data = result
    });
}

void fs_op_reply_failure(uint64_t cmd_id, uint64_t status, fs_cmpl_data_t result) {
    fs_queue_reply((fs_cmpl_t){ 
        .id = cmd_id,
        .status = status,
        .data = result
    });
}

void handle_initialise(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_initialise()\n");
    
    if (mounted) {
        LOG_FS_OPS("already mounted!\n");
        fs_queue_reply((fs_cmpl_t){ 
            .id = cmd.id,
            .status = FS_STATUS_ERROR,
            .data = {0}
        });
    }

    uint64_t status = FS_STATUS_ERROR;

    uint64_t cmd_size = sizeof(char) * ((PATH_MAX * 2) + 16);
    char *sh_mount_cmd = malloc(cmd_size);
    if (!sh_mount_cmd) {
        LOG_FS_ERR("out of memory\n");
        exit(EXIT_FAILURE);
    }

    snprintf(sh_mount_cmd, cmd_size, "mount %s %s", blk_device, mnt_point);
    LOG_FS_OPS("mounting with shell command: %s\n", sh_mount_cmd);

    if (system(sh_mount_cmd) == 0) {
        mounted = true;
        LOG_FS_OPS("block device at %s mounted at %s\n", blk_device, mnt_point);
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        LOG_FS_OPS("failed to mount block device at %s\n", blk_device);
        fs_op_reply_failure(cmd.id, FS_STATUS_ERROR, (fs_cmpl_data_t){0});
    }

    free(sh_mount_cmd);
}

void handle_deinitialise(fs_cmd_t cmd)
{

}
void handle_open(fs_cmd_t cmd)
{

}
void handle_stat(fs_cmd_t cmd)
{

}
void handle_fsize(fs_cmd_t cmd)
{

}
void handle_close(fs_cmd_t cmd)
{

}
void handle_read(fs_cmd_t cmd)
{

}
void handle_write(fs_cmd_t cmd)
{

}
void handle_rename(fs_cmd_t cmd)
{

}
void handle_unlink(fs_cmd_t cmd)
{

}
void handle_truncate(fs_cmd_t cmd)
{

}
void handle_mkdir(fs_cmd_t cmd)
{

}
void handle_rmdir(fs_cmd_t cmd)
{

}
void handle_opendir(fs_cmd_t cmd)
{

}
void handle_closedir(fs_cmd_t cmd)
{

}
void handle_readdir(fs_cmd_t cmd)
{

}
void handle_fsync(fs_cmd_t cmd)
{

}
void handle_seekdir(fs_cmd_t cmd)
{

}
void handle_telldir(fs_cmd_t cmd)
{

}
void handle_rewinddir(fs_cmd_t cmd)
{

}
