/*
** gint:tmu:inth-tmu - Interrupt handlers for the timer units
**
** This handler consists of 3 consecutive gates that operate as a block. It
** clears the interrupt flags, invokes a GINT_CALL() in userspace, and stops
** the timer if the callback returns non-zero.
**
** It is important to notice that the code of the gates is continuous in this
** file and thus must be continuous in memory, as the assembler will use
** relative addressing methods. This "block operations" is only possible for
** handlers that are mapped to consecutive event codes.
*/

/* Gates for the standard Timer Unit (TMU) */
.global _inth_tmu /* 96 bytes */

.section .gint.blocks, "ax"
.align 4

/* TMU0 entry and interrupt flag clearing. */
_inth_tmu:
	mov	#0, r5
	mov	#0, r6
	mov	#0, r7

.shared1:
	mov.l	.TCR0, r1
	add	r6, r1

	/* Save the timer ID on the stack */
	mov.l	r8, @-r15
	sts.l	pr, @-r15
	mov.l	r5, @-r15

	/* Clear the interrupt flag. Because r5 contains 0, 1 or 2 the 16 top
	   bits are 0 so we can compare without extending */
1:	mov.w	@r1, r5
	extu.b	r5, r3
	cmp/eq	r5, r3
	bf/s	1b
	mov.w	r3, @r1

	/* Prepare to run the callback */
	mov.l	.callback, r8
	bra	.shared2
	mov.l	@r8, r8

/* TMU1 entry, callback and timer stop logic. */
_inth_tmu_1:
	mov	#1, r5
	mov	#12, r6
	bra	.shared1
	mov	#20, r7

.shared2:
	/* Invoke callback */
	mov.l	.tmu_callbacks, r4
	jsr	@r8
	add	r7, r4
	tst	r0, r0
	mov.l	.timer_stop, r2
	bt/s	.shared3
	mov.l	r2, @-r15

	/* Stop the timer if the return value is not zero. We use the top of
	   the stack as a gint_call_t object; only the function and first
	   argument matter, timer_stop() will ignore the rest. */
	jsr	@r8
	mov	r15, r4
	bra	.shared3
	nop
	nop

/* TMU2 entry, shared exit and storage. */
_inth_tmu_2:
	mov	#2, r5
	mov	#24, r6
	bra	.shared1
	mov	#40, r7

.shared3:
	add	#8, r15
	lds.l	@r15+, pr
	rts
	mov.l	@r15+, r8

.timer_stop:
	.long	_timer_stop
.callback:
	.long	_gint_inth_callback
.TCR0:
	.long	0xa4490010
.tmu_callbacks:
	.long	_tmu_callbacks
