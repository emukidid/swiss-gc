#include "../asm.h"

#	CardWriteAsync - "Writes data to a file asynchronously."
#	r3	file *
#	r4	buffer (32 bytes aligned)
#	r5	length (multiple of 8192)
#	r6	offset (multiple of 8192)
#	r7	cb

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	Function will never be called for Slot 1 so don't deal with that case

.globl CardWriteAsync
CardWriteAsync:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x10(%sp)	

# CODE ME
	cmpwi	%r7,	0
	beq		skip_cb
	mtctr	%r7
	li      %r3,	0
	li      %r4,	0
	bctrl

skip_cb:	
	li      %r3,	0
	li      %r4,	0
	
	lwz     %r0,	0x14(%sp)
	addi    %sp,	%sp,	0x10
	mtlr    %r0
	blr

	.globl CARDWriteAsync_length
	CardWriteAsync_length:
	.long (CardWriteAsync_length -CardWriteAsync)