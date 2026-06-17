.global	_usb_pipe_write4
.global	_usb_pipe_flush4

#define _fifo r3
#define _data r4
#define _size r5
#define _buf r6
#define _bufsize r7

_usb_pipe_write4:
	/* Skip to writing the data if the buffer's empty. This test is free
	   because having it simplifies a later while loop into a do/while. */
	mov.b	@_bufsize, r1
	mov	#4, r0
	mov.l	@r15, _fifo
	tst	r1, r1
	bt.s	.write_data
	sub	r1, r0		/* Bytes required to fill the buffer */

	/* If we can't even fill the buffer, skip to the end push. */
	mov.l	@_buf, r2
	cmp/gt	_size, r0
	bt	.push_buffer

	/* Precompute the amount of data left after filling the buffer */
	sub	r0, _size

	/* Fill the buffer by reading unaligned bytes */
1:	mov.b	@_data+, r1
	shll8	r2
	dt	r0
	extu.b	r1, r1
	bf.s	1b
	or	r1, r2

	/* Commit the filled buffer */
	mov.l	r2, @_fifo

.write_data:
	/* Check if we have enough data to run this loop */
	/* TODO: For small sizes use another loop, so we can unroll? */
	mov	#4, r0
	cmp/gt	_size, r0
	bt	4f

	/* Determine whether we need to use unaligned reads */
	mov	#3, r0
	tst	r0, _data
	mov	_size, r1
	bt.s	3f
	shlr2	r1

	/* Unaligned write loop */
2:	movua.l	@_data+, r0
	dt	r1
	bf.s	2b
	mov.l	r0, @_fifo

	bra	4f
	nop

	/* Aligned write loop */
3:	mov.l	@_data+, r0
	dt	r1
	bf.s	3b
	mov.l	r0, @_fifo

4:	mov	#3, r0
	and	r0, _size

	mov	#0, r2
	mov	#0, r1

.push_buffer:
	/* Here r1 = buffer size, r2 = buffer contents, _size = data left */

	/* Check if there is any data left to push into the short buffer */
	tst	_size, _size
	mov	r1, r0
	add	_size, r0
	bt.s	.end
	mov.b	r0, @_bufsize

	/* Push loop */
5:	mov.b	@_data+, r1
	shll8	r2
	dt	_size
	extu.b	r1, r1
	bf.s	5b
	or	r1, r2

.end:
	rts
	mov.l	r2, @_buf

#undef _fifo
#undef _data
#undef _size
#undef _buf
#undef _bufsize

/* --- */

#define _buf r4
#define _bufsize r5
#define _fifo r6

_usb_pipe_flush4:
	/* Jump table. We skip 4*_bufsize bytes, which lands us right on labels
	   0:, 1:, 2: or 3: depending on the value of _bufsize. */
	shll2	_bufsize
	braf	_bufsize
	mov	_buf, r0

0:	/* No extra data to write out */
	rts
	nop

1:	/* Single byte */
	rts
	mov.b	r0, @_fifo

2:	/* Two bytes */
	rts
	mov.w	r0, @_fifo

3:	/* Three bytes */
	mov	r0, r1
	shlr8	r1
	mov.w	r1, @_fifo
	rts
	mov.b	r0, @(2, _fifo)

#undef _buf
#undef _bufsize
#undef _fifo
