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
	b		handle_card				# Looks at VAR_MC_FUNC to see what function to call
	b		calculate_speed			# Calculates how long it takes to read 2048 bytes from the device

	.globl fake_interrupt
fake_interrupt:
	mflr    0
	stw     0, 4(1)
	stwu    1, -0x60(1)
	stmw    19, 0x2C(1)
	
	mr		20,	3
	mr		21,	4
	mr		22,	12

#check if we've set our fake IRQ & we have our handler address ready.
	lis		3,	VAR_AREA
	lwz		0,	VAR_FAKE_IRQ_SET(3)	# r3 = our fake IRQ
	cmpwi	0,	0
	beq		no_fake_irq
	lwz		0,	VAR_DVDIRQ_HNDLR(3)	# r3 = addr of __DVDInterruptHandler
	cmpwi	0,	0
	beq		not_ready_yet
	li		0,	0
	stw		0,	VAR_FAKE_IRQ_SET(3)	# zero out our fake IRQ
	
	lwz		12,	VAR_DVDIRQ_HNDLR(3)	# r12 = addr of __DVDInterruptHandler
	stw		0,	VAR_DVDIRQ_HNDLR(3)	# zero out our addr of __DVDInterruptHandler in case the next DOL is loaded
	mtlr	12

	mr		4,	21
	mr		3,	20
	blrl							# jump to the handler

not_ready_yet:
no_fake_irq:
	mr		12,	22
	mr		4,	21
	mr		3,	20

	mtlr	12
	blrl
	
	lmw     19,	0x2C(1)
	lwz     0,	0x64(1)
	addi    1,	1,	0x60
	mtlr    0
	blr
	
	.globl queue_read
queue_read:
	mflr    	0             		# move lr  
	stwu    	1,  -64(1)   		# grow stack frame 
	stw     	0,   28(1)    		# save lr
	stw			3,	12(1)
	stw			4,	16(1)
	stw			5,	20(1)
	stw			6,	24(1)
	bl			add_read_to_queue
	lwz			3,	12(1)
	lwz			4,	16(1)
	lwz			5,	20(1)
	lwz			6,	24(1)
	lwz     	0,	28(1)    		# load lr
	addi    	1,  1, 64   		# grow stack frame 
	mtlr    	0             		# move lr  
	li			0, 	0				# patched over this
	blr
	
	.globl dcache_flush_icache_inv
dcache_flush_icache_inv:
	clrlwi. 	5, 3, 27  # check for lower bits set in address
	beq 1f
	addi 		4, 4, 0x20 
1:
	addi 		4, 4, 0x1f
	srwi 		4, 4, 5
	mtctr 		4
2:
	dcbf 		0, 3
	addi 		3, 3, 0x20
	bdnz 		2b
	sc
	sync
	isync
	blr

	.globl process_read_queue
process_read_queue:
	stwu	1,	-160(1) 		# grow stack frame
	stw		0,   12(1)			# store r0
	stmw	3,   16(1)			# store r3-r31

	bl		process_queue

	lwz		0,   12(1)			# restore r0
	lwz		4,	20(1)			# restore r4 (context)
	lwz     3, 128 (4)
	mtcr    3					# restore cr
	lwz     3, 0x0084 (4)
	mtlr    3					# restore lr
	lwz     3, 0x0088 (4)
	mtctr   3					# restore ctr
	lwz     3, 0x008C (4)
	mtxer   3					# restore xer
	lmw 	3,   16(1)			# restore r3-r31
	addi	1,	1, 160			# kill stack frame
	lis		5,	0x8000
	lwz		5,  0x3010(5)
	mtsrr0	5					# setup for the rfi
	rfi							# jump to handler

.globl patched_memcpy
patched_memcpy:
	cmpwi	3, 0x0500
	mr		4, 24
	bnelr
	lis		4, ext_handler_custom@h
	ori		4, 4, ext_handler_custom@l
	blr

.globl ext_handler_custom
ext_handler_custom:
	mtsprg0     4
	lwz         4, 0x00C0 (0)
	stw         3, 12 (4)
	mfsprg0     3
	stw         3, 16 (4)
	stw         5, 20 (4)
	lhz         3, 0x01A2 (4)
	ori         3, 3, 0x0002
	sth         3, 0x01A2 (4)
	mfcr        3
	stw         3, 128 (4)
	mflr        3
	stw         3, 0x0084 (4)
	mfctr       3
	stw         3, 0x0088 (4)
	mfxer       3
	stw         3, 0x008C (4)
	mfsrr0      3
	stw         3, 0x0198 (4)
	mfsrr1      3
	stw         3, 0x019C (4)
	mr          5, 3
	nop
	mfmsr       3
	ori         3, 3, 0x0030
	mtsrr1      3
	li          3, 4
	lwz         4, 0x00D4 (0)
	rlwinm.     5, 5, 0, 30, 30
	bne-        ext_h
	lis         5, 0x8008
	subi        5, 5, 28720
	mtsrr0      5
	rfi
ext_h:
	lis			5, process_read_queue@h
	ori			5, 5, process_read_queue@l
	mtsrr0      5
	rfi

	
