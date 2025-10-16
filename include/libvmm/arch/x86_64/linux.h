/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Documents referenced.
// 1. https://www.kernel.org/doc/html/v6.1/x86/boot.html
// 2. Linux: arch/x86/include/uapi/asm/bootparam.h

/* The so-called "zeropage", taken from [2], as referenced in Section 1.15 "64-bit Boot Protocol" of [1] */

// @billn maybe put these three in a conversion.h
typedef uint8_t __u8;
typedef uint32_t __u32;
typedef uint64_t __u64;

// struct boot_params {
// 	struct screen_info screen_info;			/* 0x000 */
// 	struct apm_bios_info apm_bios_info;		/* 0x040 */
// 	__u8  _pad2[4];					/* 0x054 */
// 	__u64  tboot_addr;				/* 0x058 */
// 	struct ist_info ist_info;			/* 0x060 */
// 	__u64 acpi_rsdp_addr;				/* 0x070 */
// 	__u8  _pad3[8];					/* 0x078 */
// 	__u8  hd0_info[16];	/* obsolete! */		/* 0x080 */
// 	__u8  hd1_info[16];	/* obsolete! */		/* 0x090 */
// 	struct sys_desc_table sys_desc_table; /* obsolete! */	/* 0x0a0 */
// 	struct olpc_ofw_header olpc_ofw_header;		/* 0x0b0 */
// 	__u32 ext_ramdisk_image;			/* 0x0c0 */
// 	__u32 ext_ramdisk_size;				/* 0x0c4 */
// 	__u32 ext_cmd_line_ptr;				/* 0x0c8 */
// 	__u8  _pad4[112];				/* 0x0cc */
// 	__u32 cc_blob_address;				/* 0x13c */
// 	struct edid_info edid_info;			/* 0x140 */
// 	struct efi_info efi_info;			/* 0x1c0 */
// 	__u32 alt_mem_k;				/* 0x1e0 */
// 	__u32 scratch;		/* Scratch field! */	/* 0x1e4 */
// 	__u8  e820_entries;				/* 0x1e8 */
// 	__u8  eddbuf_entries;				/* 0x1e9 */
// 	__u8  edd_mbr_sig_buf_entries;			/* 0x1ea */
// 	__u8  kbd_status;				/* 0x1eb */
// 	__u8  secure_boot;				/* 0x1ec */
// 	__u8  _pad5[2];					/* 0x1ed */
// 	/*
// 	 * The sentinel is set to a nonzero value (0xff) in header.S.
// 	 *
// 	 * A bootloader is supposed to only take setup_header and put
// 	 * it into a clean boot_params buffer. If it turns out that
// 	 * it is clumsy or too generous with the buffer, it most
// 	 * probably will pick up the sentinel variable too. The fact
// 	 * that this variable then is still 0xff will let kernel
// 	 * know that some variables in boot_params are invalid and
// 	 * kernel should zero out certain portions of boot_params.
// 	 */
// 	__u8  sentinel;					/* 0x1ef */
// 	__u8  _pad6[1];					/* 0x1f0 */
// 	struct setup_header hdr;    /* setup header */	/* 0x1f1 */
// 	__u8  _pad7[0x290-0x1f1-sizeof(struct setup_header)];
// 	__u32 edd_mbr_sig_buffer[EDD_MBR_SIG_MAX];	/* 0x290 */
// 	struct boot_e820_entry e820_table[E820_MAX_ENTRIES_ZEROPAGE]; /* 0x2d0 */
// 	__u8  _pad8[48];				/* 0xcd0 */
// 	struct edd_info eddbuf[EDDMAXNR];		/* 0xd00 */
// 	__u8  _pad9[276];				/* 0xeec */
// } __attribute__((packed));

uintptr_t linux_setup_images(uintptr_t ram_start,
                             size_t ram_size,
                             uintptr_t kernel,
                             size_t kernel_size,
                             uintptr_t initrd_src,
                             size_t initrd_size,
                             char *cmdline);