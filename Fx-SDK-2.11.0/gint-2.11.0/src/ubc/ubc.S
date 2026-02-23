.global _ubc_setDBR
.global _ubc_getDBR
.text

_ubc_setDBR:
	ldc	r4, dbr
	rts
	nop

_ubc_getDBR:
	stc	dbr, r0
	rts
	nop

.global _ubc_dbh
_ubc_dbh:
	/* We don't support breaking in a context where register bank 1 is used */
	stc	ssr, r0
	mov.l	.sr_rb1_mask, r1
	tst	r0, r1
	bf	.dbh_panic

	/* We backup registers in the correct order to build gdb_cpu_state_t */
	stc.l	ssr,  @-r15
	sts.l	macl, @-r15
	sts.l	mach, @-r15
	stc.l	vbr,  @-r15
	stc.l	gbr,  @-r15
	sts.l	pr,   @-r15
	stc.l	spc,  @-r15
	stc.l	sgr,  @-r15
	mov.l	r14,  @-r15
	mov.l	r13,  @-r15
	mov.l	r12,  @-r15
	mov.l	r11,  @-r15
	mov.l	r10,  @-r15
	mov.l	r9,   @-r15
	mov.l	r8,   @-r15
	stc.l	R7_BANK, @-r15
	stc.l	R6_BANK, @-r15
	stc.l	R5_BANK, @-r15
	stc.l	R4_BANK, @-r15
	stc.l	R3_BANK, @-r15
	stc.l	R2_BANK, @-r15
	stc.l	R1_BANK, @-r15
	stc.l	R0_BANK, @-r15

	/* We set the ubc_dbh_lock before enabling interrupts */
	mov.l	.ubc_dbh_lock, r0
	mov	#1, r1
	mov.b	r1, @r0

	/* Enable interrupts and switch register bank
	   Original SR is kept in r8 */
	stc	sr, r8
	mov	r8, r1
	mov.l	.sr_mask, r0
	and	r0, r1
	ldc	r1, sr

	mov	r15, r4
	mov.l	.handler, r0
	jsr	@r0
	nop

	/* Restore original SR to access the correct register bank */
	ldc	r8, sr

	/* We can release the ubc_dbh_lock now that interrupts have been
	   disabled */
	mov.l	.ubc_dbh_lock, r0
	mov	#0, r1
	mov.b	r1, @r0

	ldc.l	@r15+, R0_BANK
	ldc.l	@r15+, R1_BANK
	ldc.l	@r15+, R2_BANK
	ldc.l	@r15+, R3_BANK
	ldc.l	@r15+, R4_BANK
	ldc.l	@r15+, R5_BANK
	ldc.l	@r15+, R6_BANK
	ldc.l	@r15+, R7_BANK
	mov.l	@r15+, r8
	mov.l	@r15+, r9
	mov.l	@r15+, r10
	mov.l	@r15+, r11
	mov.l	@r15+, r12
	mov.l	@r15+, r13
	mov.l	@r15+, r14
	ldc.l	@r15+, sgr
	ldc.l	@r15+, spc
	lds.l	@r15+, pr
	ldc.l	@r15+, gbr
	ldc.l	@r15+, vbr
	lds.l	@r15+, mach
	lds.l	@r15+, macl
	ldc.l	@r15+, ssr
	rte
	nop

.dbh_panic:
	stc	sr, r1
	mov.l	.sr_mask, r0
	and	r0, r1
	ldc	r1, sr

	mov.l	.panic_code, r4

	mov.l	.panic, r0
	mov.l	@r0, r0
	jmp	@r0
	nop

.align 4
.handler:	.long _ubc_debug_handler
.ubc_dbh_lock:	.long _ubc_dbh_lock
.panic_code:	.long 0x10a0
.panic:		.long _gint_exc_panic
.sr_rb1_mask:	.long (1 << 29)
.sr_mask:	.long ~0x300000f0  /* IMASK = 0 : mask no interrupts
                                      BL = 0    : do not block interrupts
                                      RB = 0    : use register BANK0
                                    */

.data
.global _ubc_dbh_lock
_ubc_dbh_lock: .byte 0
