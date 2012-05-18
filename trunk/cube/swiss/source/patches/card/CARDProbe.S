#include "../asm.h"

#	CARDProbe - "Checks if some object is inserted in the specified slot. The inserted object could be a non-memory card device."
#	r3	Channel

#	return slot 0 true
#	return slot 1 false

.globl CARDProbe
CARDProbe:

	mflr	%r0
	cmpwi	%r3, 0
	beq		CARDPresent

	li      %r3, 0
	b		end

CARDPresent:
	li		%r3, 1
end:

	mtlr    %r0
	blr

	.globl CARDProbe_length
	CARDProbe_length:
	.long (CARDProbe_length -CARDProbe)