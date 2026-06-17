.global _cpu_csleep_init
.global _cpu_csleep
.global _cpu_csleep_cancel

_cpu_csleep_init:
	mov.l	.memcpy, r1
	mova	sleep, r0
	mov	r0, r5
	jmp	@r1
	mov	#(sleep_end - sleep), r6

.align 4
.memcpy:
	.long	_memcpy

_cpu_csleep:
	mov.l	r8, @-r15
	sts.l	pr, @-r15
	mov	r4, r8

	/* Check if the sleep instruction is still there */
1:	mov.w	@(8, r8), r0
	extu.w	r0, r0
	cmp/eq	#0x001b, r0
	bf	2f

	/* Invalidate the cache in case of previous ICS being cached */
	mov	r8, r0
	icbi	@r0
	add	#18, r0
	icbi	@r0

	/* Execute the sleep, and loop */
	jsr	@r8
	nop
	bra	1b
	nop

2:	lds.l	@r15+, pr
	rts
	mov.l	@r15+, r8

_cpu_csleep_cancel:
	mov	#0x0009, r0
	add	#8, r4
	mov.w	r0, @r4
	icbi	@r4
	rts
	nop

.align 4

/* This is identical in functionality to the CPU driver's sleep() function */
sleep:
	mov.l	2f, r0
	mov.l	@r0, r0
	cmp/pl	r0
	bt	1f
	sleep
1:	rts
	nop
	nop
2:	.long	_cpu_sleep_block_counter
sleep_end:
