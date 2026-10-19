/*
** gint:cpu:registers - Access to primary registers used in the CPU
*/

.global _cpu_getVBR
.global _cpu_setVBR
.global _cpu_setCPUOPM
.global _cpu_getCPUOPM
.global _cpu_getSR
.global _cpu_setSR
.text

/* cpu_setVBR(): Change VBR address */
_cpu_setVBR:
	ldc	r4, vbr
	rts
	nop

_cpu_getVBR:
	stc	vbr, r0
	rts
	nop

_cpu_setCPUOPM:
	/* Set CPUOPM as requested */
	mov.l	1f, r0
	mov.l	r4, @r0

	/* Read CPUOPM again */
	mov.l	@r0, r5

	/* Invalidate a cache address */
	mov	#-96, r0
	shll16	r0
	shll8	r0
	icbi	@r0

	rts
	nop

_cpu_getCPUOPM:
	mov.l	1f, r0
	rts
	mov.l	@r0, r0

.align 4
1:	.long	0xff2f0000

_cpu_getSR:
	stc	sr, r0
	rts
	nop

_cpu_setSR:
	/* Set only MD, RB, BL, DSP and IMASK */
	mov.l	1f, r0
	not	r0, r1
	stc	sr, r2

	and	r1, r2
	and	r0, r4
	or	r4, r2

	ldc	r2, sr
	rts
	nop

.align 4
1:	.long	0x700010f0
