@ Double-precision floating point on the CMM2. MIT licence, as zeptoforth.
@
@ The STM32F746 zeptoforth knows has a single-precision FPU, so its float32 module works in one cell. The H743 has double
@ precision in hardware, and this takes float32's place: its word names with v as dv, dv+ for v+ and dvsin for vsin, and a
@ value is two cells, ( lo hi ) with the high word on top, as a double cell is, so 2dup, 2swap, 2@, 2constant and the rest
@ apply to it. Arithmetic is done by the FPU here, and the maths functions by C's library, through zf_cmm2.c for parsing
@ and printing.
@
@ d0 to d2 are only live inside one of these words, which zf_in_c keeps preemption out of for the few instructions it
@ lasts (the tick's C code would use them): so the FPU has no state to save between words, or per task.

	.fpu fpv5-d16

	.macro fp_begin
	ldr r12, =zf_in_c
	str r12, [r12]
	.endm

	@@ (a tick that came meanwhile takes effect here: the words push lr for it)
	.macro fp_end
	movs r1, #0
	ldr r12, =zf_in_c
	str r1, [r12]
	tick_check
	.endm

	@@ A call to C from inside fp_begin: as ccall, with zf_in_c left set for what follows
	.macro ccall_fp fn
	mov r4, sp
	ldr r12, =zf_c_sp
	ldr r12, [r12]
	mov sp, r12
	bl \fn
	mov sp, r4
	.endm

	@@ ( f1 f0 -- ) into d0 and d1, the stack left as it is
	.macro load2
	ldr r0, [dp]
	vmov d1, r0, tos
	ldr r0, [dp, #8]
	ldr r1, [dp, #4]
	vmov d0, r0, r1
	.endm

	@@ d0 as the answer to two values
	.macro store2
	vmov r0, r1, d0
	adds dp, #8
	str r0, [dp]
	movs tos, r1
	.endm

	@@ ( f -- ) into d0, and d0 as the answer to one
	.macro load1
	ldr r0, [dp]
	vmov d0, r0, tos
	.endm

	.macro store1
	vmov r0, r1, d0
	str r0, [dp]
	movs tos, r1
	.endm

	.macro vbin name, label, op
	define_word "\name", visible_flag
\label:
	push {lr}
	fp_begin
	load2
	\op d0, d0, d1
	store2
	fp_end
	pop {pc}
	end_inlined
	.endm

	.macro vun name, label, op
	define_word "\name", visible_flag
\label:
	push {lr}
	fp_begin
	load1
	\op d0, d0
	store1
	fp_end
	pop {pc}
	end_inlined
	.endm

	@@ ( f1 f0 -- flag ), cond being true of f1 against f0, and false if either is NaN (but for dv<>)
	.macro vcompare name, label, cond
	define_word "\name", visible_flag
\label:
	push {lr}
	fp_begin
	load2
	vcmp.f64 d0, d1
	vmrs APSR_nzcv, FPSCR
	add.w dp, dp, #12
	b\cond 1f
	movs tos, #0
	b 2f
1:	movs tos, #-1
2:	fp_end
	pop {pc}
	end_inlined
	.endm

	.macro vfn1 name, label, cfn
	define_word "\name", visible_flag
\label:
	push {r4, lr}
	fp_begin
	load1
	ccall_fp \cfn
	store1
	fp_end
	pop {r4, pc}
	end_inlined
	.endm

	.macro vfn2 name, label, cfn
	define_word "\name", visible_flag
\label:
	push {r4, lr}
	fp_begin
	load2
	ccall_fp \cfn
	store2
	fp_end
	pop {r4, pc}
	end_inlined
	.endm

	vbin "dv+", _dv_add, vadd.f64
	vbin "dv-", _dv_sub, vsub.f64
	vbin "dv*", _dv_mul, vmul.f64
	vbin "dv/", _dv_div, vdiv.f64
	vbin "dvmin", _dv_min, vminnm.f64
	vbin "dvmax", _dv_max, vmaxnm.f64
	vun "dvsqrt", _dv_sqrt, vsqrt.f64
	vun "dvnfract", _dv_nfract, vrintz.f64
	vun "dvceil", _dv_ceil, vrintp.f64
	vun "dvfloor", _dv_floor, vrintm.f64
	vun "dvround-half-even", _dv_round_even, vrintn.f64
	vun "dvround-half-away-zero", _dv_round_away, vrinta.f64

	.ltorg

	vcompare "dv<", _dv_lt, mi
	vcompare "dv<=", _dv_le, ls
	vcompare "dv=", _dv_eq, eq
	vcompare "dv<>", _dv_ne, ne
	vcompare "dv>", _dv_gt, gt
	vcompare "dv>=", _dv_ge, ge

	@@ ( f -- f' )
	define_word "dvabs", visible_flag
_dv_abs:
	bic tos, tos, #0x80000000
	bx lr
	end_inlined

	define_word "dvnegate", visible_flag
_dv_negate:
	eor tos, tos, #0x80000000
	bx lr
	end_inlined

	@@ The fractional part ( f -- f' )
	define_word "dvfract", visible_flag
_dv_fract:
	push {lr}
	fp_begin
	load1
	vrintz.f64 d1, d0
	vsub.f64 d0, d0, d1
	store1
	fp_end
	pop {pc}
	end_inlined

	@@ ( f1 f0 -- f' ): f1 less the nearest whole multiple of f0, as float32's vmod is with an FPv5 FPU
	define_word "dvmod", visible_flag
_dv_mod:
	push {lr}
	fp_begin
	load2
	vdiv.f64 d2, d0, d1
	vrintn.f64 d2, d2
	vmul.f64 d2, d2, d1
	vsub.f64 d0, d0, d2
	store2
	fp_end
	pop {pc}
	end_inlined

	@@ ( n -- f )
	define_word "n>dv", visible_flag
_n_to_dv:
	push {lr}
	fp_begin
	vmov s0, tos
	vcvt.f64.s32 d0, s0
	vmov r0, r1, d0
	str r0, [dp, #-4]!
	movs tos, r1
	fp_end
	pop {pc}
	end_inlined

	@@ ( u -- f )
	define_word "u>dv", visible_flag
_u_to_dv:
	push {lr}
	fp_begin
	vmov s0, tos
	vcvt.f64.u32 d0, s0
	vmov r0, r1, d0
	str r0, [dp, #-4]!
	movs tos, r1
	fp_end
	pop {pc}
	end_inlined

	@@ ( f -- n ), towards zero
	define_word "dv>n", visible_flag
_dv_to_n:
	push {lr}
	fp_begin
	ldr r0, [dp], #4
	vmov d0, r0, tos
	vcvt.s32.f64 s0, d0
	vmov tos, s0
	fp_end
	pop {pc}
	end_inlined

	@@ ( f -- u ), towards zero
	define_word "dv>u", visible_flag
_dv_to_u:
	push {lr}
	fp_begin
	ldr r0, [dp], #4
	vmov d0, r0, tos
	vcvt.u32.f64 s0, d0
	vmov tos, s0
	fp_end
	pop {pc}
	end_inlined

	.ltorg

	@@ A single-precision value, as float32 has it, to a double ( v -- f )
	define_word "v>dv", visible_flag
_v_to_dv:
	push {lr}
	fp_begin
	vmov s0, tos
	vcvt.f64.f32 d0, s0
	vmov r0, r1, d0
	str r0, [dp, #-4]!
	movs tos, r1
	fp_end
	pop {pc}
	end_inlined

	@@ And back ( f -- v )
	define_word "dv>v", visible_flag
_dv_to_v:
	push {lr}
	fp_begin
	ldr r0, [dp], #4
	vmov d0, r0, tos
	vcvt.f32.f64 s0, d0
	vmov tos, s0
	fp_end
	pop {pc}
	end_inlined

	@@ An S31.32 fixed-point value to a double ( lo hi -- f ), a double being two cells too
	define_word "f64>dv", visible_flag
_fix_to_dv:
	push {r4, lr}
	fp_begin
	ldr r0, [dp]
	movs r1, tos
	ccall_fp zf_fix2d
	store1
	fp_end
	pop {r4, pc}
	end_inlined

	@@ And back ( f -- lo hi )
	define_word "dv>f64", visible_flag
_dv_to_fix:
	push {r4, lr}
	fp_begin
	load1
	ccall_fp zf_d2fix
	str r0, [dp]
	movs tos, r1
	fp_end
	pop {r4, pc}
	end_inlined

	@@ The value as text at addr, which has 40 bytes ( f addr -- count )
	define_word "format-dv", visible_flag
_format_dv:
	push {r4, lr}
	fp_begin
	ldr r1, [dp]
	ldr r2, [dp, #4]
	vmov d0, r2, r1
	movs r0, tos
	ccall_fp zf_vformat
	adds dp, #8
	movs tos, r0
	fp_end
	pop {r4, pc}
	end_inlined

	@@ Text as a value ( addr bytes -- f flag ), the flag true if all of it was a number: 1.5de0, or 1.5e0 without the d
	define_word "parse-dv", visible_flag
_parse_dv:
	push {r4, lr}
	fp_begin
	ldr r0, [dp]
	movs r1, tos
	ccall_fp zf_vparse
	vmov r0, r1, d0
	ldr r2, =zf_vparse_ok
	ldr r2, [r2]
	subs dp, #4
	str r1, [dp]
	str r0, [dp, #4]
	movs tos, r2
	fp_end
	pop {r4, pc}
	end_inlined

	.ltorg

	vfn1 "dvsin", _dv_sin, sin
	vfn1 "dvcos", _dv_cos, cos
	vfn1 "dvtan", _dv_tan, tan
	vfn1 "dvasin", _dv_asin, asin
	vfn1 "dvacos", _dv_acos, acos
	vfn1 "dvatan", _dv_atan, atan
	vfn1 "dvsinh", _dv_sinh, sinh
	vfn1 "dvcosh", _dv_cosh, cosh

	.ltorg

	vfn1 "dvtanh", _dv_tanh, tanh
	vfn1 "dvasinh", _dv_asinh, asinh
	vfn1 "dvacosh", _dv_acosh, acosh
	vfn1 "dvatanh", _dv_atanh, atanh
	vfn1 "dvexp", _dv_exp, exp
	vfn1 "dvexpm1", _dv_expm1, expm1
	vfn1 "dvln", _dv_ln, log
	vfn1 "dvlnp1", _dv_lnp1, log1p
	vfn2 "dvatan2", _dv_atan2, atan2
	vfn2 "dvpow", _dv_pow, pow

	.ltorg

