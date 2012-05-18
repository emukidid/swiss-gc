#include "../asm.h"

#	CARDProbeEX - "This function verifies whether a memory card (of any type) is inserted in the specified slot."
#	r3	Channel
#	r4	MemSize (can be zero ptr) - "The memory card size returned, in megabits, will be 4, 8, 16, 32, 64, or 128 Mb."
#	r5	SecSize (can be zero ptr) - "The sector size returned, in bytes, will be 8, 16, 32, 64, or 128 KB."

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDProbeEX
CARDProbeEX:

	mflr	%r0
	cmpwi	%r3, 0
	beq		CARDPresent

	li      %r3, -3
	b		end

CARDPresent:
	cmpwi	%r4, 0
	beq ZeroMemPtr

	li		%r3, 8					# 8 MBit Card
	stw		%r3, 0(%r4)

ZeroMemPtr:
	cmpwi	%r5, 0
	beq ZeroSecPtr

	li		%r3, 0x2000				# 8192 byte sectors
	stw		%r3, 0(%r5)

ZeroSecPtr:
	li		%r3, 0

end:
	mtlr    %r0
	blr

	.globl CARDProbeEX_length
	CARDProbeEX_length:
	.long (CARDProbeEX_length -CARDProbeEX)