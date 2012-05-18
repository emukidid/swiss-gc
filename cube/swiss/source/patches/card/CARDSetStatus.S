#include "../asm.h"

#	CARDSetStatus - "Sets a file status using a file number synchronously."
#	r3	chan
#	r4	fileNo
#	r5	CARDStat* stat

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDSetStatus
CARDSetStatus:

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

	li      %r3,	0
	li      %r4,	0

	lwz     %r0, 0x14(%sp)
	addi    %sp, %sp, 0x10
	mtlr    %r0
	blr

	.globl CARDSetStatus_length
	CARDSetStatus_length:
	.long (CARDSetStatus_length -CARDSetStatus)