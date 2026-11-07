@ Copyright (c) 2013 Matthias Koch
@ Copyright (c) 2020-2023 Travis Bemann
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

@ The console: keys and characters through zf_cmm2.c to zf_console.c, the terminal on the screen and the serial port, so
@ Forth reads and writes the screen and USB keyboard as well as the serial port. The words keep the names the kernel's
@ hooks are set to. A few words of the CMM2's own follow them.

        .equ CTRL_C          ,   3

	@@ Nothing to set up but the reboot hook, which runs before the kernel would reset the processor: a program may
	@@ not, so reboot starts Forth again, as it does on a board, the flash dictionary kept and the RAM one gone
	define_internal_word "uart-init", visible_flag
_uart_init:
	ldr r0, =reboot_hook
	ldr r1, =_handle_reset
	str r1, [r0]
	@@ Colours if the terminal has the screen: the firmware's console would print them as text
	ldr r0, =color_enabled
	ldr r1, =zf_con_live
	ldr r1, [r1]
	str r1, [r0]
	bx lr
	end_inlined

	@@ Emit one character ( c -- ). No pause, unlike the UART's words, which have a wait to fill: con_putc does the waiting,
	@@ the tick shares the time out, and a pause here cost the rest of the task's slice on every character
	define_internal_word "serial-emit", visible_flag
_serial_emit:
	push {r4, lr}
	movs r0, tos
	pull_tos
	ccall zf_emit
	pop {r4, pc}
	end_inlined

	@@ Receive one character ( -- c )
	define_internal_word "serial-key", visible_flag
_serial_key:
	push {r4, lr}
1:	bl _serial_key_q
	cmp tos, #0
	pull_tos
	beq 1b
	ccall zf_key
	push_tos
	movs tos, r0
	pop {r4, pc}
	end_inlined

	@@ Test whether a character may be emitted ( -- flag )
	define_internal_word "serial-emit?", visible_flag
_serial_emit_q:
	push_tos
	movs tos, #-1
	bx lr
	end_inlined

	@@ Test whether a character is ready be received ( -- flag ): zf_key_q says 1 for a key, 0 for none, -1 for
	@@ Ctrl-C, which reboots as it does on the UART
	define_internal_word "serial-key?", visible_flag
_serial_key_q:
	push {r4, lr}
	bl _pause
	ccall zf_key_q
	cmp r0, #0
	bpl 1f
	ldr r1, =uart_special_enabled
	ldr r1, [r1]
	cmp r1, #0
	beq 2f
	bl _reboot
2:	movs r0, #0
1:	push_tos
	movs tos, #0
	cmp r0, #0
	beq 3f
	movs tos, #-1
3:	pop {r4, pc}
	end_inlined

	@@ What a bad address raises
	define_word "x-bad-address", visible_flag
_zf_bad_address:
	push {lr}
	string_ln "bad address"
	bl _type
	pop {pc}
	end_inlined

	@@ Where a fault in Forth's own code sends the thread (zf_cmm2.c): the exception, as if the word had raised it.
	@@ ?raise puts the stacks back
	.global zf_fault_forth
	.thumb_func
zf_fault_forth:
	cpsie i
	push_tos
	ldr tos, =_zf_bad_address
	b _raise

	@@ And one in C or a task switch, where nothing can be trusted to go on from: on C's stack to say so, then Forth
	@@ starts again, as at Ctrl-C
	.global zf_fault_restart
	.thumb_func
zf_fault_restart:
	cpsie i
	ldr r0, =zf_c_sp
	ldr r0, [r0]
	mov sp, r0
	bl zf_fault_said
	b _handle_reset

	.ltorg

	@@ Back to MMBasic ( -- )
	define_word "bye", visible_flag
_zf_bye:
	push {r4, lr}
	ccall zf_bye
	pop {r4, pc}
	end_inlined

	@@ Keep the flash dictionary on the SD card, for the next start ( -- )
	define_word "save-image", visible_flag
_zf_save:
	push {r4, lr}
	ccall zf_save
	pop {r4, pc}
	end_inlined

	@@ Make what has just been compiled fit to run ( -- ): the memory is cached, and code written through the data
	@@ cache is not yet what the instruction cache will fetch
	define_word "sync-code", visible_flag
_sync_code:
	push {r4, lr}
	ccall zf_sync_code
	pop {r4, pc}
	end_inlined

	.ltorg

