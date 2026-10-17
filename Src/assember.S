.syntax unified
.syntax unified

.section .itcm.asm,"ax",%progbits      @ (in ITCM: the linker script's .ITCMSection takes .itcm*)
.balign 4
.global clearpage
.thumb_func                @ (a Thumb function: its address has bit 0 set, so a long branch to it stays in Thumb)
clearpage:  PUSH {R4,R5}
			movs r4,#0
			movs r5,#0
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			strd r4,r5,[r0],#8
			POP {R4,R5}
			bx lr

.global varnamecopy
.thumb_func                @ (a Thumb function: its address has bit 0 set, so a long branch to it stays in Thumb)
varnamecopy: 	PUSH {R4,R5}
			LDRD r4,r5,[r1],#8
			strD r4,r5,[r0],#8
			LDRD r4,r5,[r1],#8
			strD r4,r5,[r0],#8
			LDRD r4,r5,[r1],#8
			strD r4,r5,[r0],#8
			LDRD r4,r5,[r1],#8
			strD r4,r5,[r0],#8
			POP {R4,R5}
			bx lr

.global cleardims
.thumb_func                @ (a Thumb function: its address has bit 0 set, so a long branch to it stays in Thumb)
cleardims:   PUSH {R5}
			movs r5,#0
			str r5,[r0]
			str r5,[r0,#4]
			str r5,[r0,#8]
			str r5,[r0,#12]
			str r5,[r0,#16]
			POP {R5}
			bx lr

.global clearvar
.thumb_func                @ (a Thumb function: its address has bit 0 set, so a long branch to it stays in Thumb)
clearvar:   PUSH {R5}
			movs r5,#0
			str r5,[r0]
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
			str r5,[r0],#4
		    POP {R5}
			bx lr
