#include "../asm.h"

#	CARDCreateAsync - "Creates a file asynchronously."
#	r3	chan
#	r4	fileName	-	32 bytes long
#	r5	size		-	Multiple of card sector size
#	r6	fileInfo
#	r7	cb

#	532 bytes to play with here (133 instructions)

#	return slot 0 CARD_RESULT_EXIST or CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDCreateAsync
CARDCreateAsync:

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

	cmpwi	%r7,	0
	beq		skip_cb
	mtlr	%r7
	li      %r3,	0
	li      %r4,	0
	blrl

skip_cb:	

	lwz     %r0,	0x14(%sp)
	addi    %sp,	%sp,	0x10
	mtlr    %r0
	blr

	.globl CARDCreateAsync_length
	CARDCreateAsync_length:
	.long (CARDCreateAsync_length - CARDCreateAsync)