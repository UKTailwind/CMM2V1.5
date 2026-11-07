@ Copyright (c) 2019-2026 Travis Bemann
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

@ The CMM2's memory, as zeptoforth wants it: the kernel is linked into the program, and what it knows as flash and
@ RAM are fixed blocks above the program and its heap, which is the console's: of the G2's 8 MB, or on a G1, linked at
@ a fixed place in its heap (ZF_G1, zf_g1.c), of what is left there. zf_cmm2.c checks that the loader gave them.

	.equ thumb2, 1
        .equ chip0, 0x73746D
        .equ chip1, 0x660002EA
	.equ cortex_m7, 1
        .equ cortex_m33, 0
	.ifdef ZF_G1
	.equ ram_start, 0xD0610000
	.equ ram_end, 0xD0710000
	.equ flash_start, 0xD0400000
	.equ flash_dict_start, 0xD0550000
	.equ flash_dict_end, 0xD0610000
	.else
	.equ ram_start, 0xD1C00000
	.equ ram_end, 0xD1F00000
	.equ flash_start, 0xD1800000
	.equ flash_dict_start, 0xD1A00000
	.equ flash_dict_end, 0xD1C00000
	.endif
	.equ rstack_size, 0x10000
	.equ rstack_top, ram_end
	.equ stack_size, 0x10000
	.equ stack_top, ram_end - rstack_size
	.equ vector_count, 16
	.equ vector_table_size, vector_count * 4 @ in bytes
	.equ VTOR, 0xE000ED08
	.equ VTOR_value, vector_table
	.equ flash_buffers_top, stack_top - stack_size
	.equ flash_block_size, 1 @ in bytes
	.equ flash_buffer_count, 0
	.equ flash_buffer_size, 0
	.equ flash_buffer_space, 0
	.equ flash_buffer_addr, 0
	.equ flash_min_address, flash_dict_start
	.equ input_buffer_size, 255
	.equ pad_offset, 128
	.equ flash_mini_dict_size, 0 @ in bytes
	.equ cpu_count, 1
	.equ syntax_stack_size, 64 @ in bytes
