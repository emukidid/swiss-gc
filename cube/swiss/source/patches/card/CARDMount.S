#include "../asm.h"

#	CARDMount - "Mounts a memory card synchronously."
#	r3	Channel
#	r4	workarea	-	 5 * 8 * 1024 bytes
#	r5	detachCallback

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_MEMCARD_WORK,	-0x10	# Memory card work area 40960 bytes big

.globl CARDMount
CARDMount:
	cmpwi	%r3, 0
	beq		CARDPresent

	li      %r3, -3
	b		end

CARDPresent:	
	li		%r3, 0

# Store the workarea ptr so that we can use it in our code
	mtctr	%r3
	lis		%r3, VAR_AREA
	stw		%r4, VAR_MEMCARD_WORK(%r3)
	mfctr	%r3
end:
	blr

	.globl CARDMount_length
	CARDMount_length:
	.long (CARDMount_length -CARDMount)