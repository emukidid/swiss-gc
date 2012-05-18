#include "../asm.h"

#	CARDCreate - "Creates a file synchronously."
#	r3	chan
#	r4	fileName	-	32 bytes long
#	r5	size		-	Multiple of card sector size
#	r6	fileInfo

#	532 bytes to play with here (133 instructions)

#	return slot 0 CARD_RESULT_EXIST or CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDCreate
CARDCreate:

	cmpwi	%r3, 0
	beq		DoCode
	li		%r3,	-3
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

	.globl CARDCreate_length
	CARDCreate_length:
	.long (CARDCreate_length - CARDCreate)