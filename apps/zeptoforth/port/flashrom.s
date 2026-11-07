@ Copyright (c) 2013 Matthias Koch
@ Copyright (c) 2019-2024 Travis Bemann
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

@ The flash dictionary in RAM: the block at flash_dict_start, kept on the SD card by save-image and read back at the start.
@ It behaves as the flash it stands for: it starts as all ones, a byte or halfword of all ones is not written, a place
@ already written cannot be written again. The STM32F746's words otherwise, less its sectors, and the scans stop at
@ flash_dict_start, since what is below that here is the program's heap and not erased flash.

	@@ Write a byte to flash
	define_word "cflash!", visible_flag
_store_flash_1:
	movs r0, tos
	pull_tos
	movs r1, tos
	pull_tos
	ldr r2, =flash_min_address
	cmp r0, r2
	blo 5f
	ldr r3, =flash_dict_end
	cmp r0, r3
	bhs 6f
	ldr r3, =0xFF
	ands r1, r3
	cmp r1, r3
	beq 2f
	ldrb r2, [r0]
	cmp r2, r3
	bne 4f
	strb r1, [r0]
2:	bx lr
4:	push_tos
	ldr tos, =_store_flash_already_written
	bl _raise
5:	push_tos
	ldr tos, =_attempted_to_write_core_flash
	bl _raise
6:	push_tos
	ldr tos, =_attempted_to_write_past_flash_end
	bl _raise
	bx lr
	end_inlined

	@@ Write a halfword to flash
	define_word "hflash!", visible_flag
_store_flash_2:
	movs r0, tos
	pull_tos
	movs r1, tos
	pull_tos
	ldr r2, =flash_min_address
	cmp r0, r2
	blo 5f
	ldr r3, =flash_dict_end
	cmp r0, r3
	bhs 6f
	tst r0, #1
	bne 7f
	ldr r3, =0xFFFF
	ands r1, r3
	cmp r1, r3
	beq 2f
	ldrh r2, [r0]
	cmp r2, r3
	bne 4f
	strh r1, [r0]
2:	bx lr
4:	push_tos
	ldr tos, =_store_flash_already_written
	bl _raise
5:	push_tos
	ldr tos, =_attempted_to_write_core_flash
	bl _raise
6:	push_tos
	ldr tos, =_attempted_to_write_past_flash_end
	bl _raise
7:	push_tos
	ldr tos, =_store_flash_unaligned
	bl _raise
	bx lr
	end_inlined

	@@ Exception handler for unaligned flash writes
	define_internal_word "flash!-unaligned", visible_flag
_store_flash_unaligned:
	push {lr}
	string_ln "unaligned flash write"
	bl _type
	pop {pc}
	end_inlined

	@@ Exception handler for flash writes where flash has already been
	@@ written
	define_word "x-flash-already-written", visible_flag
_store_flash_already_written:
	push {lr}
	string_ln "flash already written"
	bl _type
	pop {pc}
	end_inlined

	@@ Exception handler for flash writes where flash has already been
	@@ written
        define_word "x-write-core-flash", visible_flag
_attempted_to_write_core_flash:
	push {lr}
	string_ln "attempted to write to core flash"
	bl _type
	pop {pc}
	end_inlined

	@@ Exception handler for flash writes past the end of flash
        define_word "x-write-past-flash-end", visible_flag
_attempted_to_write_past_flash_end:
	push {lr}
	string_ln "attempted to write past flash end"
	bl _type
	pop {pc}
	end_inlined

	@@ Erase after a given address, then start again
	define_internal_word "erase-after", visible_flag
_erase_after:
	push {lr}
	movs r1, #0					@@ (the tick's words are in what is about to go)
	ldr r0, =zf_armed
	str r1, [r0]
	ldr r0, =0xFFFFFFFF
	ldr r1, =flash_dict_end
	bics tos, #3
1:	cmp tos, r1
	bhs 2f
	str r0, [tos]
	adds tos, #4
	b 1b
2:	bl _reboot
	pop {pc}
	end_inlined

	@@ Erase all flash except for the zeptoforth runtime
	define_word "erase-all", visible_flag
_erase_all:
	push {lr}
	push_tos
	ldr tos, =flash_dict_start
	bl _erase_after
	pop {pc}
	end_inlined

	@@ Find the end of the flash dictionary: with nothing in it, so that next-flash-block gives flash_dict_start
	define_internal_word "find-flash-end", visible_flag
_find_flash_end:
	push_tos
	ldr tos, =flash_dict_end
	ldr r1, =flash_dict_start
1:	subs tos, #4
	cmp tos, r1
	blo 2f
	ldr r0, [tos]
	adds r0, #1
	beq 1b
	bx lr
2:	ldr tos, =flash_dict_start - 8
	bx lr
	end_inlined

	@@ Find the next flash block
	define_internal_word "next-flash-block", visible_flag
_next_flash_block:
	bics tos, #3
	adds tos, #8
	bx lr
	end_inlined

	@@ Find the start of the last flash word: the kernel's last, if there is none in the flash dictionary
	define_internal_word "find-last-flash-word", visible_flag
_find_last_flash_word:
	ldr r1, =0xDEADBEEF
	ldr r2, =flash_dict_start
1:	subs tos, #4
	cmp tos, r2
	blo 2f
	ldr r0, [tos]
	cmp r0, r1
	bne 1b
3:	subs tos, #4
	cmp tos, r2
	blo 2f
	ldr r0, [tos]
	cmp r0, r1
	beq 3b
	ldr tos, [tos]
	bx lr
2:	ldr tos, =last_kernel_word
	bx lr
	end_inlined

	@@ Write a word to an address in one or more flash_buffers
	define_word "flash!", visible_flag
_store_flash_4:
	push {lr}
	movs r0, tos
	pull_tos
	push {r0, tos}
	ldr r1, =0xFFFF
	ands tos, r1
	push_tos
	movs tos, r0
	bl _store_flash_2
	push_tos
	pop {r0, tos}
	lsrs tos, tos, #16
	ldr r1, =0xFFFF
	ands tos, r1
	push_tos
	movs tos, r0
	adds tos, #2
	bl _store_flash_2
	pop {pc}
	end_inlined

	@@ Write a word to an address in one or more flash_buffers
	define_word "2flash!", visible_flag
_store_flash_8:
	push {lr}
	push {tos}
	bl _store_flash_4
	push_tos
	pop {tos}
	adds tos, #4
	bl _store_flash_4
	pop {pc}
	end_inlined

	@@ Flush all the buffered flash
	define_internal_word "flush-all-flash", visible_flag
_flush_all_flash:
	push {lr}
	pop {pc}
	end_inlined

	@@ Fill flash until it is aligned to a block
	define_word "flash-block-align,", visible_flag
_flash_block_align:
	push {lr}
	pop {pc}
	end_inlined

	@@ Get the flash block size in bytes
	define_word "flash-block-size", visible_flag | inlined_flag
_flash_block_size:
	push_tos
	ldr tos, =flash_block_size
	bx lr
	end_inlined

	@@ Initialize the flash buffers
	define_internal_word "init-flash-buffers", visible_flag
_init_flash_buffers:
	bx lr
	end_inlined

        @ Placeholder for cflash-buffer@ on other architectures, same as c@
        define_internal_word "cflash-buffer@", visible_flag
_get_flash_buffer_value_1:
        ldrb tos, [tos]
        bx lr
        end_inlined

        @ Placeholder for hflash-buffer@ on other architectures, same as h@
        define_internal_word "hflash-buffer@", visible_flag
_get_flash_buffer_value_2:
        ldrh tos, [tos]
        bx lr
        end_inlined

        @ Placeholder for flash-buffer@ on other architectures, same as @
        define_internal_word "flash-buffer@", visible_flag
_get_flash_buffer_value_4:
        ldr tos, [tos]
        bx lr
        end_inlined

	.ltorg

