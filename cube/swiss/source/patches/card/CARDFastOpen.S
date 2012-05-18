#include "../asm.h"

#	CARDFastOpen - "Opens a file using a file number."
#	r3	chan
#	r4	FileNo
#	r5	FileInfo

#	return slot 0 CARD_RESULT_NOFILE or CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDFastOpen
CARDFastOpen:

	mflr    %r0
	stw     %r0,	4(%sp)
	stwu    %sp,	-0x10(%sp)	

	cmpwi	%r3,	0
	bne		NoCard
			
#CODE ME
	b		end
	
NoCard:
	li		%r3,	-3
end:

	lwz     %r0,	0x14(%sp)
	addi    %sp,	%sp,	0x10
	mtlr    %r0
	blr

	.globl CARDFastOpen_length
	CARDFastOpen_length:
	.long (CARDFastOpen_length - CARDFastOpen)