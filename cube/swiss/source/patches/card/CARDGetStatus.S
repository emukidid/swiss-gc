#include "../asm.h"

#	CARDGetStatus - "Gets a file status using a file number."
#	r3	chan
#	r4	FileNo
#	r5	Stat

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDGetStatus
CARDGetStatus:

	cmpwi	%r3,	0
	beq		DoCode
	li		%r3,	0
	li		%r4,	-3
	blr

DoCode:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x10(%sp)	
			
#CODE ME

	lwz     %r0, 0x14(%sp)
	addi    %sp, %sp, 0x10
	mtlr    %r0
	blr

	.globl CARDGetStatus_length
	CARDGetStatus_length:
	.long (CARDGetStatus_length -CARDGetStatus)