#include "../asm.h"

#	CARDCheck - "Checks for the file system consistency of the memory card and repair it if possible."
#	r3	Channel

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDCheck
CARDCheck:


	mflr	%r0
	cmpwi	%r3, 0
	beq		CARDPresent

	li      %r3, -3
	b		end

CARDPresent:
	li		%r3, 0
end:

	mtlr    %r0
	blr

   .globl CARDCheck_length
   CARDCheck_length:
   .long (CARDCheck_length - CARDCheck)