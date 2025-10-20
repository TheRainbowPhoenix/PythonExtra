/* START: Sets up the inner and outer loop. The outer loop is anything between
   the calls to macros START and END, while the inner loop is the code between
   labels 2: and 3: (both *INCLUDED*). */
.macro START
	ldrs 2f
	ldre 3f
1:	ldrc r2
	nop
.endm

/* END: Finishes the outer loop and adds strides. */
.macro END
	dt	r1
	add	r4, r3
	bf.s	1b
	add	r6, r5
.endm

/* EPILOGUE: Finishes the call by reloading registers saved in the prologue. */
.macro EPILOGUE
	mov.l	@r15+, r9
	mov	r3, r0
	rts
	mov.l	@r15+, r8
.endm
