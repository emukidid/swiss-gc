#include "../asm.h"

#	CARDGetResultCode - "CARDGetResultCode returns the result code of the previously issued CARD function call."
#	r3	Channel

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDGetResultCode
CARDGetResultCode:

	cmpwi	%r3, 0
	beq		CARDPresent

	li      %r3, -3
	b		end

CARDPresent:
	li		%r3,	0
end:

	blr

	.globl CARDGetResultCode_length
	CARDGetResultCode_length:
	.long (CARDGetResultCode_length -CARDGetResultCode)