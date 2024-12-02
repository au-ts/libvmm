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
#include <string.h>
#include <sys/mount.h>
#include <linux/limits.h>
#include <linux/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/syscall.h>

#include <uio/fs.h>
#include <lions/fs/protocol.h>

#include "log.h"
#include "op.h"
#include "util.h"

#define NAME_MAX_LEN 256

/* From main.c */
extern char blk_device[PATH_MAX];
extern char mnt_point[PATH_MAX];
extern char *fs_data;

/* Metadata with the mounted FS */
bool mounted = false;
/* Each read/write is bounded by the data share region size */
char inflight_data[UIO_LENGTH_FS_DATA];

/* A simple macro to abort FS operations if the FS isn't mounted */
#define CHECK_MOUNTED(cmd_id) {if(!mounted){fs_op_reply_failure(cmd_id, FS_STATUS_ERROR, (fs_cmpl_data_t){0});return;}}

#define TIMESPEC_TO_NS(ts) ((long long)ts.tv_sec * 1000000000LL + ts.tv_nsec)

/* Error checking wrapper around fs_malloc_create_path. Atomic to failures. */
char *prepare_path(uint64_t cmd_id, fs_buffer_t client_path) {
    size_t path_total_len;
    if (client_path.size > PATH_MAX) {
        fs_op_reply_failure(cmd_id, errno_to_lions_status(ENAMETOOLONG), (fs_cmpl_data_t){0});
        return NULL;
    }
    char *path = fs_malloc_create_path(client_path, &path_total_len);
    if (!path) {
        fs_op_reply_failure(cmd_id, errno_to_lions_status(ENOMEM), (fs_cmpl_data_t){0});
        return NULL;
    }
    if (path_total_len > PATH_MAX) {
        fs_op_reply_failure(cmd_id, errno_to_lions_status(ENAMETOOLONG), (fs_cmpl_data_t){0});
        free(path);
        return NULL;
    }
    return path;
}

void handle_initialise(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_initialise(): entry\n");
    
    if (mounted) {
        LOG_FS_OPS("handle_initialise(): already mounted!\n");
        fs_op_reply_failure(cmd.id, FS_STATUS_ERROR, (fs_cmpl_data_t){0});
        return;
    }

    /* Use the shell to mount the filesystem */
    uint64_t cmd_size = sizeof(char) * ((PATH_MAX * 2) + 16);
    char *sh_mount_cmd = malloc(cmd_size);
    if (!sh_mount_cmd) {
        LOG_FS_ERR("handle_initialise(): out of memory\n");
        exit(EXIT_FAILURE);
    }

    snprintf(sh_mount_cmd, cmd_size, "mount %s %s", blk_device, mnt_point);
    LOG_FS_OPS("handle_initialise(): mounting with shell command: %s\n", sh_mount_cmd);

    if (system(sh_mount_cmd) == 0) {
        mounted = true;
        LOG_FS_OPS("handle_initialise(): block device at %s mounted at %s\n", blk_device, mnt_point);
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        LOG_FS_OPS("handle_initialise(): failed to mount block device at %s\n", blk_device);
        fs_op_reply_failure(cmd.id, FS_STATUS_ERROR, (fs_cmpl_data_t){0});
    }

    free(sh_mount_cmd);
}

void handle_deinitialise(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_deinitialise(): entry\n");

    CHECK_MOUNTED(cmd.id);

    /* Use the shell to unmount the filesystem */
    uint64_t cmd_size = sizeof(char) * ((PATH_MAX) + 16);
    char *sh_umount_cmd = malloc(cmd_size);
    if (!sh_umount_cmd) {
        LOG_FS_ERR("handle_deinitialise(): out of memory\n");
        exit(EXIT_FAILURE);
    }

    snprintf(sh_umount_cmd, cmd_size, "umount %s", mnt_point);
    LOG_FS_OPS("handle_deinitialise(): mounting with shell command: %s\n", sh_umount_cmd);

    int umount_exit_code = system(sh_umount_cmd);
    switch(umount_exit_code) {
        case EXIT_SUCCESS: {
            mounted = false;
            LOG_FS_OPS("handle_deinitialise(): filesystem at %s, with backing block device at %s UNMOUNTED.\n", mnt_point, blk_device);
            fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
        }
        default: {
            // Assume that this is due to inflight operations. 
            fs_op_reply_failure(cmd.id, FS_STATUS_OUTSTANDING_OPERATIONS, (fs_cmpl_data_t){0});
        }
    }

    free(sh_umount_cmd);
}

void handle_open(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_open(): entry\n");

    struct fs_cmd_params_file_open params = cmd.params.file_open;
    char *path = prepare_path(cmd.id, params.path);

    LOG_FS_OPS("handle_open(): got path %s\n", path);
    int flags = 0;
    if (params.flags & FS_OPEN_FLAGS_CREATE) {
        flags |= O_CREAT;
    }

    if (params.flags & FS_OPEN_FLAGS_READ_WRITE) {
        flags |= O_RDWR;
    } else if (params.flags & FS_OPEN_FLAGS_READ_ONLY) {
        flags |= O_RDONLY;
    } else if (params.flags & FS_OPEN_FLAGS_WRITE_ONLY) {
        flags |= O_WRONLY;
    }

    int fd = open(path, flags);
    if (fd != -1) {
        fs_cmpl_data_t result;
        result.file_open.fd = fd;
        fs_op_reply_success(cmd.id, result);
    } else {
        fs_op_reply_failure(cmd.id, FS_STATUS_ERROR, (fs_cmpl_data_t){0});
    }

    free(path);
}

void handle_stat(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_stat(): entry\n");

    size_t path_total_len;
    fs_cmd_params_stat_t params = cmd.params.stat;
    if (params.path.size > PATH_MAX) {
        fs_op_reply_failure(cmd.id, errno_to_lions_status(ENAMETOOLONG), (fs_cmpl_data_t){0});
        return;
    }
    char *path = fs_malloc_create_path(params.path, &path_total_len);
    if (!path) {
        fs_op_reply_failure(cmd.id, errno_to_lions_status(ENOMEM), (fs_cmpl_data_t){0});
        return;
    }
    if (path_total_len > PATH_MAX) {
        fs_op_reply_failure(cmd.id, errno_to_lions_status(ENAMETOOLONG), (fs_cmpl_data_t){0});
        free(path);
        return;
    }
    if (params.buf.size < sizeof(fs_stat_t)) {
        fs_op_reply_failure(cmd.id, FS_STATUS_INVALID_BUFFER, (fs_cmpl_data_t){0});
        free(path);
        return;
    }

    LOG_FS_OPS("handle_stat(): got concatenated path %s\n", path);

    struct stat stat_data;
    int err = stat(path, &stat_data);

    if (err != -1) {
        fs_stat_t *resp_stat = (fs_stat_t *) fs_get_buffer(params.buf);
        resp_stat->dev = stat_data.st_dev;
        resp_stat->ino = stat_data.st_ino;
        resp_stat->mode = stat_data.st_mode;
        resp_stat->nlink = stat_data.st_nlink;
        resp_stat->uid = stat_data.st_uid;
        resp_stat->gid = stat_data.st_gid;
        resp_stat->rdev = stat_data.st_rdev;
        resp_stat->size = stat_data.st_size;
        resp_stat->blksize = stat_data.st_blksize;
        resp_stat->blocks = stat_data.st_blocks;

        resp_stat->atime = stat_data.st_atim.tv_sec;
        resp_stat->mtime = stat_data.st_mtim.tv_sec;
        resp_stat->ctime = stat_data.st_ctim.tv_sec;

        resp_stat->atime_nsec = TIMESPEC_TO_NS(stat_data.st_atim);
        resp_stat->mtime_nsec = TIMESPEC_TO_NS(stat_data.st_mtim);
        resp_stat->ctime_nsec = TIMESPEC_TO_NS(stat_data.st_ctim);

        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        int err_num = errno;
        LOG_FS_OPS("handle_stat(): fail with errno %d\n", err_num);
        if (err_num == ENOENT) {
            fs_op_reply_failure(cmd.id, FS_STATUS_INVALID_PATH, (fs_cmpl_data_t){0});
        }
    }

    free(path);
}

void handle_fsize(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_fsize(): entry\n");
    fs_cmd_params_file_size_t params = cmd.params.file_size;
    uint64_t fd = params.fd;

    off_t sz = lseek(fd, 0L, SEEK_END);
    if (sz > -1) {
        fs_cmpl_data_t result;
        result.file_size.size = sz;
        fs_op_reply_success(cmd.id, result);
    } else {
        fs_op_reply_failure(cmd.id, errno_to_lions_status(errno), (fs_cmpl_data_t){0});
    }
}

void handle_close(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_close(): entry\n");

    fs_cmd_params_file_close_t params = cmd.params.file_close;
    uint64_t fd = params.fd;

    errno = 0;
    close(fd);

    if (errno == 0) {
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        fs_op_reply_failure(cmd.id, errno_to_lions_status(errno), (fs_cmpl_data_t){0});
    }
}

void handle_read(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_read(): entry\n");
    fs_cmd_params_file_read_t params = cmd.params.file_read;
    
    uint64_t fd = params.fd;
    uint64_t count = params.buf.size;
    uint64_t off = params.offset;

    ssize_t bytes_read = pread(fd, inflight_data, count, off);
    if (bytes_read > -1) {
        fs_memcpy(fs_get_buffer(params.buf), inflight_data, bytes_read);
        fs_cmpl_data_t result;
        result.file_read.len_read = bytes_read;
        fs_op_reply_success(cmd.id, result);
    } else {
        fs_op_reply_failure(cmd.id, errno_to_lions_status(errno), (fs_cmpl_data_t){0});
    }
}

void handle_write(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_write(): entry\n");
    fs_cmd_params_file_write_t params = cmd.params.file_write;

    uint64_t fd = params.fd;
    uint64_t count = params.buf.size;
    uint64_t off = params.offset;

    LOG_FS_OPS("count = %p, off = %p, buff = %p\n", count, off, fs_get_buffer(params.buf));
    fs_memcpy(inflight_data, fs_get_buffer(params.buf), count);
    ssize_t bytes_written = pwrite(fd, inflight_data, count, off);
    if (bytes_written > -1) {
        fs_cmpl_data_t result;
        result.file_write.len_written = bytes_written;
        fs_op_reply_success(cmd.id, result);
    } else {
        fs_op_reply_failure(cmd.id, errno_to_lions_status(errno), (fs_cmpl_data_t){0});
    }
}

void handle_rename(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_rename(): entry\n");
    fs_cmd_params_rename_t params = cmd.params.rename;

    char *src_path = prepare_path(cmd.id, params.old_path);
    char *dst_path = prepare_path(cmd.id, params.new_path);
    if (!src_path || !dst_path) {
        return;
    }

    if (rename(src_path, dst_path) == 0) {
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        int err_num = errno;
        LOG_FS_OPS("handle_rename(): fail with errno %d\n", err_num);
        fs_op_reply_failure(cmd.id, errno_to_lions_status(err_num), (fs_cmpl_data_t){0});
    }

    free(src_path);
    free(dst_path);
}

void handle_unlink(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_unlink(): entry\n");
    fs_cmd_params_file_remove_t params = cmd.params.file_remove;
    char *path = prepare_path(cmd.id, params.path);
    if (!path) {
        return;
    }

    LOG_FS_OPS("handle_unlink(): got concatenated path %s\n", path);

    if (unlink(path) == 0) {
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        int err_num = errno;
        LOG_FS_OPS("handle_unlink(): fail with errno %d\n", err_num);
        fs_op_reply_failure(cmd.id, errno_to_lions_status(err_num), (fs_cmpl_data_t){0});
    }

    free(path);
}

void handle_truncate(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_truncate(): entry\n");

    fs_cmd_params_file_truncate_t params = cmd.params.file_truncate;
    uint64_t fd = params.fd;
    uint64_t len = params.length;

    if (ftruncate(fd, len) == 0) {
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        int err_num = errno;
        LOG_FS_OPS("handle_truncate(): fail with errno %d\n", err_num);
        fs_op_reply_failure(cmd.id, errno_to_lions_status(err_num), (fs_cmpl_data_t){0});
    }
}

void handle_mkdir(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_mkdir(): entry\n");
    /* Concatenate the client provided path with the mount point. */
    fs_cmd_params_dir_remove_t params = cmd.params.dir_remove;
    char *path = prepare_path(cmd.id, params.path);
    if (!path) {
        return;
    }

    LOG_FS_OPS("handle_mkdir(): got concatenated path %s\n", path);

    if (mkdir(path, 0) == 0) {
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        int err_num = errno;
        LOG_FS_OPS("handle_mkdir(): fail with errno %d\n", err_num);
        fs_op_reply_failure(cmd.id, errno_to_lions_status(err_num), (fs_cmpl_data_t){0});
    }

    free(path);
}

void handle_rmdir(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_rmdir(): entry\n");

    /* Concatenate the client provided path with the mount point. */
    fs_cmd_params_dir_remove_t params = cmd.params.dir_remove;
    char *path = prepare_path(cmd.id, params.path);
    if (!path) {
        return;
    }

    LOG_FS_OPS("handle_rmdir(): got concatenated path %s\n", path);

    if (rmdir(path) == 0) {
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        int err_num = errno;
        LOG_FS_OPS("handle_opendir(): fail with errno %d\n", err_num);
        fs_op_reply_failure(cmd.id, errno_to_lions_status(err_num), (fs_cmpl_data_t){0});
    }

    free(path);
}

void handle_opendir(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_opendir(): entry\n");

    /* Concatenate the client provided path with the mount point. */
    fs_cmd_params_dir_open_t params = cmd.params.dir_open;
    char *path = prepare_path(cmd.id, params.path);
    if (!path) {
        return;
    }

    LOG_FS_OPS("handle_opendir(): got concatenated path %s\n", path);

    /* Using opendir instead of open for better portability. */
    DIR *dir_stream = opendir(path);
    if (dir_stream) {
        LOG_FS_OPS("handle_opendir(): ok\n");
        fs_cmpl_data_t result;
        /* This is very sketchy, setting the "fd" as the pointer to dir_stream. */
        result.dir_open.fd = (uint64_t) dir_stream;
        fs_op_reply_success(cmd.id, result);
    } else {
        int err_num = errno;
        LOG_FS_OPS("handle_opendir(): fail with errno %d\n", err_num);
        fs_op_reply_failure(cmd.id, errno_to_lions_status(err_num), (fs_cmpl_data_t){0});
    }

    free(path);
}

void handle_closedir(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_closedir(): entry\n");
    fs_cmd_params_dir_close_t params = cmd.params.dir_close;
    DIR *dir_stream = (DIR *) params.fd;

    if (closedir(dir_stream) == 0) {
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        int err_num = errno;
        LOG_FS_OPS("handle_closedir(): fail with errno %d\n", err_num);
        fs_op_reply_failure(cmd.id, errno_to_lions_status(err_num), (fs_cmpl_data_t){0});
    }
}

void handle_readdir(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_readdir(): entry\n");
    fs_cmd_params_dir_read_t params = cmd.params.dir_read;
    DIR *dir_stream = (DIR *) params.fd;
    char *path = fs_get_buffer(params.buf);

    if (params.buf.size < NAME_MAX_LEN) {
        LOG_FS_OPS("handle_readdir(): client buf not big enough: %d < %d\n", params.buf.size, NAME_MAX_LEN);
        fs_op_reply_failure(cmd.id, FS_STATUS_INVALID_BUFFER, (fs_cmpl_data_t){0});
    }

    errno = 0;
    struct dirent *entry = readdir(dir_stream);
    if (!entry) {
        if (errno == 0) {
            fs_op_reply_failure(cmd.id, FS_STATUS_END_OF_DIRECTORY, (fs_cmpl_data_t){0});
            return;
        } else {
            int err_num = errno;
            LOG_FS_OPS("handle_readdir(): fail with errno %d\n", err_num);
            fs_op_reply_failure(cmd.id, errno_to_lions_status(err_num), (fs_cmpl_data_t){0});
            return;
        }
    }

    size_t name_len = strlen(entry->d_name);
    strcpy(path, entry->d_name);

    fs_cmpl_data_t result;
    memset(&result, 0, sizeof(result));
    result.dir_read.path_len = name_len;
    fs_op_reply_success(cmd.id, result);
}

void handle_fsync(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_fsync(): entry\n");

    fs_cmd_params_file_sync_t params = cmd.params.file_sync;
    uint64_t fd = params.fd;

    errno = 0;
    fsync(fd);

    if (errno == 0) {
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        fs_op_reply_failure(cmd.id, errno_to_lions_status(errno), (fs_cmpl_data_t){0});
    }
}

void handle_seekdir(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_seekdir(): entry\n");

    fs_cmd_params_dir_seek_t params = cmd.params.dir_seek;
    DIR *dir_stream = (DIR *) params.fd;
    int64_t loc = params.loc;

    errno = 0;
    seekdir(dir_stream, loc);

    if (errno == 0) {
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        fs_op_reply_failure(cmd.id, errno_to_lions_status(errno), (fs_cmpl_data_t){0});
    }
}

void handle_telldir(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_telldir(): entry\n");

    fs_cmd_params_dir_tell_t params = cmd.params.dir_tell;
    DIR *dir_stream = (DIR *) params.fd;

    long pos = telldir(dir_stream);
    if (pos != -1) {
        fs_cmpl_data_t result;
        memset(&result, 0, sizeof(result));
        result.dir_tell.location = pos;
        fs_op_reply_success(cmd.id, result);
    } else {
        int err_num = errno;
        LOG_FS_OPS("handle_telldir(): fail with errno %d\n", err_num);
        fs_op_reply_failure(cmd.id, errno_to_lions_status(err_num), (fs_cmpl_data_t){0});
    }
}

void handle_rewinddir(fs_cmd_t cmd)
{
    LOG_FS_OPS("handle_rewinddir(): entry\n");

    fs_cmd_params_dir_rewind_t params = cmd.params.dir_rewind;
    DIR *dir_stream = (DIR *) params.fd;

    errno = 0;
    rewinddir(dir_stream);

    if (errno == 0) {
        fs_op_reply_success(cmd.id, (fs_cmpl_data_t){0});
    } else {
        fs_op_reply_failure(cmd.id, errno_to_lions_status(errno), (fs_cmpl_data_t){0});
    }
}
