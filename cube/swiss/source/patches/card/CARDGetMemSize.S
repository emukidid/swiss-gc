#include "../asm.h"

#	CARDGetMemSize - "Gets the memory size of the specified memory card in M bits. The size returned will be 4M, 8M, 16M, 32M, 64M, or 128M bits."
#	r3	Channel
#	r4	size(u16)

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDGetMemSize
CARDGetMemSize:

	cmpwi	%r3,	0
	beq		DoCode
	li		%r3,	-3
	blr

DoCode:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x10(%sp)	

#possible sizes 4,8,16,32,64,128
	li		%r3,	8
	sth		%r3,	0(%r4)

end:
	li		%r3,	0

	lwz     %r0, 0x14(%sp)
	addi    %sp, %sp, 0x10
	mtlr    %r0
	blr

	.globl CARDGetMemSize_length
	CARDGetMemSize_length:
	.long (CARDGetMemSize_length -CARDGetMemSize)