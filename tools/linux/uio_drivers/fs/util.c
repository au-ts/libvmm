/*
 * Copyright 2024, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

#include <lions/fs/protocol.h>
#include <uio/fs.h>

#include "log.h"

extern char mnt_point[PATH_MAX];
extern struct fs_queue *comp_queue;
extern char *fs_data;

void fs_queue_reply(fs_cmpl_t cmpl) {
    assert(fs_queue_length_producer(comp_queue) != FS_QUEUE_CAPACITY);
    fs_queue_idx_empty(comp_queue, 0)->cmpl = cmpl;
    fs_queue_publish_production(comp_queue, 1);
}

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

void *fs_get_buffer(fs_buffer_t buf) {
    if (buf.offset >= UIO_LENGTH_FS_DATA
        || buf.size > UIO_LENGTH_FS_DATA - buf.offset
        || buf.size == 0) {
        return NULL;
    }
    return (void *)(fs_data + buf.offset);
}

char *fs_malloc_create_path(fs_buffer_t params_path, size_t *path_len) {
    size_t path_size = strlen(mnt_point) + params_path.size + 2; // extra slash and terminator

    char *path = malloc(path_size);
    if (!path) {
        return NULL;
    }

    strcpy(path, mnt_point);
    strcat(path, "/");
    strncat(path, fs_get_buffer(params_path), params_path.size);

    *path_len = path_size;
    return path;
}

void fs_memcpy(char *dest, const char *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dest[i] = src[i];
    }
}

uint64_t errno_to_lions_status(int err_num) {
    switch (errno) {
        case ENOENT: {
            return FS_STATUS_INVALID_PATH;
        }
        case EACCES: {
            return FS_STATUS_SERVER_WAS_DENIED;
        }
        case EBADF: {
            return FS_STATUS_INVALID_FD;
        }
        case EFAULT: {
            return FS_STATUS_INVALID_BUFFER;
        }
        case EMFILE:
        case ENFILE: {
            return FS_STATUS_TOO_MANY_OPEN_FILES;
        }
        case ENOMEM: {
            return FS_STATUS_ALLOCATION_ERROR;
        }
        case EOVERFLOW:
        case ENAMETOOLONG: {
            return FS_STATUS_INVALID_NAME;
        }
        case ENOSPC: {
            return FS_STATUS_DIRECTORY_IS_FULL;
        }
        case EBUSY: {
            return FS_STATUS_OUTSTANDING_OPERATIONS;
        }
        default:
            return FS_STATUS_ERROR;
    }
}
