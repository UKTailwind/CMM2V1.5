@ Copyright (c) 2019-2025 Travis Bemann
@
@ Permission is hereby granted, free of charge, to any person obtaining a copy
@ of this software and associated documentation files (the "Software"), to deal
@ in the Software without restriction, including without limitation the rights
@ to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
@ copies of the Software, and to permit persons to whom the Software is
@ furnished to do so, subject to the following conditions:
@ 
@ The above copyright notice and this permission notice shall be included in
@ all copies or substantial portions of the Software.
@ 
@ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
@ IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
@ FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
@ AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
@ LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
@ OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
@ SOFTWARE.

@ The kernel's top file for the CMM2: the STM32F746's, less what a program may not do. The firmware keeps the vectors, the
@ clocks and the console's UART, so the vector table is never installed and uart-init only sets the reboot hook.

	.syntax	unified
	.cpu cortex-m7
	.thumb

	.include "config.s"
	.include "../m4_m7/macro.s"
	.include "variables.s"

	@@ A call from the kernel to C: on C's own stack, as a task's return stack may be small and a service will not be,
	@@ with r4 (which the caller has pushed) keeping sp. zf_in_c keeps preemption out while it lasts: the services
	@@ are not reentrant. C keeps r4 to r11, so tos and dp survive; r0 to r3 are scratch in the kernel as they are in C
	.macro ccall fn
	ldr r12, =zf_in_c
	str r12, [r12]
	mov r4, sp
	ldr r12, =zf_c_sp
	ldr r12, [r12]
	mov sp, r12
	bl \fn
	mov sp, r4
	movs r1, #0
	ldr r12, =zf_in_c
	str r1, [r12]
	tick_check
	.endm

	@@ If a tick came while the thread was in C, it takes effect now (port/tasks.s), r0 to r3 kept. lr goes, so the caller
	@@ has pushed it
	.macro tick_check
	ldr r12, =zf_pending
	ldr r12, [r12]
	cmp r12, #0
	beq 9f
	bl _zf_pend
9:
	.endm

	.text

	.include "../common/vectors.s"

	@@ The first (null) dictionary entry
	.p2align 2
	.word invisible_flag
	.word 0
10:	.byte 0
	.p2align 2

	@@ The entry point, from zf_cmm2.c, and where the reboot word comes back to
	.global zf_reset
	.thumb_func
zf_reset:
_handle_reset:
	@@ No preemption, and no tasks, until they are set up again (port/tasks.s)
	movs r1, #0
	ldr r0, =zf_armed
	str r1, [r0]
	ldr r0, =zf_in_switch
	str r1, [r0]
	ldr r0, =zf_in_c
	str r1, [r0]
	ldr r0, =zf_tick_xt
	str r1, [r0]
	ldr r0, =zf_switch_xt
	str r1, [r0]
	cpsie i
	@@ Initialize r11, relied upon by swdcom
	mov r11, #0
	@@ Initialize the top of stack register
	ldr tos, =0xFEDCBA98
	@@ Get the dictionary base
	ldr r0, =dict_base
	ldr r1, =ram_current
	str r1, [r0]
	@@ Initialize HERE
	ldr r0, =ram_current + ram_here_offset
	ldr r1, =ram_current + user_offset
	str r1, [r0]
	@@ Initialize the data stack pointer
	ldr r1, =stack_top
	movs dp, r1
	@@ Initialize the return stack pointer
	ldr r1, =rstack_top
	mov sp, r1
	@@ Put a garbage value in HANDLER to force a crash if is used
	ldr r0, =ram_current + handler_offset
	ldr r1, =0xF0E1C2D3
	str r1, [r0]
	@@ Call the rest of the runtime in an exception handler
	push_tos
	ldr tos, =outer_exc_handled
	bl _try
	@@ If the inner loop returns, reboot
	b _handle_reset

	@@ The outermost exception handling - if an exception happens here the
	@@ system will reboot
outer_exc_handled:
	push {r4, lr}
	ccall zf_cpu_hz
	pop {r4, lr}
        ldr r1, =sysclk
        str r0, [r1]
	bl _init_variables
	bl _init_dict
	bl _uart_init
	bl _init_flash_buffers
	movs r1, #1
	ldr r0, =zf_armed
	str r1, [r0]
	bl _do_init
	bl _do_welcome
	bl _quit

	.ltorg

	.include "flashrom.s"
	.include "console.s"
	.include "tasks.s"
	.include "float64.s"
	.include "files.s"
	.include "services.s"
	.include "expose.s"
        .include "../common/syntax.s"
	.include "../m4_m7/core.s"
	.include "../m4_m7/divide.s"
	.include "../common/outer.s"
	.include "../common/cond.s"
	.include "../m4_m7/asm.s"
	.include "../common/strings.s"
	.include "../m4_m7/double.s"
	.include "../m4_m7/exception.s"
	.include "../common/final.s"

	@@ The kernel's memory, for zf_cmm2.c
	.p2align 2
	.global zf_mem
zf_mem:	.word flash_dict_start, flash_dict_end, ram_start, ram_end, color_enabled
