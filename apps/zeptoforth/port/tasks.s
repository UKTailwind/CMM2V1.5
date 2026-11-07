@ Task switching and preemption on the CMM2. MIT licence, as zeptoforth.
@
@ On a board of its own zeptoforth switches tasks in PendSV, each task's state an exception frame on its own stack. Here
@ PendSV and the stack it runs on are the firmware's, so all of it happens in thread mode, around zeptoforth's own
@ switch-tasks: a task is suspended by the same frame, r0 to r3, r12, lr, pc and xPSR, so init-context's first frame and
@ task-force-call's change to a frame's pc both work as they are, but it is built and taken apart by these routines.
@
@ cmm2-switch is what pause does in place of pending PendSV. zf_preempt is where the firmware's tick (zf_frame in
@ zf_cmm2.c) sends the thread, by changing the pc in the frame of the exception it interrupted: the thread "calls" it at
@ whatever it was doing, every register as it was, and it saves them in the same frame. Nothing is done while the thread
@ is in C or here, zf_in_c and zf_in_switch saying so. A place in an IT block cannot be returned to from thread mode, so
@ zf_frame leaves those for the next tick.
@
@ A word of the frame's xPSR that exception entry would leave as an exception number holds PRIMASK in bit 0: pause is
@ often called with interrupts off, where PendSV would wait for them. Here the switch happens at once, so they are put
@ on for it, or an idle system would sleep with the firmware's interrupts off, and off again on the way back.

	.equ frame_size, 32

	@@ Switch tasks: pause's job ( -- ). Inside a switch (a pending operation may pause), do-pause? is enough
	define_word "cmm2-switch", visible_flag
_zf_switch:
	ldr r12, =zf_in_switch
	ldr r12, [r12]
	cmp r12, #0
	beq 1f
	bx lr
1:	sub sp, #frame_size
	str r0, [sp, #0]
	str r1, [sp, #4]
	str r2, [sp, #8]
	str r3, [sp, #12]
	str r12, [sp, #16]
	str lr, [sp, #20]
	str lr, [sp, #24]
	mrs r0, APSR
	mrs r1, PRIMASK
	orr r0, r1
	str r0, [sp, #28]
	movs r1, #1
	ldr r0, =zf_in_switch
	str r1, [r0]
	cpsie i
_zf_run_switch:
	ldr lr, =_zf_frame_return + 1
	ldr r0, =zf_switch_xt
	ldr r0, [r0]
	orr r0, #1
	bx r0
	end_inlined

	@@ switch-tasks returns here, on the stack of the task to go on with: its frame comes off. Interrupted here the
	@@ task is only preempted again, the frame above sp being left alone
_zf_frame_return:
	movs r1, #0
	ldr r0, =zf_in_switch
	str r1, [r0]
	ldr r0, [sp, #24]
	orr r0, #1
	str r0, [sp, #24]
	ldr r0, [sp, #28]
	tst r0, #1
	beq 1f
	cpsid i
1:	tst r0, #0x200
	bne 2f
	msr APSR_nzcvqg, r0
	pop {r0, r1, r2, r3, r12, lr}
	ldr pc, [sp], #8
2:	msr APSR_nzcvqg, r0				@@ (init-context's frame with a word of alignment)
	pop {r0, r1, r2, r3, r12, lr}
	ldr pc, [sp], #12
	end_inlined

	@@ A tick came during C or a float word and left zf_pending, so that a task living in the maths library is preempted
	@@ as often as any other: tick_check comes here on the way out, with lr where to go on from. Every register is kept
	@@ by zf_preempt's frame, r0 being C's answer
_zf_pend:
	push {r0, r1}
	movs r1, #0
	ldr r0, =zf_pending
	str r1, [r0]
	ldr r0, =zf_in_switch
	ldr r1, [r0]
	cmp r1, #0
	bne 1f
	movs r1, #1
	str r1, [r0]
	ldr r0, =zf_resume_pc
	str lr, [r0]
	pop {r0, r1}
	b zf_preempt
1:	pop {r0, r1}
	bx lr
	end_inlined

	@@ The tick, in thread mode. zf_tick does the firmware's polling, and says if Ctrl-C has been pressed: with the
	@@ thread stopped wherever it was, that is what stops a word that never ends. Then the tasker's word, which decides
	@@ if the time slice is up, and switch-tasks, which does nothing if it is not
	.global zf_preempt
	.thumb_func
zf_preempt:
	sub sp, #frame_size
	str r0, [sp, #0]
	str r1, [sp, #4]
	str r2, [sp, #8]
	str r3, [sp, #12]
	str r12, [sp, #16]
	str lr, [sp, #20]
	mrs r0, APSR
	str r0, [sp, #28]
	ldr r0, =zf_resume_pc
	ldr r0, [r0]
	str r0, [sp, #24]
	push {r4}
	ccall zf_tick
	pop {r4}
	cmp r0, #0
	beq 1f
	ldr r1, =uart_special_enabled
	ldr r1, [r1]
	cmp r1, #0
	beq 1f
	bl _reboot
1:	ldr r0, =zf_tick_xt
	ldr r0, [r0]
	cmp r0, #0
	beq 2f
	orr r0, #1
	blx r0
	b _zf_run_switch
2:	b _zf_frame_return
	end_inlined

	@@ Wait for something to happen, with no task ready ( -- ): the firmware is still looked after
	define_word "cmm2-idle", visible_flag
_zf_idle:
	push {r4, lr}
	ccall zf_idle
	cmp r0, #0
	beq 1f
	ldr r1, =uart_special_enabled
	ldr r1, [r1]
	cmp r1, #0
	beq 1f
	bl _reboot
1:	pop {r4, pc}
	end_inlined

	@@ The time in 100 us ticks, which is what zeptoforth counts in ( -- ticks )
	define_word "cmm2-ticks", visible_flag
_zf_ticks:
	push {r4, lr}
	ccall zf_ticks
	push_tos
	movs tos, r0
	pop {r4, pc}
	end_inlined

	@@ What the tasker runs on: the tick's word and switch-tasks ( tick-xt switch-xt -- )
	define_word "cmm2-hooks!", visible_flag
_zf_hooks:
	ldr r0, =zf_switch_xt
	str tos, [r0]
	pull_tos
	ldr r0, =zf_tick_xt
	str tos, [r0]
	pull_tos
	bx lr
	end_inlined

	.ltorg

