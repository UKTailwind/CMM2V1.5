@ Reading a source file from the SD card, through the firmware's files (zf_cmm2.c). MIT licence, as zeptoforth.
@ forth/files.fs makes include of these.

	@@ Open a file to read ( addr bytes -- handle ), the handle negative if it could not be
	define_word "cmm2-open", visible_flag
_zf_open:
	push {r4, lr}
	ldr r0, [dp], #4
	movs r1, tos
	ccall zf_open
	movs tos, r0
	pop {r4, pc}
	end_inlined

	@@ Its next line, without its line ending ( handle -- addr bytes ), bytes being -1 at the end of the file
	define_word "cmm2-line", visible_flag
_zf_line:
	push {r4, lr}
	movs r0, tos
	ccall zf_line
	ldr r1, =zf_line_addr
	ldr r1, [r1]
	str r1, [dp, #-4]!
	movs tos, r0
	pop {r4, pc}
	end_inlined

	@@ ( handle -- )
	define_word "cmm2-close", visible_flag
_zf_close:
	push {r4, lr}
	movs r0, tos
	pull_tos
	ccall zf_close
	pop {r4, pc}
	end_inlined

	.ltorg

