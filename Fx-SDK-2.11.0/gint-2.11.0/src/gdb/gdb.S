.global _gdb_stubcall_write

_gdb_stubcall_write:
	mov	#64, r3
	trapa	#33
	rts
	nop
