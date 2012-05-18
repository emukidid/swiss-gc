#include "../asm.h"

#	CARDOpen - "Opens a file."
#	r3	chan
#	r4	FileName
#	r5	FileInfo

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDOpen
CARDOpen:

	mflr    %r0
	stw     %r0,	4(%sp)
	stwu    %sp,	-0x10(%sp)	

	cmpwi	%r3,	0
	bne		NoCard
			
#	CODE ME
	b		end
	
NoCard:
	li		%r3,	-3
end:
	mr		%r4,	%r3

	lwz     %r0,	0x14(%sp)
	addi    %sp,	%sp,	0x10
	mtlr    %r0
	blr

	.globl CARDOpen_length
	CARDOpen_length:
	.long (CARDOpen_length -CARDOpen)