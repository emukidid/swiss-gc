#include "../asm.h"

#	CARDSetStatusAsync - "Sets a file status using a file number asynchronously."
#	r3	chan
#	r4	fileNo
#	r5	stat
#	r6	cb

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDSetStatusAsync
CARDSetStatusAsync:

	cmpwi	%r3,	0
	beq		DoCode
	li		%r3,	0
	li		%r4,	-3
	blr

DoCode:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x10(%sp)
			
# CODE ME

	cmpwi	%r6,	0
	beq		skip_cb
	mtctr	%r6
	li      %r3,	0
	li      %r4,	0
	bctrl

skip_cb:	
	li      %r3,	0
	li      %r4,	0

	lwz     %r0, 0x14(%sp)
	addi    %sp, %sp, 0x10
	mtlr    %r0
	blr

	.globl CARDSetStatusAsync_length
	CARDSetStatusAsync_length:
	.long (CARDSetStatusAsync_length -CARDSetStatusAsync)