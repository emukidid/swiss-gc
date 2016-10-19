#***************************************************************************
# Device indepentent Read code launch pad
#
# we have 0x1800 bytes to play with at 0x80001800 (code+data)
# This code is placed at 0x80001800
#**************************************************************************
#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.extern __DSPHandler
.section .text
	.globl _start, __main
_start:
	b		patched_memcpy			# void patched_memcpy(dst,src,size)
	b		calculate_speed			# Calculates how long it takes to read 2048 bytes from the device
	b		stop_di_irq
	b		trigger_di_irq
	b		__DSPHandler
	b		patched_memcpy_dbg
	b		check_exit				# checks for a button combo and exits if true

	.globl check_exit
check_exit:
	lwz		r5, 0(r4)
	rlwinm	r5, r5, 16, 16, 31
	andi.	r5, r5, 0x0234				# B,Z,R,D-Pad Down
	cmplwi	r5, 0x0234
	beq		do_exit
	blr
do_exit:
	b		exit_to_pref

	.globl trigger_di_irq
trigger_di_irq:
	stwu	sp,	-64(sp) 		# grow stack frame
	mflr	r0
	stw		r0,   12(sp)		# store lr

	mfmsr	r5
	stw		r5,	  32(sp)		# store old msr
	rlwinm	r5,r5,0,17,15
	mtmsr	r5
	
	bl		DIUpdateRegisters

	lwz		r5,	  32(sp)		# restore old msr
	mtmsr	r5
	lwz		r0,   12(sp)		# restore lr
	mtlr	r0
	addi	sp, sp, 64
	blr

	.globl stop_di_irq
stop_di_irq:
	lis		r3, 0xCC00
	lwz		r0, 0x6000(r3)
	stw		r0, 0x6000(r3)
	li		r0, 0
	lis		r3, VAR_AREA
	stw		r0, VAR_FAKE_IRQ_SET(r3)
	blr

	.globl dcache_store
dcache_store:
	addi	r4, r4, 0x1F
	srwi	r4, r4, 5
	mtctr	r4
	li		r0, 0
dcache_store_loop:
	dcbst	r0, r3
	addi	r3, r3, 0x20
	bdnz	dcache_store_loop
	sync
	blr

	.globl dcache_flush_icache_inv
dcache_flush_icache_inv:
	clrlwi. 	5, 3, 27  # check for lower bits set in address
	beq 1f
	addi 		r4, r4, 0x20 
1:
	addi 		r4, r4, 0x1f
	srwi 		r4, r4, 5
	mtctr 		r4
2:
	dcbf 		r0, r3
	icbi		r0, r3		#todo kill this off
	addi 		r3, r3, 0x20
	bdnz 		2b
	sc
	sync
	isync
	blr

	.globl process_read_queue
process_read_queue:
	stwu	sp,	-64(sp) 		# grow stack frame
	stw		r0,   12(sp)		# store r0
	stw		r3,   16(sp)		# store r3
	addi	sp,	sp,	20
	stswi	r4,	sp,	(12-4)*4# store r4-r12
	addi	sp,	sp,	-20
	
	bl		StreamUpdateRegisters
	bl		DIUpdateRegisters

	lwz		r0, 12(sp)			# restore r0
	lwz		r4,	20(sp)			# restore r4 (context)
	lwz     r3, 128 (r4)
	mtcr    r3					# restore cr	# Necessary?
	lwz     r3, 0x0084 (r4)                     # Necessary?
	mtlr    r3					# restore lr    # Necessary?
	lwz     r3, 0x0088 (r4)                     # Necessary?
	mtctr   r3					# restore ctr   # Necessary?
	lwz     r3, 0x008C (r4)                     # Necessary?
	mtxer   r3					# restore xer   # Necessary?
	lwz		r3,	16(sp)			# restore r3
	addi	sp,		sp,	20
	lswi	r4,	sp,	(12-4)*4	# restore r4-r12
	addi	sp,	sp, 64-20		# kill stack frame
	lis		r5,	0x8000
	lwz		r5, 0x3010(r5)
	mtsrr0	r5					# setup for the rfi
	rfi							# jump to handler

.globl patched_memcpy
patched_memcpy:
	mr		r4, r24
	cmpwi	r3, 0x0500
	beq		customhandler
	cmpwi	r3, 0xF00
	bgt		redirecthandler
	blr
customhandler:
	lis		r4, ext_handler_custom@h
	ori		r4, r4, ext_handler_custom@l
	blr
redirecthandler:
	li		r3, 0xF00
	blr
	
.globl patched_memcpy_dbg
patched_memcpy_dbg:
	mr		r4, r24
	rlwinm	r3, r3, 0, 16, 31
	cmpwi	r3, 0x0500
	beq		customhandlerdbg
	cmpwi	r3, 0xF00
	addis	r3, r3, 0x8000
	bgt		redirecthandlerdbg
	blr
customhandlerdbg:
	lis		r4, ext_handler_custom@h
	ori		r4, r4, ext_handler_custom@l
	addis	r3, r3, 0x8000
	blr
redirecthandlerdbg:
	li		r3, 0xF00
	blr

.globl ext_handler_custom
ext_handler_custom:
	mtsprg0     r4
	lwz         r4, 0x00C0 (r0)
	stw         r3, 12 (r4)
	mfsprg0     r3
	stw         r3, 16 (r4)
	stw         r5, 20 (r4)
	lhz         r3, 0x01A2 (r4)
	ori         r3, r3, 0x0002
	sth         r3, 0x01A2 (r4)
	mfcr        r3
	stw         r3, 128 (r4)
	mflr        r3
	stw         r3, 0x0084 (r4)
	mfctr       r3
	stw         r3, 0x0088 (r4)
	mfxer       r3
	stw         r3, 0x008C (r4)
	mfsrr0      r3
	stw         r3, 0x0198 (r4)
	mfsrr1      r3
	stw         r3, 0x019C (r4)
	mr          r5, r3
	nop
	mfmsr       r3
	ori         r3, r3, 0x0030
	mtsrr1      r3
	li          r3, 4
	lwz         r4, 0x00D4 (r0)
	lis			r5, process_read_queue@h
	ori			r5, r5, process_read_queue@l
	mtsrr0      r5
	rfi
