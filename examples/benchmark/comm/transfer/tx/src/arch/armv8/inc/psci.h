/** 
 * Bao, a Lightweight Static Partitioning Hypervisor 
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Sandro Pinto <sandro.pinto@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details. 
 *
 */

#ifndef __PSCI_H__
#define __PSCI_H__

#include <stdint.h>

#define PSCI_VERSION				(0x84000000)
#define PSCI_CPU_SUSPEND_AARCH32	(0x84000001)
#define PSCI_CPU_SUSPEND_AARCH64	(0xc4000001)
#define PSCI_CPU_OFF				(0x84000002)
#define PSCI_CPU_ON_AARCH32			(0x84000003)
#define PSCI_CPU_ON_AARCH64			(0xc4000003)
#define PSCI_AFFINITY_INFO_AARCH32	(0x84000004)
#define PSCI_AFFINITY_INFO_AARCH64	(0xc4000004)

#define PSCI_POWER_STATE_LVL_0 		0x0000000
#define PSCI_POWER_STATE_LVL_1 		0x1000000
#define PSCI_POWER_STATE_LVL_2 		0x2000000
#define PSCI_STATE_TYPE_STANDBY		0x00000
#define PSCI_STATE_TYPE_POWERDOWN	(1UL << 30)
#define PSCI_STATE_TYPE_BIT			(1UL << 30)

#define PSCI_CPU_IS_ON			0
#define PSCI_CPU_IS_OFF			1

#define PSCI_INVALID_ADDRESS		(-1L)
#define PSCI_E_SUCCESS			 0
#define PSCI_E_NOT_SUPPORTED	-1
#define PSCI_E_INVALID_PARAMS	-2
#define PSCI_E_DENIED			-3
#define PSCI_E_ALREADY_ON		-4
#define PSCI_E_ON_PENDING		-5
#define PSCI_E_INTERN_FAIL		-6
#define PSCI_E_NOT_PRESENT		-7
#define PSCI_E_DISABLED			-8
#define PSCI_E_INVALID_ADDRESS	-9


/* psci wake up from off entry point */
void _psci_wake_up();

uint64_t psci_version(void);

uint64_t psci_cpu_suspend(uint64_t power_state,
		     uintptr_t entrypoint,
		     uint64_t context_id);

uint64_t psci_cpu_off(void);

uint64_t psci_cpu_on(uint64_t target_cpu,
		uintptr_t entrypoint,
		uint64_t context_id);

uint64_t psci_affinity_info(uint64_t target_affinity,
		       uint64_t lowest_affinity_level);

#endif /* __PSCI_H__ */
