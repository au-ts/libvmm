/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdio.h>

#define DEBUG_FS

#if defined(DEBUG_FS)
#define LOG_FS(...) do{ fprintf(stderr, "UIO(FS): "); fprintf(stderr, __VA_ARGS__); }while(0)
#else
#define LOG_FS(...) do{}while(0)
#endif

#define LOG_FS_ERR(...) do{ fprintf(stderr, "UIO(FS)|ERROR: "); fprintf(stderr, __VA_ARGS__); }while(0)
#define LOG_FS_WARN(...) do{ fprintf(stderr, "UIO(FS)|WARN: "); fprintf(stderr, __VA_ARGS__); }while(0)
