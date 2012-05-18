#include "../asm.h"

#	CARDReadAsync - "Reads data from a file asynchronously."
#	r3	CARDFileInfo *
#	r4	buffer	(32 bytes aligned)
#	r5	length	(multiple of 512)
#	r6	offset	(multiple of 512)
#	r7	cb

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	Function will never be called for Slot 1 so don't deal with that case

.globl CARDReadAsync
CARDReadAsync:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x20(%sp)

#CODE ME	
	cmpwi	%r7,	0
	beq		skip_cb
	mtctr	%r7
	li      %r3,	0
	li      %r4,	0
	bctrl

skip_cb:	
	li      %r3,	0
	li      %r4,	0

	lwz     %r0, 0x24(%sp)
	addi    %sp, %sp, 0x20 
	mtlr    %r0
	blr

	.globl CARDReadAsync_length
	CARDReadAsync_length:
	.long (CARDReadAsync_length -CARDReadAsync)