/*
 * Copyright 2024, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <lions/fs/protocol.h>

void handle_initialise(fs_cmd_t cmd);
void handle_deinitialise(fs_cmd_t cmd);
void handle_open(fs_cmd_t cmd);
void handle_stat(fs_cmd_t cmd);
void handle_fsize(fs_cmd_t cmd);
void handle_close(fs_cmd_t cmd);
void handle_read(fs_cmd_t cmd);
void handle_write(fs_cmd_t cmd);
void handle_rename(fs_cmd_t cmd);
void handle_unlink(fs_cmd_t cmd);
void handle_truncate(fs_cmd_t cmd);
void handle_mkdir(fs_cmd_t cmd);
void handle_rmdir(fs_cmd_t cmd);
void handle_opendir(fs_cmd_t cmd);
void handle_closedir(fs_cmd_t cmd);
void handle_readdir(fs_cmd_t cmd);
void handle_fsync(fs_cmd_t cmd);
void handle_seekdir(fs_cmd_t cmd);
void handle_telldir(fs_cmd_t cmd);
void handle_rewinddir(fs_cmd_t cmd);

static void (*const cmd_handler[FS_NUM_COMMANDS])(fs_cmd_t cmd) = {
    [FS_CMD_INITIALISE] = handle_initialise,
    [FS_CMD_DEINITIALISE] = handle_deinitialise,
    [FS_CMD_FILE_OPEN] = handle_open,
    [FS_CMD_FILE_CLOSE] = handle_close,
    [FS_CMD_STAT] = handle_stat,
    [FS_CMD_FILE_READ] = handle_read,
    [FS_CMD_FILE_WRITE] = handle_write,
    [FS_CMD_FILE_SIZE] = handle_fsize,
    [FS_CMD_RENAME] = handle_rename,
    [FS_CMD_FILE_REMOVE] = handle_unlink,
    [FS_CMD_FILE_TRUNCATE] = handle_truncate,
    [FS_CMD_DIR_CREATE] = handle_mkdir,
    [FS_CMD_DIR_REMOVE] = handle_rmdir,
    [FS_CMD_DIR_OPEN] = handle_opendir,
    [FS_CMD_DIR_CLOSE] = handle_closedir,
    [FS_CMD_FILE_SYNC] = handle_fsync,
    [FS_CMD_DIR_READ] = handle_readdir,
    [FS_CMD_DIR_SEEK] = handle_seekdir,
    [FS_CMD_DIR_TELL] = handle_telldir,
    [FS_CMD_DIR_REWIND] = handle_rewinddir,
};
