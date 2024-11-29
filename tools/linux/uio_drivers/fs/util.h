/*
 * Copyright 2024, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdlib.h>

#include <lions/fs/protocol.h>

/* Enqueue a reply into the completion queue */
void fs_queue_reply(fs_cmpl_t cmpl);
/* Thin wrappers for the above */
void fs_op_reply_success(uint64_t cmd_id, fs_cmpl_data_t result);
void fs_op_reply_failure(uint64_t cmd_id, uint64_t status, fs_cmpl_data_t result);

/* Convert a fs_buffer_t into our vaddr */
void *fs_get_buffer(fs_buffer_t buf);
/* Copy the path from client then concat it with the mount point. 
   If the call succeeds, the path's total length will be written to *path_len.
   *** Returns a buffer from malloc. *** */
char *fs_malloc_create_path(fs_buffer_t path, size_t *path_len);

/* Byte-by-byte memcpy as the string.h's memcpy is not compatible with UIO mappings */
void fs_memcpy(char *dest, const char *src, size_t n);

/* Converts the errno into LionsOS' equivalent status. */
uint64_t errno_to_lions_status(int err_num);
