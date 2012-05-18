#include "../asm.h"

#	CARDMount - "Mounts a memory card synchronously."
#	r3	Channel
#	r4	workarea	-	 5 * 8 * 1024 bytes
#	r5	detachCallback

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDMount
CARDMount:
	cmpwi	%r3, 0
	beq		CARDPresent

	li      %r3, -3
	b		end

CARDPresent:	
	li		%r3, 0

# TODO - Store the workarea ptr so that we can use it in our code
	
end:
	blr

	.globl CARDMount_length
	CARDMount_length:
	.long (CARDMount_length -CARDMount)