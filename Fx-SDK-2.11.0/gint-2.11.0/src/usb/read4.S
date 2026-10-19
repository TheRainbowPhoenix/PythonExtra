.global _usb_pipe_read4

/* User buffer and round size */
#define _data r4
#define _datasize r5
/* FIFO address and amount of data available in there */
#define _fifo r6
#define _fifosize r7
/* Short buffer address and *pointer to* its size */
#define _buf r8
#define _bufsize r9

/* Copy _datasize bytes from _fifo to _data, using USB FIFO access rules and
   storing excess data in the short buffer.
   Requires: 1 ≤ _datasize ≤ _fifosize + *_bufsize */
_usb_pipe_read4:
	mov.l	@(4, r15), r1
	mov.l	r8, @-r15
	mov.l	r9, @-r15

	mov	r1, _bufsize
	mov.b	@_bufsize, r1

/* Step #1: If _datasize ≤ *_bufsize < 4, then we fill user data from the short
   buffer, and return immediately. */

	/* 1 cycle lost here due to early use of r1 */

	mov.l	@(8, r15), _buf
	cmp/ge	_datasize, r1

	bt	.short_buffer_only
	tst	r1, r1

/* Step #2: If *_bufsize > 0, copy *_bufsize bytes from the short buffer to
  _data, emptying the short buffer. */

	bt	2f
	mov	_buf, r3

1:	mov.b	@r3+, r2
	dt	_datasize

	dt	r1
 	mov.b	r2, @_data

	bf.s	1b
	add	#1, _data

	mov.b	r1, @_bufsize
	nop

/* Step #3: Copy longwords from the FIFO to the user buffer as long as there is
   at least 4 bytes in both. Since at this stage _datasize ≤ _fifosize we can
   simply check _datasize. */

2:	/* Update _datasize and _fifosize in advance */
	mov	#-4, r0
	and	_datasize, r0

	/* If _datasize ≤ 3, skip this step */
	mov	#3, r3
	cmp/ge	_datasize, r3

	bt.s	3f
	sub	r0, _fifosize

	tst	r3, _data
	nop

	/* r2 is the number of 4-byte reads; since _datasize > 3, r2 ≥ 1 */
	mov	_datasize, r2
	shlr2	r2

	bf.s	.unaligned
	and	r3, _datasize

.aligned:
	ldrs	.al_b
	ldre	.al_e
	ldrc	r2
	mov	_fifo, r3

.al_b:	movs.l	@r3, x0
.al_e:	movs.l	x0, @_data+

	bra	3f
	nop

.unaligned:
	mov.l	@_fifo, r0
	dt	r2

	mov.b	r0, @(3, _data)
	nop

	shlr8	r0
	mov.b	r0, @(2, _data)

	shlr8	r0
	mov.b	r0, @(1, _data)

	shlr8	r0
	mov.b	r0, @_data

	bf.s	.unaligned
	add	#4, _data

/* Step #4: Load the final bytes of the round from the FIFO into the short
   buffer (can be either nothing or a standard 4-byte read), then finish with a
   copy to user data. */

3:	/* If we finished the read, don't load anything */
	tst	_datasize, _datasize
	mov	#3, r3

	bt	.epilogue
	cmp/hi	r3, _fifosize

	bf	4f
	mov	#4, _fifosize

	/* Make a 4-byte read from the FIFO. If there are less than 4 bytes
	   left this will pad with zeros. */
4:	mov.l	@_fifo, r3
	mov	_fifosize, r1

	mov.l	r3, @_buf
	nop

/* Step #1/#5: Copy 0 < _datasize ≤ *_bufsize bytes from the short buffer to
   _data, then return. r1 must be *_bufsize. */
.short_buffer_only:
	mov.l	@_buf, r3
	mov	#0, r0

	sub	_datasize, r1
	nop

	/* The loop accesses memory with @(r0, _) but also shifts the contents
	   of _buf in r3 (it's more convenient to use the available EX slot
	   than shift by 8*_datasize outside the loop) */

5:	mov.b	@(r0, _buf), r2
	dt	_datasize

	mov.b	r2, @(r0, _data)
	add	#1, r0

	bf.s	5b
	shll8	r3

.epilogue:
	mov.l	r3, @_buf
	mov.b	r1, @_bufsize
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8
