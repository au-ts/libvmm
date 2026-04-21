/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <string.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

extern bool fault_cond;

// #define DEBUG_FAULT
#if defined(DEBUG_FAULT)
#define LOG_FAULT(...) do{ if (fault_cond) { printf("%s|FAULT: ", microkit_name); printf(__VA_ARGS__); } }while(0)
#else
#define LOG_FAULT(...) do{}while(0)
#endif

bool ept_fault_is_read(seL4_Word qualification);
bool ept_fault_is_write(seL4_Word qualification);
uint64_t pte_to_gpa(uint64_t pte);
bool pte_present(uint64_t pte);
bool pt_page_size(uint64_t pte);