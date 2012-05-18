#include "../asm.h"

#	CARDDeleteAsync - "Deletes a file asynchronously."
#	r3	chan
#	r4	fileName
#	r5	cb

#	return slot 0 CARD_RESULT_NOFILE or CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDDeleteAsync
CARDDeleteAsync:

	cmpwi	%r3, 0
	beq		DoCode
	li		%r3,	0
	li		%r4,	-3
	blr

DoCode:

	mflr    %r0
	stw     %r0,	4(%sp)
	stwu    %sp,	-0x10(%sp)	
				
#CODE ME

	cmpwi	%r5,	0
	beq		skip_cb
	mtlr	%r5
	li      %r3,	0
	li      %r4,	0
	blrl

skip_cb:	

	lwz     %r0,	0x14(%sp)
	addi    %sp,	%sp,	0x10
	mtlr    %r0
	blr

	.globl CARDDeleteAsync_length
	CARDDeleteAsync_length:
	.long (CARDDeleteAsync_length - CARDDeleteAsync)