//---
//	gint:core:kernel - Installing and unloading the library
//---

#include <gint/gint.h>
#include <gint/drivers.h>
#include <gint/hardware.h>
#include <gint/mmu.h>
#include <gint/mpu/intc.h>
#include <gint/kmalloc.h>
#include <gint/cpu.h>
#include <gint/exc.h>
#include <gint/config.h>

#include <string.h>
#include <stdlib.h>

#include "vbr.h"
#include "kernel.h"

/* Reference the CPU and INTC drivers which are required for gint to work */
extern gint_driver_t drv_intc, drv_cpu;
GUNUSED gint_driver_t *gint_required_cpu = &drv_cpu;
GUNUSED gint_driver_t *gint_required_intc = &drv_intc;

/* World buffers for the OS and gint */
gint_world_t gint_world_os = NULL;
gint_world_t gint_world_addin = NULL;

/* Dynamic flags for all drivers */
uint8_t *gint_driver_flags = NULL;

/* Top of the stack */
void *gint_stack_top = NULL;

//---
//	Initialization and unloading
//---

/* kinit(): Install and start gint */
void kinit(void)
{
	#if GINT_HW_FX
	/* On fx-9860G, VBR is loaded at the end of the user RAM. On SH4, the
	   end of the user RAM hosts the stack, for which we leave 12 kB
	   (0x3000 bytes). The VBR space takes about 0x600 bytes on SH3 due to
	   the compact scheme, while it uses about 0x1100 bytes for the whole
	   expanded region. */
	uint32_t uram_end = (uint32_t)mmu_uram() + mmu_uram_size();

	/* Stack space on SH4. 14 kB is a lot, but Fugue's BFile has been seen
	   overflowing both 8 kB and 12 kB */
	if(isSH4()) {
		uram_end -= (gint[HWFS] == HWFS_FUGUE) ? 0x3800 : 0x2000;
		gint_stack_top = (void *)uram_end;
	}

	/* VBR is advanced 0x100 bytes because of an unused gap */
	uram_end -= (isSH3() ? 0x600 : 0x1100);
	uint32_t VBR = uram_end - 0x100;
	#endif

	#if GINT_HW_CG
	/* On fx-CG 50, VBR is loaded at the start of the user RAM; the linker
	   script leaves 5 kB (0x1400 bytes) before the start of the data
	   segment. The stack is again placed at the end of the region, and we
	   leave 16 kB. */
	uint32_t VBR = (uint32_t)mmu_uram();
	uint32_t uram_end = (uint32_t)mmu_uram() + mmu_uram_size() - 0x4000;
	gint_stack_top = (void *)uram_end;
	#endif

	#if GINT_HW_CP
	extern char gint_region_vbr;
	uint32_t VBR = (uint32_t)&gint_region_vbr - 0x100;
	#endif

	/* Event handler entry points */
	void *inth_entry = isSH3() ? gint_inth_7705 : gint_inth_7305;
	uint32_t exch_size = (uint32_t)&gint_exch_size;
	uint32_t tlbh_size = (uint32_t)&gint_tlbh_size;

	/* Load the event handler entry points into memory */
	memcpy((void *)VBR + 0x100, gint_exch, exch_size);
	memcpy((void *)VBR + 0x400, gint_tlbh, tlbh_size);
	memcpy((void *)VBR + 0x600, inth_entry, 64);

	/* Initialize memory allocators */
	kmalloc_init();

	#if !GINT_HW_CP
	/* Create an allocation arena with unused static RAM */
	static kmalloc_arena_t static_ram = { 0 };
	extern uint32_t euram;
	static_ram.name = "_uram";
	static_ram.is_default = isSH4();
	static_ram.start = mmu_uram() + ((uint32_t)&euram - 0x08100000);
	static_ram.end = (void *)uram_end;
	kmalloc_init_arena(&static_ram, true);
	kmalloc_add_arena(&static_ram);
	#endif

	/* Create an arena in the OS stack as well, for VRAM and more data */
	#if GINT_HW_CG && !defined(GINT_NO_OS_STACK)
	static kmalloc_arena_t os_stack = { 0 };
	os_stack.name = "_ostk";
	os_stack.is_default = true;
	if(gint[HWCALC] == HWCALC_PRIZM || gint[HWCALC] == HWCALC_FXCG_MANAGER)
		os_stack.start = (void *)0x880f0000;
	else
		os_stack.start = (void *)0x8c0f0000;
	os_stack.end = os_stack.start + (350 * 1024);
	kmalloc_init_arena(&os_stack, true);
	kmalloc_add_arena(&os_stack);
	#endif

	/* Allocate world buffers for the OS and for gint */
	gint_world_os = gint_world_alloc();
	gint_world_addin = gint_world_alloc();
	gint_driver_flags = malloc(gint_driver_count());

	/* Allocate VRAMs, which is important for panic screens */
	extern bool dvram_init(void);
	// TODO: Cannot _Exit() yet, the gint_exitbuf isn't setup!
	if(!dvram_init())
		abort();

	if(!gint_world_os || !gint_world_addin || !gint_driver_flags)
		gint_panic(0x1060);

	/* Initialize drivers */
	for(int i = 0; i < gint_driver_count(); i++)
	{
		gint_driver_t *d = &gint_drivers[i];
		if(d->constructor) d->constructor();

		uint8_t *f = &gint_driver_flags[i];
		*f = (d->flags & GINT_DRV_INIT_) | GINT_DRV_CLEAN;
	}

	/* Select the VBR address for this world before configuring */
	cpu_configure_VBR(VBR);

	gint_world_switch_in(gint_world_os, gint_world_addin);
}

/* kquit(): Quit gint and give back control to the system */
void kquit(void)
{
	gint_world_switch_out(gint_world_addin, gint_world_os);

#if !GINT_OS_FX
	extern void dvram_quit(void);
	dvram_quit();
#endif

	gint_world_free(gint_world_os);
	gint_world_free(gint_world_addin);
	free(gint_driver_flags);

	gint_world_os = NULL;
	gint_world_addin = NULL;
	gint_driver_flags = NULL;
}
