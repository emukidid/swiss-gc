#include "../asm.h"

#	CARDDelete - "Deletes a file synchronously."
#	r3	chan
#	r4	fileName

#	return slot 0 CARD_RESULT_NOFILE or CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDDelete
CARDDelete:

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

	lwz     %r0,	0x14(%sp)
	addi    %sp,	%sp,	0x10
	mtlr    %r0
	blr

	.globl CARDDelete_length
	CARDDelete_length:
	.long (CARDDelete_length - CARDDelete)