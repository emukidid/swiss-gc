#***************************************************************************
# Device indepentent Read code launch pad
#
# we have 0x1800 bytes to play with at 0x80001800 (code+data), or use above Arena Hi
# This code is placed either at 0x80001800 or Above Arena Hi (depending on the game)
#**************************************************************************
#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.section .text
	.globl _start, __main
_start:
	b		queue_read
	b		patched_memcpy			# void patched_memcpy(dst,src,size)
	b		fake_interrupt
	b		calculate_speed			# Calculates how long it takes to read 2048 bytes from the device
	b		read_immediately		# Performs a blocking read.
	b		enable_backup_disc		# Re-authenticates a DVD-R disc

	.globl fake_interrupt
fake_interrupt:
	mflr    r0
	stw     r0, 4(sp)
	stwu    sp, -0x60(sp)
	stmw    r19, 0x2C(sp)
	
	mr		r20,	r3
	mr		r21,	r4
	mr		r22,	r12

#check if we've set our fake IRQ & we have our handler address ready.
	lis		r3,	VAR_AREA
	lwz		r0,	VAR_FAKE_IRQ_SET(r3)	# r3 = our fake IRQ
	cmpwi	r0,	0
	beq		no_fake_irq
	lwz		r0,	VAR_DVDIRQ_HNDLR(r3)	# r3 = addr of __DVDInterruptHandler
	cmpwi	r0,	0
	beq		not_ready_yet
	li		r0,	0
	stw		r0,	VAR_FAKE_IRQ_SET(r3)	# zero out our fake IRQ
	lwz		r12,VAR_DVDIRQ_HNDLR(r3)	# r12 = addr of __DVDInterruptHandler
	stw		r0,	VAR_DVDIRQ_HNDLR(r3)	# zero out our addr of __DVDInterruptHandler in case the next DOL is loaded
		
	lwz		r4, VAR_READ_DVDSTRUCT+16(r3)	# load length of last read
	lwz		r3, VAR_READ_DVDSTRUCT+12(r3)	# load dst of last read
	bl		dcache_flush_icache_inv			# **TODO** ctr is mangled here, but should be OK?
	
	mtlr	r12

	mr		r4,	r21
	mr		r3,	r20
	blrl							# jump to the handler

not_ready_yet:
no_fake_irq:
	mr		r12,r22
	mr		r4,	r21
	mr		r3,	r20

	mtlr	r12
	blrl
	
	lmw     r19,0x2C(sp)
	lwz     r0,	0x64(sp)
	addi    sp,	sp,	0x60
	mtlr    r0
	blr
	
	.globl queue_read
queue_read:
	mflr    	r0             		# move lr  
	stwu    	sp,  -64(sp)   		# grow stack frame 
	stw     	r0,   28(sp)    	# save lr
	stw			r3,	12(sp)
	stw			r4,	16(sp)
	stw			r5,	20(sp)
	stw			r6,	24(sp)
	bl			add_read_to_queue
	lwz			r3,	12(sp)
	lwz			r4,	16(sp)
	lwz			r5,	20(sp)
	lwz			r6,	24(sp)
	lwz     	r0,	28(sp)    		# load lr
	addi    	sp,  sp, 64   		# grow stack frame 
	mtlr    	r0             		# move lr  
	li			r0, 	0			# patched over this
	blr
	
	.globl read_immediately
read_immediately:
	stw         r0, 4 (1)			# overwritten, so lets do it here.
	mflr    	r0             		# move lr  
	stwu    	r1,  -64(sp)   		# grow stack frame 
	stw     	r0,   28(sp)    	# save lr
	stw			r3,	12(sp)
	stw			r4,	16(sp)
	stw			r5,	20(sp)
	stw			r6,	24(sp)
	bl			read_entire
	lwz			r3,	12(sp)
	lwz			r4,	16(sp)
	lwz			r5,	20(sp)
	lwz			r6,	24(sp)
	lwz     	r0,	28(sp)    		# load lr
	addi    	sp,  sp, 64   		# grow stack frame 
	mtlr    	r0             		# move lr  
	li			r3, 0				# dst = 0
	li			r4,	0				# len = 0
	li			r5,	0				# ofs = 0
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
	icbi		r0, r3
	addi 		r3, r3, 0x20
	bdnz 		2b
	sc
	sync
	isync
	blr

	.globl process_read_queue
process_read_queue:
	stwu	sp,	-160(sp) 		# grow stack frame
	stw		r0,   12(sp)		# store r0
	stmw	r3,   16(sp)		# store r3-r31

	bl		process_queue

	lwz		r0, 12(sp)			# restore r0
	lwz		r4,	20(sp)			# restore r4 (context)
	lwz     r3, 128 (r4)
	mtcr    r3					# restore cr
	lwz     r3, 0x0084 (r4)
	mtlr    r3					# restore lr
	lwz     r3, 0x0088 (r4)
	mtctr   r3					# restore ctr
	lwz     r3, 0x008C (r4)
	mtxer   r3					# restore xer
	lmw 	r3, 16(sp)			# restore r3-r31
	addi	r1,	r1, 160			# kill stack frame
	lis		r5,	0x8000
	lwz		r5, 0x3010(r5)
	mtsrr0	r5					# setup for the rfi
	rfi							# jump to handler

.globl patched_memcpy
patched_memcpy:
	cmpwi	r3, 0x0500
	mr		r4, r24
	bnelr
	lis		r4, ext_handler_custom@h
	ori		r4, r4, ext_handler_custom@l
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
	rlwinm.     r5, r5, 0, 30, 30
	bne-        ext_h
	lis         r5, 0x8008
	subi        r5, r5, 28720			#hmmm? this should be set per game.
	mtsrr0      r5
	rfi
ext_h:
	lis			r5, process_read_queue@h
	ori			r5, r5, process_read_queue@l
	mtsrr0      r5
	rfi

	
	.globl enable_backup_disc
enable_backup_disc:
	mflr	r0
	stw		r0,4(sp)
	stwu	sp,-40(sp)
	stmw	r26,8(sp)

	mfmsr	r5
	rlwinm	r5,r5,0,17,15
	mtmsr	r5

	bl		_unlockdvd

	li		r3,1
	bl		_enableextensions

	li		r3,0x4100
	bl		_spinmotor

	lis		r3,0x0006
	ori		r3,r3,0x0300
	bl		_setstatus

	mfmsr	r5
	ori		r5,r5,0x8000
	mtmsr	r5

	lmw		r26,8(sp)
	lwz		r0,44(sp)
	addi	sp,sp,40
	mtlr	r0
	blr

	.globl _unlockdvd
_unlockdvd:
	lis		r3,0xcc00
	ori		r3,r3,0x6000
	lwz		r5,0(r3)
	ori		r5,r5,0x14
	stw		r5,0(r3)
	lwz		r5,4(r3)
	stw		r5,4(r3)
	lis		r5,0xff01
	ori		r5,r5,0x4d41
	stw		r5,8(r3)
	lis		r5,0x5453
	ori		r5,r5,0x4849
	stw		r5,12(r3)
	lis		r5,0x5441
	ori		r5,r5,0x0200
	stw		r5,16(r3)
	li		r5,0x01
	stw		r5,28(r3)
1:	lwz		r5,0(r3)
	andi.	r5,r5,0x14
	beq+	1b

	lwz		r5,0(r3)
	ori		r5,r5,0x14
	stw		r5,0(r3)
	lwz		r5,4(r3)
	stw		r5,4(r3)
	lis		r5,0xff00
	ori		r5,r5,0x4456
	stw		r5,8(r3)
	lis		r5,0x442d
	ori		r5,r5,0x4741
	stw		r5,12(r3)
	lis		r5,0x4d45
	ori		r5,r5,0x0300
	stw		r5,16(r3)
	li		r5,0x01
	stw		r5,28(r3)
2:	lwz		r5,0(r3)
	andi.	r5,r5,0x14
	beq+	2b
	blr

	.globl _spinmotor
_spinmotor:
	lis		r4,0xcc00
	ori		r4,r4,0x6000
	lwz		r5,0(r4)
	ori		r5,r5,0x14
	stw		r5,0(r4)
	lwz		r5,4(r4)
	stw		r5,4(r4)
	lis		r5,0xfe11
	or		r5,r5,r3
	stw		r5,8(r4)
	li		r5,0
	stw		r5,12(r4)
	stw		r5,16(r4)
	li		r5,0x01
	stw		r5,28(r4)
1:	lwz		r5,0(r4)
	andi.	r5,r5,0x14
	beq+	1b
	blr

	.globl _enableextensions
_enableextensions:
	lis		r4,0xcc00
	ori		r4,r4,0x6000
	lwz		r5,0(r4)
	ori		r5,r5,0x14
	stw		r5,0(r4)
	lwz		r5,4(r4)
	stw		r5,4(r4)
	lis		r5,0x5500
	insrwi	r5,r3,8,8 
	stw		r5,8(r4)
	li		r5,0
	stw		r5,12(r4)
	stw		r5,16(r4)
	li		r5,0x01
	stw		r5,28(r4)
1:	lwz		r5,0(r4)
	andi.	r5,r5,0x14
	beq+	1b
	blr

	.globl _setstatus
_setstatus:
	lis		r4,0xcc00
	ori		r4,r4,0x6000
	lwz		r5,0(r4)
	ori		r5,r5,0x14
	stw		r5,0(r4)
	lwz		r5,4(r4)
	stw		r5,4(r4)
	lis		r5,0xEE00
	insrwi	r5,r3,24,8
	stw		r5,8(r4)
	li		r5,0
	stw		r5,12(r4)
	stw		r5,16(r4)
	li		r5,0x01
	stw		r5,28(r4)
1:	lwz		r5,0(r4)
	andi.	r5,r5,0x14
	beq+	1b
	blr