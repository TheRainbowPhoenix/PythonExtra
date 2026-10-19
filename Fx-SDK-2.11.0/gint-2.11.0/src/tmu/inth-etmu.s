/*
** gint:tmu:inth-etmu - Interrupt handlers for the RTC-bound timers
**
** This handler uses 3 consecutive blocks like the TMU handler. However this
** time 2 empty blocks after ETMU4 (0xd20, 0xd40) are used because blocks for
** ETMU are not consecutive in memory.
**
** It would be possible to communicate between any interrupt handlers in non-
** consecutive gates. A simple way is to store at runtime a pointer to the
** desired object in one handler. But that costs a lot of space. If the
** relative position of interrupt handlers is known, the best option left is
** the unnatural @(disp,pc) addressing mode, and it doesn't even work with the
** SH3's compact VBR scheme.
*/

/* Gates for the extra timers (informally called ETMU) */
.global _inth_etmu4 /* 96 bytes */
.global _inth_etmux /* 32 bytes */

.section .gint.blocks, "ax"
.align 4

/* 3-block handler installed at the ETMU4 gate. */
_inth_etmu4:
	mova	.storage_etmu4, r0
	mov	#7, r2

.shared:
	mov.l	r8, @-r15
	sts.l	pr, @-r15

	/* Prepare an indirect call to timer_stop(<id>) */
	add	#-20, r15
	mov.l	r2, @(4, r15)

	/* Clear interrupt flag in TCR */
	mov	r0, r1
	mov.l	@(4, r1), r3
1:	mov.b	@r3, r0
	tst	#0x02, r0
	and	#0xfd, r0
	bf/s	1b
	mov.b	r0, @r3

	/* Invoke callback */
	mov.l	.gint_inth_callback, r8
	mov.l	@r8, r8
	jsr	@r8
	mov.l	@r1, r4
	tst	r0, r0
	bt	2f

	/* If return value is non-zero, stop the timer with another callback */
	mov.l	.timer_stop, r0
	mov.l	r0, @r15
	jsr	@r8
	mov	r15, r4

2:	add	#20, r15
	lds.l	@r15+, pr
	rts
	mov.l	@r15+, r8

	.zero	26

.timer_stop:
	.long	_timer_stop
.gint_inth_callback:
	.long	_gint_inth_callback
.storage_etmu4:
	.long	_tmu_callbacks + 140
	.long	0xa44d00bc /* RTCR4 */

/* Generic gate for all other ETMU handlers, falling back to ETMU4. */
_inth_etmux:
	/* Dynamically compute the target of the jump */
	stc	vbr, r3
	mov.l	1f, r2
	add	r2, r3

	mova	.storage_etmux, r0
	mov.w	.id_etmux, r2
	jmp	@r3
	nop
	nop
	nop

.id_etmux:
	.word	0 /* Timer ID */

	/* Offset from VBR where ETMU4 is located; set during configure */
1:	.long	(.shared - _inth_etmu4)

.storage_etmux:
	.long	_tmu_callbacks
	.long	0 /* TCR address */
