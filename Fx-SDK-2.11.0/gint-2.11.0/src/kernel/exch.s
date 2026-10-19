.global _gint_exch
.section .gint.exch, "ax"
.align 4

_gint_exch:
	sts.l	pr, @-r15
	stc.l	gbr, @-r15
	sts.l	mach, @-r15
	sts.l	macl, @-r15
	mov.l	r8, @-r15
	mov.l	r9, @-r15

	/* Get the first word of the gint hardware array (HWMPU). If it has the
	   last bit set, we're SH3 */
	mov.l	.gint, r0
	mov.l	@r0, r0
	tst	#1, r0
	mov.l	.expevt_sh4, r8
	bt	catch
	mov.l	.expevt_sh3, r8

catch:
	/* Panic if the catcher is NULL */
	mov.l	.catcher, r0
	mov.l	@r0, r0
	tst	r0, r0
	bt	panic

	/* Set BL=0, IMASK=15 */
	stc	sr, r9
	mov.l	.SR_set_IMASK, r1
	or	r9, r1
	mov.l	.SR_clear_BL, r2
	and	r2, r1
	ldc	r1, sr

	/* Call the catcher and leave if it returns zero (exception handled) */
	jsr	@r0
	mov.l	@r8, r4

	ldc	r9, sr
	tst	r0, r0
	bt	end

panic:
	/* RTE to the panic function, but manually, so that SPC is preserved */
	mov.l	@r8, r4
	ldc	r4, r4_bank

	mov.l	@r15+, r9
	mov.l	@r15+, r8
	lds.l	@r15+, macl
	lds.l	@r15+, mach
	ldc.l	@r15+, gbr
	lds.l	@r15+, pr

	/* Here we switch banks so r0..r7 change meaning! */
	stc	ssr, r0
	ldc	r0, sr

	mov.l	.panic, r0
	mov.l	@r0, r0
	jmp	@r0
	nop

end:
	mov.l	@r15+, r9
	mov.l	@r15+, r8
	lds.l	@r15+, macl
	lds.l	@r15+, mach
	ldc.l	@r15+, gbr
	lds.l	@r15+, pr
	rte
	nop

.align 4

.gint:
	.long	_gint
.expevt_sh4:
	.long	0xff000024
.expevt_sh3:
	.long	0xffffffd4
.catcher:
	.long	_gint_exc_catcher
.panic:
	.long	_gint_exc_panic
.SR_set_IMASK:
	.long	(0xf << 4)
.SR_clear_BL:
	.long	~(1 << 28)
