@ The firmware's services for Forth. MIT licence, as zeptoforth.
@
@ One word for all of them: it hands the data stack to a shim in zf_services.c, picked by the number on top, and the shim
@ says where it left the stack, so each word's stack effect is its shim's and the kernel knows nothing of them.
@ forth/cmm2.fs makes the words. zf_services.c is linked after the kernel, so growing it moves nothing here and the saved
@ image stays good.
@
@ It is built from float64.s's macros and not ccall because of what a shim does to the stack. An answer can lie below the
@ old dp, and the tick runs Forth on this stack: dp has to be right before zf_in_c lets the tick in.

	@@ ( ... index -- ... )
	define_word "cmm2-call", visible_flag
_zf_call:
	push {r4, lr}
	fp_begin
	movs r0, tos
	movs r1, dp
	ccall_fp zf_call
	movs dp, r0
	pull_tos
	fp_end
	pop {r4, pc}
	end_inlined

	.ltorg
