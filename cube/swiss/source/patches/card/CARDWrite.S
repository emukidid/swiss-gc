#include "../asm.h"

#	CardWrite - "Writes data to a file synchronously."
#	r3	file *
#	r4	buffer (32 bytes aligned)
#	r5	length (multiple of 8192)
#	r6	offset (multiple of 8192)

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	Function will never be called for Slot 1 so don't deal with that case

.globl CardWrite
CardWrite:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x10(%sp)	

# CODE ME

	li      %r3,	0
	li      %r4,	0
	
	lwz     %r0,	0x14(%sp)
	addi    %sp,	%sp,	0x10
	mtlr    %r0
	blr

	.globl CARDWrite_length
	CardWrite_length:
	.long (CardWrite_length -CardWrite)