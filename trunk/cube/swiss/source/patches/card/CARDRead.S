#include "../asm.h"

#	CARDRead - "Reads data from a file synchronously."
#	r3	CARDFileInfo *
#	r4	buffer	(32 bytes aligned)
#	r5	length	(multiple of 512)
#	r6	offset	(multiple of 512)

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	Function will never be called for Slot 1 so don't deal with that case

.globl CARDRead
CARDRead:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x20(%sp)

#CODE ME
	
	li      %r3,	0
	li      %r4,	0

	lwz     %r0, 0x24(%sp)
	addi    %sp, %sp, 0x20 
	mtlr    %r0
	blr

	.globl CARDRead_length
	CARDRead_length:
	.long (CARDRead_length -CARDRead)