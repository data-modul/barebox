/*
 * cpu.c - A few helper functions for ARM
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * @file
 * @brief A few helper functions for ARM
 */

#include <common.h>
#include <init.h>
#include <command.h>
#include <cache.h>
#include <asm/mmu.h>
#include <asm/system.h>
#include <asm/memory.h>
#include <asm-generic/memory_layout.h>
#include <asm/cputype.h>
#include <asm/cache.h>
#include <asm/ptrace.h>

#include "mmu.h"

/**
 * Enable processor's instruction cache
 */
void icache_enable(void)
{
	u32 r;

	r = get_cr();
	r |= CR_I;
	set_cr(r);
}

/**
 * Disable processor's instruction cache
 */
void icache_disable(void)
{
	u32 r;

	r = get_cr();
	r &= ~CR_I;
	set_cr(r);
}

/**
 * Detect processor's current instruction cache status
 * @return 0=disabled, 1=enabled
 */
int icache_status(void)
{
	return (get_cr () & CR_I) != 0;
}

#if __LINUX_ARM_ARCH__ <= 7
/*
 * SoC like the ux500 have the l2x0 always enable
 * with or without MMU enable
 */
struct outer_cache_fns outer_cache;

/*
 * Clean and invalide caches, disable MMU
 */
void mmu_disable(void)
{
	__mmu_cache_flush();
	if (outer_cache.disable) {
		outer_cache.flush_all();
		outer_cache.disable();
	}
	__mmu_cache_off();
}
#endif

/**
 * Disable MMU and D-cache, flush caches
 * @return 0 (always)
 *
 * This function is called by shutdown_barebox to get a clean
 * memory/cache state.
 */
static void arch_shutdown(void)
{
	uint32_t r;

#ifdef CONFIG_MMU
	mmu_disable();
#endif
	flush_icache();

#if __LINUX_ARM_ARCH__ <= 7
	/*
	 * barebox normally does not use interrupts, but some functionalities
	 * (eg. OMAP4_USBBOOT) require them enabled. So be sure interrupts are
	 * disabled before exiting.
	 */
	__asm__ __volatile__("mrs %0, cpsr" : "=r"(r));
	r |= PSR_I_BIT;
	__asm__ __volatile__("msr cpsr, %0" : : "r"(r));
#endif
}
archshutdown_exitcall(arch_shutdown);

extern unsigned long arm_stack_top;

static int arm_request_stack(void)
{
	if (!request_sdram_region("stack", arm_stack_top - STACK_SIZE, STACK_SIZE))
		pr_err("Error: Cannot request SDRAM region for stack\n");

	return 0;
}
coredevice_initcall(arm_request_stack);
