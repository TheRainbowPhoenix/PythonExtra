/*
**	gint:core:inth - Interrupt handlers
**	This file only contains the entry points because the gates are managed
**	by device drivers. Each driver will supply its own interrupt handler
**	blocks depending on its configuration.
*/

#define CPP_ASM
#include <gint/hardware.h>
#include <gint/config.h>

.global _gint_inth_7305

#if GINT_HW_FX
.global _gint_inth_7705
#endif

.global _gint_inth_callback

.section .gint.blocks, "ax"
.align 4

/* Interrupt handlers

   The .gint.blocks section consists of blocks of 32 bytes intended for mapping
   into the VBR space (exception, TLB miss, and interrupt handlers). Each event
   gate is linearly associated with a block number on SH4:

     block_id = (event_code - 0x3c0) / 0x20

   and associated with a compact block number on SH3 through a table (to ensure
   that blocks are consecutive in memory).

   This file provides entry points; drivers may provide their own interrupt
   handlers, and store them in the .gint.blocks section for consistency. They
   should be aware of the consequences of reordering the blocks into the VBR
   space:

   - It is possible to map MPU-specific blocks at runtime, to avoid checking
     the MPU each time an interrupt is handled;
   - However, the blocks can only rely on relative displacements or cross-
     references if their relative order is known and heeded for both on SH4 and
     SH3. A good example of this is the timer driver. Please be careful. */

/* SH7305-TYPE DEBUG INTERRUPT HANDLER - 26 BYTES */

#if 0
_gint_inth_7305:
	mov.l	1f, r0
	mov.l	@r0, r4

	sts.l	pr, @-r15
	mov.l	2f, r0
	jsr	@r0
	nop
	lds.l	@r15+, pr
	rte
	nop

	.zero 6

2:	.long	_debug
1:	.long	0xff000028
#endif

/* SH7305-TYPE INTERRUPT HANDLER ENTRY - 40 BYTES */

_gint_inth_7305:
	/* Save caller-saved registers which might currently be in use by the
	   interrupted function, as we don't know what the callback will do */
	sts.l	pr, @-r15
	stc.l	gbr, @-r15
	sts.l	mach, @-r15
	sts.l	macl, @-r15

	/* Get the event code from the INTEVT register */
	mov.l	1f, r0
	mov.l	@r0, r0

	/* Interrupt codes start at 0x400 */
	mov	#4, r1
	shll8	r1
	sub	r1, r0

	/* Add the distance to the first entry and jump as a subroutine */
	add	#(.first_entry - .jump_over), r0
	bsrf	r0
	nop

.jump_over:
	/* Restore caller-saved registers */
	lds.l	@r15+, macl
	lds.l	@r15+, mach
	ldc.l	@r15+, gbr
	lds.l	@r15+, pr

	rte
	nop

	.zero	24
1:	.long	0xff000028
.first_entry:

#if GINT_HW_FX

/* SH7705-TYPE INTERRUPT HANDLER ENTRY - 56 BYTES */

_gint_inth_7705:
	/* Save caller-saved registers as before */
	sts.l	pr, @-r15
	stc.l	gbr, @-r15
	sts.l	mach, @-r15
	sts.l	macl, @-r15

	/* Get the event code from the INTEVT2 register */
	mov.l	1f, r0
	mov.l	@r0, r0		/* r0 = old_code */

	/* Translate the event code to compact format. The compact format is
	   laid out in a way that leaves no gap in the VBR space. Additionally,
	   gates are installed starting at VBR + 0x200 to save space. */
	mov.l	2f, r2
	mov	#-5, r3
	shld	r3, r0		/* r0 = old_code >> 5 */
	add	#-32, r0	/* r0 = (old_code - 0x400) >> 5 */
	mov.b	@(r0, r2), r0	/* r0 = gate_number */
	add	#16, r0		/* r0 = (0x200 + gate_number * 0x20) >> 5 */
	mov	#5, r3
	shld	r3, r0		/* r0 = 0x200 + gate_number * 0x20 */

	/* Start at VBR + 0x200 and jump! */
	stc	vbr, r1
	add	r1, r0
	jsr	@r0
	nop

	/* Restore saved registers */
	lds.l	@r15+, macl
	lds.l	@r15+, mach
	ldc.l	@r15+, gbr
	lds.l	@r15+, pr

	rte
	nop

	.zero 8
1:	.long	0xa4000000	/* INTEVT2 register */
2:	.long	_inth_remap

.data
.align 4

/* EVENT CODE TRANSLATION TABLE - 96 BYTES
   This table was originally used to translate SH3 event codes to corresponding
   SH4 events codes so that the VBR can be laid out in the SH4 model on every
   platform. However, due to memory size limitations on SH3, it now also
   translates SH4 event codes to adjacent gates numbers to reduce the size of
   the VBR section.

   The VBR interrupt space on SH3 is laid out as follows:

   VBR offset  SH3 events       Description
   -------------------------------------------------------------------
   0x200       400 420 440      TMU0, TMU1, TMU2
   0x260       f00 --- --- ---  ETMU0, 3-gate logic at ETMU4
   0x2e0       4a0              RTC Periodic Interrupt
   -------------------------------------------------------------------
   0x600       --- ---          Entry gate
   -------------------------------------------------------------------

   There is space for 16 gates at VBR + 0x200 so the VBR currently ends after
   the interrupt entry gate at VBR + 0x640. */

_inth_remap:
	.byte	   0,    1,    2, 0xff, 0xff,    7, 0xff, 0xff
	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	.byte	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	.byte	   3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff

#endif

.section .gint.mapped, "ax"

/* CALLBACK HELPER
   This function implements the callback with context saving. It is a general
   function and does not need to reside in VBR space which is block-structured.
   This function saves registers r0_bank...r7_bank, enables interrupts,
   switches back to user bank and executes the callback. It does not save other
   registers (pr/mach/macl/gbr) which are managed by the handler entry. */

/* gint_inth_callback: Indirect call from kernel space to userland
   @r4  Address of callback function
   -> Returns the return value of the callback (int). */
_gint_inth_callback_reloc:
	stc.l	r0_bank, @-r15
	stc.l	r1_bank, @-r15
	stc.l	r2_bank, @-r15
	stc.l	r3_bank, @-r15
	stc.l	r4_bank, @-r15
	stc.l	r5_bank, @-r15
	stc.l	r6_bank, @-r15
	stc.l	r7_bank, @-r15
	stc.l	spc, @-r15
	stc.l	ssr, @-r15
	/* We backup the address of the gint_inth_callback_context_t built on
	   the stack to the user bank. */
	ldc	r15, r4_bank

	stc.l	sr, @-r15

	/* Save some values to user bank; once we enable interrupts, the kernel
	   bank might be overwritten at any moment. */
	ldc	r4, r3_bank

	/* Enable interrupts and go back to user bank. On SH4, SR.IMASK is set
	   to the level of the current interrupt, which makes sure we can only
	   be re-interrupted by something with a higher priority. */
	stc	sr, r1
	mov.l	.SR_clear_RB_BL, r0
	and	r0, r1

	/* On SH3 the CPUOPM.INTMU bit is not supported, and on the fx-CG
	   emulator, it is outright ignored. In these situations, set IMASK to
	   15 to block interrupts while allowing TLB misses to be handled. */
	mov.l	.gint, r2
	mov.l	@r2, r0
	tst	#1, r0
	bf	.set_imask
	mov.l	@(4*HWCALC,r2), r0
	cmp/eq	#HWCALC_FXCG_MANAGER, r0
	bt	.set_imask

	bra	.load_sr
	nop

.set_imask:
	mov.l	.SR_set_IMASK, r0
	or	r0, r1

.load_sr:
	ldc	r1, sr

	/* We are now in the user bank with r3 set. Perform the call. We want
	   to forward the return value to kernel bank, but this bank can be
	   changed at any moment since interrupts are enabled. */
	sts.l	pr, @-r15

	/* We only set r4 to the value of the first argument in the gint_call_t
	   if the address is even (i.e. GINT_CALL_FLAG() was not used). */
	mov.l	@r3, r0
	tst	#1, r0
	bf	.do_not_set_r4
	mov.l	@(4, r3), r4
.do_not_set_r4:
	/* And we make sure to realign the address in case it was odd. */
	mov	#0xfe, r2
	and	r2, r0

	mov.l	@(8, r3), r5
	mov.l	@(12, r3), r6
	mov.l	@(16, r3), r7
	jsr	@r0
	nop
	lds.l	@r15+, pr

	/* Restore the previous status register and the registers of the
	   interrupted procedure. Restoring sr gets us back to system bank with
	   interrupts disabled. */
	ldc.l	@r15+, sr

	/* We can now pull the return value since interrupts are disabled */
	stc	r0_bank, r0

	ldc.l	@r15+, ssr
	ldc.l	@r15+, spc
	ldc.l	@r15+, r7_bank
	ldc.l	@r15+, r6_bank
	ldc.l	@r15+, r5_bank
	ldc.l	@r15+, r4_bank
	ldc.l	@r15+, r3_bank
	ldc.l	@r15+, r2_bank
	ldc.l	@r15+, r1_bank
	rts
	ldc.l	@r15+, r0_bank

.align 4
.SR_clear_RB_BL:
	.long	~((1 << 29) | (1 << 28))
.SR_set_IMASK:
	.long	(0xf << 4)
.gint:
	.long	_gint

.section .gint.mappedrel, "aw"
_gint_inth_callback:
	.long	_gint_inth_callback_reloc
